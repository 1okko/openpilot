// bigmodeld：C4 上行进程（16 号）。生命周期跟随 modeld（process_config only_onroad）：
//   camered VisionIPC 两路取帧 → warp 成 512×256 NV12 进编码缓冲（协议 v2，App 不再 warp）
//   → 按 frame_id 配对 → V4L 双路硬编
//   （HEVC Main、VBR、无 B 帧、GOP 20、默认 2 Mb/s/路可调）→ 10 号布局组 FRAME
//   经 TCP 发出（road 出包即发）；REPLY 独立线程收下解析并转交 modeld。不落盘、不进 loggerd
//   （设了编码输出回调 ⇒ 不建 PubMaster ⇒ loggerd 无编码数据）。
//
// 实现口径见本目录 README「实现口径」；纯逻辑在 frame_scheduler / uplink_sender /
// frame_meta / frame_codec，宿主单测 test_bigmodeld.cc。
//
// 线程与锁序（全文件统一，防死锁）：
//   取帧×2（state_mtx：配对/状态机/索引 + 编码缓冲池）、编码器 dequeue×2（输出回调：
//   meta 缓存 → sender.submit_*）、writer（sender.step）、REPLY reader（read_some →
//   parse → msgq）、标定/输入元数据（SubMaster → MetaProvider.set_rpy/set_model_inputs）。
//   锁序：sender.mtx > state_mtx > {ev_mtx, meta 缓存, buf 池, 分段计时}（叶子锁，绝不反向嵌套）。
//   sender 事件回调在其锁内触发 ⇒ 回调只记账 + 推事件队列（ev_mtx），状态机转移与
//   request_keyframe 在下一次组帧前由 drain_events_locked() 按序执行（事件仍落在
//   「事件后的第一个提交帧」，且不会在回调里反向取 state_mtx）。

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "common/params.h"
#include "common/timing.h"
#include "common/util.h"
#include "cereal/messaging/messaging.h"
#include "system/loggerd/encoder/v4l_encoder.h"
#include "system/loggerd/loggerd.h"

#include <media/msm_media_info.h>

#include "frame_codec.h"
#include "frame_meta.h"
#include "frame_scheduler.h"
#include "frame_stages.h"
#include "meta_cache.h"
#include "pair_matcher.h"
#include "server_locator.h"
#include "uplink_sender.h"
#include "warp_pack.h"

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

ExitHandler do_exit;

namespace {

constexpr int kFps = 20;
constexpr int kGopSize = 20;
constexpr int kDefaultBitrate = 2'000'000;  // 2 Mb/s/路（512×256 下约 4 KB/帧/路）
// 编码输入 = warp 后 512×256 NV12，按 venus 对齐布局（QBUF 要求 plane.length ≥ S_FMT sizeimage，21 号 EINVAL 坑）
const size_t kEncStride = VENUS_Y_STRIDE(COLOR_FMT_NV12, chipmunk::kModelW);
const size_t kEncUvOffset = kEncStride * VENUS_Y_SCANLINES(COLOR_FMT_NV12, chipmunk::kModelH);
const size_t kEncBufSize = VENUS_BUFFER_SIZE(COLOR_FMT_NV12, chipmunk::kModelW, chipmunk::kModelH);
constexpr int kEncPoolDepth = 6;   // 每路编码缓冲池（1 配对槽 + ≤5 在编码器在途）
constexpr int kSchedFifoPrio = 53;
// writer/reply 线程：core 3 有 pandad（FIFO 54，常驻 ~36%）抢占 → 编码完到 send 等 1～8 ms；
// core 2 只有 RT 5 的 locationd 系（台架 A/B/A：RTT p90 62.4→56.2 ms）。EINVAL（离线核）容忍
const std::vector<int> kCpuAffinity = {2};
// capture 线程做 warp（查找表 ~0.3 ms/路，标定变化那帧建表 ~1 ms），FIFO 50。
// core 4/5 上有 FIFO 53 的 controlsd/card/ui，capture 排队使取帧 p50 比 camerad 出帧晚 ~7 ms。
// core 7 有 modeld（FIFO 54，每帧跑 ~9.6 ms）：road 在此每帧排队 ~4.3 ms，取帧 p50 30.1 ms（modeld 24.4 ms）。
// core 6 有 camerad（TS）：wide 的短突发抢占它，让 road 帧对所有订阅者晚 ~0.7 ms；road 也放这里则 modeld 晚 ~1 ms。
// road→core 2（RT 5 的 locationd 系 + 本进程 writer/reply，帧到达时都被抢占或错开）、wide→core 6，
// 台架 jungle 实测：取帧 p50 30.1→24.5 ms，大模型 REPLY p50 54.6→49.6 ms / p99 61.2→53.9 ms，
// modeld 收帧不变，locationd inputsOK 无异常。wide 也放 core 2 只再快 ~0.7 ms，却让 FIFO 50 线程
// 更多压在 locationd 所在核（eagled 曾因高优先级拖慢 sensord 致 locationdTemporaryError），不取。
constexpr int kCaptureFifoPrio = 50;
const std::vector<int> kCaptureCore[2] = {{2}, {6}};

enum StreamId { kRoad = 0, kWide = 1 };

void setup_realtime(const char* who, int prio = kSchedFifoPrio, const std::vector<int>& cores = kCpuAffinity) {
  if (util::set_realtime_priority(prio) != 0) {
    LOGW("bigmodeld: %s SCHED_FIFO %d 失败 errno=%d", who, prio, errno);
  }
  if (util::set_core_affinity(cores) != 0) {
    LOGW("bigmodeld: %s 绑核失败（容忍）errno=%d", who, errno);
  }
}

// ---- TCP 上行 socket（可注入接缝的系统实现）----
// 非阻塞：write_some/read_some 语义对齐 UplinkSocket（>0 进展 / 0 会阻塞 / -1 错或 EOF）。
// 连接目标由 ServerLocator 给（06 号：手动 IP / 限当前 Wi-Fi 子网的 mDNS 发现）；
// 每次建连失败报 on_connect_failure（发现节流）。
class TcpSocket : public UplinkSocket {
 public:
  explicit TcpSocket(ServerLocator* loc) : loc_(loc) {}
  ~TcpSocket() override { close(); }

  bool connect(int timeout_ms) override {
    std::lock_guard<std::mutex> lk(mtx_);
    close_locked();

    ServerEndpoint ep;
    if (!loc_->resolve(&ep)) {
      loc_->on_connect_failure();  // 没找到服务也算一次失败（持续重试）
      return false;
    }

    struct addrinfo hints = {}, *res = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    char portstr[16];
    std::snprintf(portstr, sizeof portstr, "%d", int(ep.port));
    if (getaddrinfo(ep.host.c_str(), portstr, &hints, &res) != 0 || res == nullptr) {
      loc_->on_connect_failure();
      return false;
    }

    int fd = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0) {
      freeaddrinfo(res);
      loc_->on_connect_failure();
      return false;
    }
    int fl = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, fl | O_NONBLOCK);

    int rc = ::connect(fd, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    if (rc != 0) {
      if (errno != EINPROGRESS) {
        ::close(fd);
        loc_->on_connect_failure();
        return false;
      }
      struct pollfd pfd = {.fd = fd, .events = POLLOUT, .revents = 0};
      if (poll(&pfd, 1, timeout_ms) <= 0) {
        ::close(fd);
        loc_->on_connect_failure();
        return false;
      }
      int err = 0;
      socklen_t el = sizeof err;
      if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &el) != 0 || err != 0) {
        ::close(fd);
        loc_->on_connect_failure();
        return false;
      }
    }
    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
    fd_ = fd;
    return true;
  }

  int write_some(const uint8_t* data, size_t len) override {
    std::lock_guard<std::mutex> lk(mtx_);
    if (fd_ < 0) return -1;
    ssize_t n = ::send(fd_, data, len, MSG_NOSIGNAL);
    if (n > 0) return (int)n;
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return 0;
    return -1;
  }

  int read_some(uint8_t* data, size_t len) override {
    std::lock_guard<std::mutex> lk(mtx_);
    if (fd_ < 0) return -1;
    ssize_t n = ::recv(fd_, data, len, 0);
    if (n > 0) return (int)n;
    if (n == 0) return -1;  // EOF = 断连
    if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
    return -1;
  }

  void close() override {
    std::lock_guard<std::mutex> lk(mtx_);
    close_locked();
  }

  // 可读即返回（REPLY 到达即醒，不睡满轮询周期）；poll 在锁外，免得挡住发送线程。
  // 期间 fd 被并发关闭只会让 poll 提前返回，调用方随后 read_some 自会报错重连。
  void wait_readable(int timeout_ms) {
    int fd;
    {
      std::lock_guard<std::mutex> lk(mtx_);
      fd = fd_;
    }
    if (fd < 0) return;
    struct pollfd pfd = {.fd = fd, .events = POLLIN, .revents = 0};
    poll(&pfd, 1, timeout_ms);
  }

 private:
  void close_locked() {
    if (fd_ >= 0) {
      ::close(fd_);
      fd_ = -1;
    }
  }

  std::mutex mtx_;
  int fd_ = -1;
  ServerLocator* loc_;
};

// ---- 编码缓冲池：VisionIPC 帧拷进 plane.length ≥ sizeimage 的自有缓冲（21 号坑）----
// encode_frame 把缓冲排队进 V4L（最多 BUF_IN_COUNT 在途），编码器 dequeue 线程经
// input_done_callback 归还。acquire 阻塞 = 上游背压。
class BufPool {
 public:
  void init(size_t len, size_t width, size_t height, size_t stride, size_t uv_offset) {
    std::lock_guard<std::mutex> lk(mtx_);
    len_ = len;
    bufs_.resize(kEncPoolDepth);
    for (auto& b : bufs_) {
      b.allocate(len);
      b.init_yuv(width, height, stride, uv_offset);
      free_.push_back(&b);
    }
  }

  ~BufPool() {
    for (auto& b : bufs_) b.free();
  }

  VisionBuf* acquire() {
    std::unique_lock<std::mutex> lk(mtx_);
    cv_.wait(lk, [this] { return !free_.empty(); });
    VisionBuf* b = free_.back();
    free_.pop_back();
    return b;
  }

  void release(VisionBuf* b) {
    {
      std::lock_guard<std::mutex> lk(mtx_);
      free_.push_back(b);
    }
    cv_.notify_one();
  }

  size_t len() const { return len_; }

 private:
  std::mutex mtx_;
  std::condition_variable cv_;
  std::vector<VisionBuf> bufs_;
  std::vector<VisionBuf*> free_;
  size_t len_ = 0;
};

// ---- 组帧提交上下文：meta_cache.h（查表 + miss 分类，宿主单测）----

struct EncoderCtx {
  BufPool pool;
  MetaCache meta;
  std::unique_ptr<V4LEncoder> enc;
  std::vector<uint8_t> au;  // 回调线程私有（每编码器一条 dequeue 线程）
};

// ---- 主状态 ----
class Bigmodeld {
 public:
  Bigmodeld(ServerLocator* loc, int bitrate)
      : sock_(loc), bitrate_(bitrate), cur_bitrate_(bitrate) {
    sender_ = std::make_unique<UplinkSender>(
        &sock_, [] { return (uint64_t)millis_since_boot(); }, UplinkSenderConfig{},
        [this](const UplinkEventInfo& e) { on_uplink_event(e); });
    // 07 号：进程启动即写 connecting——BigmodelLinkState 保留上趟旧值，
    // 不写则新一趟起步会读到上趟 connected 闪绿
    link_mark_connecting();
  }

  // ===== 取帧/配对线程（×2）=====
  void capture_thread(StreamId sid) {
    setup_realtime(sid == kRoad ? "bgm_road" : "bgm_wide", kCaptureFifoPrio, kCaptureCore[sid]);

    VisionStreamType type = sid == kRoad ? VISION_STREAM_NARROW_ROAD : VISION_STREAM_WIDE_ROAD;
    VisionIpcClient vipc("camerad", type, false);
    chipmunk::WarpLut warp_lut;  // 本线程独占：标定不变时每帧只按表取像素
    bool inited = false;

    while (!do_exit) {
      if (!vipc.connect(false)) {
        util::sleep_for(100);
        continue;
      }
      if (!inited) {
        init_encoder(sid);
        inited = true;
      }
      while (!do_exit) {
        VisionIpcBufExtra extra;
        VisionBuf* buf = vipc.recv(&extra, 100);
        if (buf == nullptr) continue;
        // 上游覆盖/滞后（缓冲被新帧顶掉）：不进编码器；后续 SOF 可形成时间槽空洞
        if (buf->get_frame_id() != extra.frame_id) continue;
        const uint64_t recv_ns = nanos_since_boot();
        // warp 进编码缓冲后立即可放回 VisionIPC 缓冲（配对等待不占上游缓冲）
        VisionBuf* dst = ctx_[sid].pool.acquire();
        float mat[9];
        meta_.warp(sid == kWide, mat);
        const chipmunk::Nv12View src{buf->y, buf->uv, (int)buf->width, (int)buf->height,
                                     (int)buf->stride, (int)buf->stride};
        if (!warp_lut.warp(src, mat, chipmunk::Nv12Out{dst->y, dst->uv, (int)dst->stride, (int)dst->stride})) {
          LOGE("bigmodeld: %s warp 失败（源 %zux%zu stride %zu）", sid == kRoad ? "road" : "wide",
               buf->width, buf->height, buf->stride);
          ctx_[sid].pool.release(dst);
          continue;
        }
        place_frame(sid, extra, dst, mat, recv_ns, nanos_since_boot());
      }
    }
  }

  // ===== 发送线程 =====
  void writer_thread() {
    setup_realtime("bgm_writer");
    while (!do_exit) {
      if (!sender_->step()) sender_->wait_for_work(2);
    }
  }

  // ===== REPLY 接收线程（(e)）=====
  void reply_thread() {
    setup_realtime("bgm_reply");
    std::vector<uint8_t> buf;

    // 坏流/EOF/ERR 统一收尾：记断连 + 走重连路径 + 清半包缓冲（16 号 code-review：Duplicated Code）
    auto fail = [&](const char* why) {
      LOGE("bigmodeld: REPLY 流异常（%s），断连重连", why);
      sender_->notify_disconnect();
      hello_seen_ = false;
      link_mark_connecting();
      buf.clear();
    };

    while (!do_exit) {
      if (!sender_->connected()) {
        hello_seen_ = false;
        util::sleep_for(50);
        continue;
      }
      uint8_t tmp[8192];
      int n = sock_.read_some(tmp, sizeof tmp);
      if (n < 0) {
        // EOF/错误：走重连路径；统计保留（分段遥测跨连接累计）
        fail("EOF/错误");
        continue;
      }
      if (n == 0) {
        sock_.wait_readable(2);
        continue;
      }
      buf.insert(buf.end(), tmp, tmp + n);

      // MsgHdr 分流：HELLO/REPLY/ERR 定长整条解析，坏流即断连重来
      while (buf.size() >= bgm1::kMsgHdrSize) {
        const uint8_t type = buf[5];
        const uint32_t len = bgm1::read_u32_le(buf.data() + 12);
        size_t need = 0;
        if (type == bgm1::kTypeHello && len == bgm1::kHelloPayloadSize) {
          need = bgm1::kHelloWireSize;
        } else if (type == bgm1::kTypeReply && len == bgm1::kReplyPayloadSize) {
          need = bgm1::kReplyWireSize;
        } else if (type == bgm1::kTypeErr && len == bgm1::kErrPayloadSize) {
          need = bgm1::kErrWireSize;
        } else {
          LOGE("bigmodeld: REPLY 流协议错 type=0x%02x len=%u", type, len);
          fail("协议头非法");
          break;
        }
        if (buf.size() < need) break;

        if (type == bgm1::kTypeHello) {
          // 06 号：HELLO 必须是连接首条消息且每连接仅一条（ver/instance_id/max_frame）
          bgm1::Hello h;
          bgm1::Err pe = hello_seen_ ? bgm1::Err::kBadType : bgm1::parse_hello(buf.data(), need, &h);
          if (pe != bgm1::Err::kOk) {
            LOGE("bigmodeld: HELLO 解析失败 err=%d", int(pe));
            fail("HELLO 非法/重复");
            break;
          }
          hello_seen_ = true;
          max_segment_.store(h.max_frame, std::memory_order_relaxed);  // 段长上限以 HELLO 为准（spec）
          link_.on_hello(h.instance_id);
          link_state_write();
          LOGW("bigmodeld: HELLO instance_id=0x%llx max_frame=%u（%s）",
               (unsigned long long)h.instance_id, h.max_frame, link_.value().c_str());
        } else if (!hello_seen_) {
          fail("首条消息不是 HELLO");
          break;
        } else if (type == bgm1::kTypeReply) {
          const uint64_t reply_ns = nanos_since_boot();
          bgm1::Reply r;
          if (bgm1::parse_reply(buf.data(), need, &r) != bgm1::Err::kOk) {
            fail("REPLY 解析失败");
            break;
          }
          // 04 号 C：REPLY 经 msgq 转交 modeld（outputs[0:2066) 与 flags 原样镜像，
          // 逐帧都发——App 侧解码跳帧的全零 outputs 也发，落回小模型由 modeld 判）
          MessageBuilder msg;
          auto evt = msg.initEvent();
          auto br = evt.initBigModelReply();
          br.setTEof(r.t_eof);
          br.setFlags(r.flags);
          auto outs = br.initOutputs(bgm1::kReplyOutputsCount);
          for (size_t i = 0; i < bgm1::kReplyOutputsCount; i++) outs.set(i, r.outputs[i]);
          auto st = br.initStages();
          st.setPhoneTotalMs(r.telemetry[3] / 1e3f);
          StageMs s;
          if (stages_.on_reply(FrameIdx{r.frame_idx}, reply_ns, &s)) {
            st.setCaptureMs(s.capture);
            st.setWarpMs(s.warp);
            st.setPairWaitMs(s.pair_wait);
            st.setEncodeMs(s.encode);
            st.setSendMs(s.send);
            st.setReplyWaitMs(s.reply_wait);
          }
          pm_.send("bigModelReply", msg);
        } else {
          bgm1::ErrMsg e;
          if (bgm1::parse_err(buf.data(), need, &e) == bgm1::Err::kOk) {
            LOGE("bigmodeld: 服务端 ERR code=%u detail=%u frame_idx=%u", e.code, e.detail, e.frame_idx);
          }
          sender_->notify_disconnect();
          hello_seen_ = false;
          link_mark_connecting();
          buf.clear();
          break;
        }
        buf.erase(buf.begin(), buf.begin() + need);
      }
    }
  }

  // ===== 标定/输入元数据线程：extrinsicsCalibration.rpyCalib + modelDataV2SP → 帧头 =====
  void calib_thread() {
    SubMaster sm({"extrinsicsCalibration", "modelDataV2SP"});
    while (!do_exit) {
      sm.update(1000);
      if (sm.updated("extrinsicsCalibration")) {
        auto c = sm["extrinsicsCalibration"].getExtrinsicsCalibration();
        auto rpy = c.getRpyCalib();
        if (rpy.size() == 3) {
          float r[3] = {rpy[0], rpy[1], rpy[2]};
          meta_.set_rpy(r);
        }
      }
      // 04 号 C-2：modeld 权威元数据（电平采样，迟到 ≤2 帧按 04 号口径接受）
      if (sm.updated("modelDataV2SP")) {
        auto sp = sm["modelDataV2SP"].getModelDataV2SP();
        auto at = sp.getBigActionT();
        if (at.size() == 2) {
          float a[2] = {at[0], at[1]};
          meta_.set_model_inputs(a, sp.getDesireClass());
        }
      }
    }
  }

 private:
  // 帧负载：配对状态机槽内携带（配对键/序号在 PairMatcher::Frame 上）
  struct Slot {
    VisionIpcBufExtra extra = {};
    VisionBuf* buf = nullptr;
    float mat[9] = {0};  // 本帧 warp 实际用的矩阵（帧头遥测记它，不在配对时重读标定）
    uint64_t recv_ns = 0, warped_ns = 0;  // 分段计时：VisionIPC 收到 / warp 完
  };

  void init_encoder(StreamId sid) {
    LOGW("bigmodeld: %s 编码器 init %dx%d", sid == kRoad ? "road" : "wide", chipmunk::kModelW, chipmunk::kModelH);

    ctx_[sid].pool.init(kEncBufSize, chipmunk::kModelW, chipmunk::kModelH, kEncStride, kEncUvOffset);

    EncoderInfo info{};
    info.publish_name = sid == kRoad ? "bgmRoad" : "bgmWide";  // 仅日志标识（不建 PubMaster）
    info.fps = kFps;
    int bitrate = bitrate_;
    info.get_settings = [bitrate](int) {
      return EncoderSettings{.encode_type = cereal::EncodeIndex::Type::FULL_H_E_V_C,
                             .bitrate = bitrate,
                             .gop_size = kGopSize,
                             .b_frames = 0};
    };

    V4LEncoder::Options opt;
    opt.max_performance = true;  // venus realtime 优先级：512×256 编码 p90 5.1→1.9 ms（C4 实测）
    opt.output_callback = [this, sid](int, uint32_t, VisionIpcBufExtra& extra, unsigned int flags,
                                      kj::ArrayPtr<capnp::byte> header, kj::ArrayPtr<capnp::byte> dat) {
      on_encoded(sid, extra, flags, header, dat);
    };
    opt.input_done_callback = [this, sid](VisionBuf* b) { ctx_[sid].pool.release(b); };

    ctx_[sid].enc = std::make_unique<V4LEncoder>(info, chipmunk::kModelW, chipmunk::kModelH, opt);
    ctx_[sid].enc->encoder_open();
  }

  // ---- 配对 / 编码（持 state_mtx）----
  // 配对键 = timestamp_sof 邻近（见 pair_matcher.h：真机实测两路 frame_id 是各自
  // 独立的出帧计数器、持续漂移，不能作配对键）。配对死亡只计数/日志，不上报调度器；
  // road 死亡帧的 SOF 仍参与时间槽编号，wide 死亡帧不参与编号。
  void place_frame(StreamId sid, const VisionIpcBufExtra& extra, VisionBuf* buf, const float mat[9],
                   uint64_t recv_ns, uint64_t warped_ns) {
    std::lock_guard<std::mutex> lk(state_mtx_);
    drain_events_locked();

    const bool is_road = (sid == kRoad);
    Slot slot{extra, buf};
    std::memcpy(slot.mat, mat, sizeof slot.mat);
    slot.recv_ns = recv_ns;
    slot.warped_ns = warped_ns;
    bgm::PairMatcher<Slot>::Frame f{extra.frame_id, extra.timestamp_sof, slot};
    bgm::PairMatcher<Slot>::Actions a = matcher_.push(is_road, f);

    if (a.kill_road) {
      account_pair_drop_locked(true, a.road_dead);
      ctx_[kRoad].pool.release(a.road_dead.payload.buf);
    }
    if (a.kill_wide) {
      account_pair_drop_locked(false, a.wide_dead);
      ctx_[kWide].pool.release(a.wide_dead.payload.buf);
    }
    if (a.pair) {
      encode_pair_locked(a.road.payload, a.wide.payload);
    }
  }

  void encode_pair_locked(Slot road, Slot wide) {
    const CamFrameId frame_id{road.extra.frame_id};
    const FrameIdx frame_idx = indexer_.index(road.extra.timestamp_sof);

    // I 帧事件必须先于本帧 encode_frame（21 号实测「下一帧立即 I 帧」）：
    // 本帧自己的预测 + 事件累积的请求都在此刻发出（落点 = 事件后第一个提交帧），
    // 且此时两路编码器必已就绪（成对即两路都已 init_encoder）
    SchedStep s = sched_.on_frame_submit(frame_idx);
    accumulate_requests_locked(s);
    // 双路 request 同帧落点 = 本帧双路 IDR = App 侧新序列（bgm1_server.cpp new_seq：
    // 连接首帧 || 中流双路 IDR 对）。MODEL_ABI §4.3：新序列清 previousDesire，
    // 持续中的 desire 在新序列首帧重出 pulse（pulse 跟帧走，帧丢即丢，不重试不补发）。
    const bool new_seq = need_req_road_ && need_req_wide_;
    if (need_req_road_) {
      ctx_[kRoad].enc->request_keyframe();
      need_req_road_ = false;
    }
    if (need_req_wide_) {
      ctx_[kWide].enc->request_keyframe();
      need_req_wide_ = false;
    }
    if (new_seq) meta_.reset_desire_latch();

    update_bitrate_locked();

    bgm1::FrameHeader hdr;
    meta_.fill(road.extra.timestamp_eof, &hdr);
    std::memcpy(hdr.warp_road, road.mat, sizeof hdr.warp_road);
    std::memcpy(hdr.warp_wide, wide.mat, sizeof hdr.warp_wide);
    hdr.frame_idx = u32(frame_idx);

    OutMeta om;
    om.frame_id = frame_id;
    om.frame_idx = frame_idx;
    om.conn_epoch = ConnEpoch{cur_epoch_.load(std::memory_order_relaxed)};
    om.road_idr_pred = s.road_idr;
    om.wide_idr_pred = s.wide_idr;
    om.hdr = hdr;
    ctx_[kRoad].meta.push(om);
    om.hdr = {};
    // 查表键 = 各路自己的 frame_id（on_encoded 按本路 extra.frame_id 取回；两路
    // frame_id 计数器漂移，不能混用同一个键）
    om.frame_id = CamFrameId{wide.extra.frame_id};
    ctx_[kWide].meta.push(om);

    stages_.on_submit(frame_idx, road.extra.timestamp_eof, road.recv_ns, road.warped_ns, nanos_since_boot());
    // 两路同时提交编码；缓冲由 input_done_callback 归还池
    ctx_[kRoad].enc->encode_frame(road.buf, &road.extra);
    ctx_[kWide].enc->encode_frame(wide.buf, &wide.extra);
  }

  void account_pair_drop_locked(bool is_road, const bgm::PairMatcher<Slot>::Frame& dead) {
    if (is_road) {
      const FrameIdx frame_idx = indexer_.index(dead.timestamp_sof);
      LOGW("bigmodeld: 配对缺帧（road）frame_id=%u frame_idx=%u", dead.frame_id, u32(frame_idx));
    } else {
      LOGW("bigmodeld: 配对缺帧（wide）frame_id=%u", dead.frame_id);
    }
  }

  // 请求不在此刻执行、只累积（flush 在 encode_pair_locked，保证编码器已就绪且落点正确）
  void accumulate_requests_locked(const SchedStep& s) {
    need_req_road_ |= s.request_keyframe_road;
    need_req_wide_ |= s.request_keyframe_wide;
  }

  // Params "BigmodelEncoderBitrate" 每帧读（仿 encoderd.cc:54-60）；--bitrate 为初值
  void update_bitrate_locked() {
    static Params params;
    int b = bitrate_;
    std::string val = params.get("BigmodelEncoderBitrate");
    if (!val.empty()) {
      int v = std::atoi(val.c_str());
      if (v > 0) b = v;
    }
    if (b == cur_bitrate_) return;
    ctx_[kRoad].enc->set_bitrate(b);
    ctx_[kWide].enc->set_bitrate(b);
    cur_bitrate_ = b;
    LOGW("bigmodeld: 码率切换 %d bps", b);
  }

  // 链路状态串写 Param（06 号；putNonBlocking 异步，future 竞态用锁串行化）
  void link_state_write() {
    std::lock_guard<std::mutex> lk(link_param_mtx_);
    static Params params;
    params.putNonBlocking("BigmodelLinkState", link_.value());
  }
  // 断连/重连中（等 HELLO 判 blip/restart）
  void link_mark_connecting() {
    link_.on_connecting();
    link_state_write();
  }

  // ---- sender 事件（sender.mtx 锁内回调：只记账 + 推队列 + 日志，不取 state_mtx）----
  void on_uplink_event(const UplinkEventInfo& e) {
    switch (e.ev) {
      case UplinkEvent::kNewConnection:
        LOGW("bigmodeld: 新连接（frame_idx 与 I 帧状态重置）");
        link_mark_connecting();
        break;
      case UplinkEvent::kStall:
        LOGE("bigmodeld: 假死（≥200 ms 无进展）截断重连 frame_idx=%u", u32(e.frame_idx));
        link_mark_connecting();
        break;
      case UplinkEvent::kLinkLost:
        LOGE("bigmodeld: 连接丢失（连续重连失败，持续重试中）");
        link_.on_lost();
        link_state_write();
        break;
      case UplinkEvent::kDrop:
        LOGW("bigmodeld: 丢帧（发送队列覆盖/编码输出缺帧）frame_idx=%u", u32(e.frame_idx));
        break;
      case UplinkEvent::kStreamGap:
        LOGW("bigmodeld: 码流断档（编码输出被吞，main 合成）");
        break;
      case UplinkEvent::kHeadBarrier:
        LOGW("bigmodeld: 序列头门丢弃（非双路 IDR 对不开流）frame_idx=%u", u32(e.frame_idx));
        break;
      case UplinkEvent::kFrameSent:
        stages_.on_sent(e.frame_idx, nanos_since_boot());
        break;
    }
    std::lock_guard<std::mutex> lk(ev_mtx_);
    evs_.push_back(e);
  }

  // 事件按序落状态机：request_keyframe 累积到下一次组帧前统一发出 = 事件后第一个提交帧
  void drain_events_locked() {
    std::vector<UplinkEventInfo> batch;
    {
      std::lock_guard<std::mutex> lk(ev_mtx_);
      batch.swap(evs_);
    }
    for (const auto& e : batch) {
      SchedStep s;
      switch (e.ev) {
        case UplinkEvent::kNewConnection:
          cur_epoch_.store(u32(e.conn_epoch), std::memory_order_relaxed);
          indexer_.reset();
          // 旧连接的编码在途输出不带进新连接（重连清空队列同款）
          ctx_[kRoad].meta.clear();
          ctx_[kWide].meta.clear();
          s = sched_.on_connect();
          break;
        case UplinkEvent::kDrop:
        case UplinkEvent::kHeadBarrier:
          // 码流断档 / 序列头门丢弃：都开新序列 + 双路 request（对的下一提交帧起 IDR）
          s = sched_.on_frame_dropped(e.frame_idx);
          break;
        case UplinkEvent::kStreamGap:
          // 编码输出被吞（情形 C）：无 frame_idx 键、无重复上报，不去重
          s = sched_.on_stream_gap();
          break;
        default:
          break;
      }
      accumulate_requests_locked(s);
    }
  }

  // ---- 编码输出回调（编码器 dequeue 线程）----
  void on_encoded(StreamId sid, VisionIpcBufExtra& extra, unsigned int flags,
                  kj::ArrayPtr<capnp::byte> header, kj::ArrayPtr<capnp::byte> dat) {
    MetaCache::Result r = ctx_[sid].meta.take(CamFrameId{extra.frame_id});
    if (r.st != MetaTake::kHit) {
      if (r.st == MetaTake::kStaleMiss) {
        // 旧连接/已清/已弹的迟到输出（连接建立窗口）：序列头门兜底，静默丢弃
        LOGW("bigmodeld: %s 旧连接迟到编码输出，丢弃 frame_id=%u",
             sid == kRoad ? "road" : "wide", extra.frame_id);
        return;
      }
      // 查无且非旧 = 编码输出被吞：码流断档（情形 C）——显式上报调度器
      //（新序列 + 双路 request）+ 序列头门重新关门（断档后首帧必须双路 IDR）。
      // kStreamGap 不入 frame_idx 键空间、不去重（同 frame_id 不会二次输出，
      // 跨路同值也不再互相吞掉——18 号 review 魔法位 hack 的收编，19 号 #2）。
      LOGE("bigmodeld: %s 编码输出无对应提交（码流断档）frame_id=%u",
           sid == kRoad ? "road" : "wide", extra.frame_id);
      sender_->notify_stream_gap();
      {
        std::lock_guard<std::mutex> lk(ev_mtx_);
        evs_.push_back({UplinkEvent::kStreamGap, FrameIdx{}});
      }
      return;
    }

    const OutMeta& om = r.om;
    stages_.on_encoded(om.frame_idx, nanos_since_boot());
    // 命中前缀死条目（FIFO 下输出已丢）同样 = 码流断档：上报 + 关门（各路 dedup 到 frame_idx）
    if (!r.dead.empty()) {
      LOGE("bigmodeld: %s 编码输出缺帧 %zu 个（frame_idx=%u 起）码流断档",
           sid == kRoad ? "road" : "wide", r.dead.size(), u32(r.dead.front().frame_idx));
      sender_->notify_stream_gap();
      std::lock_guard<std::mutex> lk(ev_mtx_);
      evs_.push_back({UplinkEvent::kDrop, r.dead.front().frame_idx});
    }

    const bool keyframe = (flags & V4L2_BUF_FLAG_KEYFRAME) != 0;
    const uint8_t* data = reinterpret_cast<const uint8_t*>(dat.begin());
    size_t len = dat.size();
    if (keyframe && header.size() > 0) {
      // IDR AU 自带 VPS/SPS/PPS：v4l_encoder 每帧都给保存的 codec config，
      // 只在实际 keyframe 时拼在 AU 前（v4l_encoder.cc:120-133）
      EncoderCtx& c = ctx_[sid];
      c.au.resize(header.size() + dat.size());
      std::memcpy(c.au.data(), header.begin(), header.size());
      std::memcpy(c.au.data() + header.size(), dat.begin(), dat.size());
      data = c.au.data();
      len = c.au.size();
    }

    // 段长超协议上限（spec「线协议」= HELLO 的 max_frame，默认 1 MiB，服务端判 BAD_FRAME）：
    // 帧死亡，不进发送路径（丢帧上报 + 新序列 + 序列头门关门，经事件队列按序落状态机）
    const uint32_t max_seg = max_segment_.load(std::memory_order_relaxed);
    if (len > max_seg) {
      LOGE("bigmodeld: %s 段长 %zu 超上限 %u，丢帧 frame_idx=%u",
           sid == kRoad ? "road" : "wide", len, max_seg, u32(om.frame_idx));
      sender_->notify_stream_gap();
      std::lock_guard<std::mutex> lk(ev_mtx_);
      evs_.push_back({UplinkEvent::kDrop, om.frame_idx});
      return;
    }

    if (sid == kRoad) {
      // bit0 = road 实际 keyframe 位（精确）、bit1 = wide IDR 策略预测
      sender_->submit_road(om.conn_epoch, om.frame_idx, om.hdr, data, len,
                           keyframe, om.road_idr_pred, om.wide_idr_pred);
    } else {
      sender_->submit_wide(om.conn_epoch, om.frame_idx, data, len, keyframe);
    }
  }

  // ---- 成员（锁序：sender_ > state_mtx_ > {ev_mtx_, MetaCache, BufPool}）----
  MetaProvider meta_;
  TcpSocket sock_;
  std::unique_ptr<UplinkSender> sender_;
  PubMaster pm_{{"bigModelReply"}};  // 04 号 C：REPLY 经 msgq 转交 modeld
  FrameStageLog stages_;  // C4 本机分段计时（随 REPLY 上报）

  // 06 号：链路状态（HELLO instance_id 对比 → blip/restart）→ Param "BigmodelLinkState"（07 号读）
  LinkStateTracker link_;
  bool hello_seen_ = false;       // 本连接已收 HELLO（reply 线程独占）
  std::atomic<uint32_t> max_segment_{bgm1::kDefaultMaxSegment};  // 段长上限（HELLO 一到即更新）
  std::mutex link_param_mtx_;     // 串行化 Param 写（putNonBlocking 的 future 非线程安全）

  std::mutex state_mtx_;
  FrameScheduler sched_;
  FrameIndexer indexer_;
  bgm::PairMatcher<Slot> matcher_;

  bool need_req_road_ = false;  // 事件累积的 request_keyframe，组帧前统一 flush
  bool need_req_wide_ = false;
  // 连接代号（sender 建连次数，kNewConnection 事件带出）：组帧时 stamp 进 OutMeta，
  // submit 侧与 sender 当前代号不符即拒（重连窗口内旧输出不得混进新连接）
  std::atomic<uint32_t> cur_epoch_{0};  // ConnEpoch 的裸存储（atomic 强类型无加法）

  std::mutex ev_mtx_;
  std::vector<UplinkEventInfo> evs_;

  EncoderCtx ctx_[2];
  int bitrate_;
  int cur_bitrate_;
};

void usage() {
  fprintf(stderr,
          "usage: bigmodeld [--host HOST|auto] [--port PORT] [--bitrate BPS]\n"
          "  --host 缺省读 Params \"BigmodelServerHost\"（空 = mDNS 自动发现，限 wlan0 子网）\n");
}

}  // namespace

int main(int argc, char* argv[]) {
  std::string host;  // --host：手动 IP / "auto"；缺省走 Params 与自动发现（06 号）
  int port = 7070;
  int bitrate = kDefaultBitrate;

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    auto need_val = [&](const char* what) -> const char* {
      if (i + 1 >= argc) {
        fprintf(stderr, "bigmodeld: %s 缺参数值\n", what);
        usage();
        exit(2);
      }
      return argv[++i];
    };
    if (arg == "--host") {
      host = need_val("--host");
    } else if (arg == "--port") {
      port = std::atoi(need_val("--port"));
    } else if (arg == "--bitrate") {
      bitrate = std::atoi(need_val("--bitrate"));
    } else {
      fprintf(stderr, "bigmodeld: 未知参数 %s\n", arg.c_str());
      usage();
      return 2;
    }
  }
  if (port <= 0 || port > 65535 || bitrate <= 0) {
    usage();
    return 2;
  }

  // 06 号连接目标：--host 显式给定优先（adb reverse/联调用）；否则读 Params
  // "BigmodelServerHost"（07 号设置页写）：非空 = 手动 IP，空 = 自动发现（限 wlan0 子网）。
  std::string fixed = host;
  if (fixed.empty()) {
    Params params;
    fixed = params.get("BigmodelServerHost");
    while (!fixed.empty() && fixed.back() == '\n') fixed.pop_back();
  }
  if (fixed == "auto") fixed.clear();

  std::unique_ptr<ServerLocator> loc;
  if (!fixed.empty()) {
    ServerEndpoint ep;
    ep.host = fixed;
    ep.port = uint16_t(port);
    loc = std::make_unique<FixedLocator>(ep);
    LOGW("bigmodeld: start host=%s port=%d bitrate=%d（手动）", fixed.c_str(), port, bitrate);
  } else {
    // 发现范围每次现探测（Wi-Fi 断开/换网后范围跟着变）
    loc = std::make_unique<AvahiLocator>([](ServerEndpoint* out) {
      SubnetScope s;
      if (!detect_wifi_scope(&s)) return false;  // Wi-Fi 没起：发现不到，持续重试
      return browse_avahi(out, s);
    });
    LOGW("bigmodeld: start host=auto（mDNS 限 wlan0 子网）port=%d bitrate=%d", port, bitrate);
  }

  Bigmodeld bg(loc.get(), bitrate);
  std::vector<std::thread> ts;
  ts.emplace_back(&Bigmodeld::capture_thread, &bg, kRoad);
  ts.emplace_back(&Bigmodeld::capture_thread, &bg, kWide);
  ts.emplace_back(&Bigmodeld::writer_thread, &bg);
  ts.emplace_back(&Bigmodeld::reply_thread, &bg);
  ts.emplace_back(&Bigmodeld::calib_thread, &bg);
  for (auto& t : ts) t.join();

  LOGW("bigmodeld: exit");
  return 0;
}
