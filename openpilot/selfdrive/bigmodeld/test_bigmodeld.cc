// 宿主单测（16 号）：纯逻辑模块全转移 + 发送器（假 socket/假时钟）+ warp golden + 编解码往返。
// 无设备依赖，Mac/Linux 直接编译（README「宿主测试」）：
//   clang++ -std=c++17 -O1 test_bigmodeld.cc frame_codec.cpp frame_meta.cpp \
//           frame_scheduler.cpp uplink_sender.cpp -o /tmp/test_bigmodeld && /tmp/test_bigmodeld

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include "frame_codec.h"
#include "frame_meta.h"
#include "frame_scheduler.h"
#include "frame_stages.h"
#include "meta_cache.h"
#include "pair_matcher.h"
#include "server_locator.h"
#include "uplink_sender.h"

// ---- 极简测试框架 ----
static int g_checks = 0;
static int g_fails = 0;

#define CHECK(cond)                                              \
  do {                                                           \
    g_checks++;                                                  \
    if (!(cond)) {                                               \
      g_fails++;                                                 \
      printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);     \
    }                                                            \
  } while (0)

#define CHECK_NEAR(a, b, tol)                                                    \
  do {                                                                           \
    g_checks++;                                                                  \
    double _a = (a), _b = (b), _t = (tol);                                       \
    if (!(std::fabs(_a - _b) <= _t)) {                                           \
      g_fails++;                                                                 \
      printf("FAIL %s:%d: |%g - %g| > %g\n", __FILE__, __LINE__, _a, _b, _t);    \
    }                                                                            \
  } while (0)

// =====================================================================
// FrameScheduler / FrameIndexer：I 帧策略、码流断档恢复与时间槽编号
// =====================================================================

// 连接起始 + GOP20 基准 + wide 晚 10 帧（road 0,20,40,…；wide 0,10,30,50,…）
static void test_sched_gop() {
  FrameScheduler s;
  SchedStep c = s.on_connect();
  CHECK(c.request_keyframe_road && c.request_keyframe_wide);
  CHECK(!c.drop_detected);

  // 新序列第 0 帧 = request 落点：两路 IDR
  SchedStep s0 = s.on_frame_submit(FrameIdx{0});
  CHECK(s0.road_idr && s0.wide_idr);
  CHECK(!s0.request_keyframe_road && !s0.request_keyframe_wide && !s0.drop_detected);

  for (uint32_t i = 1; i <= 9; i++) {
    SchedStep x = s.on_frame_submit(FrameIdx{i});
    CHECK(!x.any());
  }

  // 第 10 帧：wide 补 I（本帧发出 request、本帧即 IDR）
  SchedStep s10 = s.on_frame_submit(FrameIdx{10});
  CHECK(s10.request_keyframe_wide && s10.wide_idr && !s10.road_idr);

  for (uint32_t i = 11; i <= 19; i++) CHECK(!s.on_frame_submit(FrameIdx{i}).any());

  // road GOP 20
  SchedStep s20 = s.on_frame_submit(FrameIdx{20});
  CHECK(s20.road_idr && !s20.wide_idr && !s20.request_keyframe_wide);

  for (uint32_t i = 21; i <= 29; i++) CHECK(!s.on_frame_submit(FrameIdx{i}).any());

  // wide 基准 30（0,10,30,50,…）
  SchedStep s30 = s.on_frame_submit(FrameIdx{30});
  CHECK(s30.wide_idr && !s30.road_idr && !s30.request_keyframe_wide);

  for (uint32_t i = 31; i <= 39; i++) CHECK(!s.on_frame_submit(FrameIdx{i}).any());

  SchedStep s40 = s.on_frame_submit(FrameIdx{40});
  CHECK(s40.road_idr && !s40.wide_idr);
  for (uint32_t i = 41; i <= 49; i++) CHECK(!s.on_frame_submit(FrameIdx{i}).any());

  SchedStep s50 = s.on_frame_submit(FrameIdx{50});
  CHECK(s50.wide_idr && !s50.road_idr);
}

// 发送侧显式丢弃（kDrop）→ 新序列（seq 归零、两路 request、再走相位）
static void test_sched_drop() {
  FrameScheduler s;
  s.on_connect();
  s.on_frame_submit(FrameIdx{0});
  s.on_frame_submit(FrameIdx{1});

  SchedStep d = s.on_frame_dropped(FrameIdx{2});  // 发送侧显式丢弃，不是配对失败
  CHECK(d.drop_detected && d.request_keyframe_road && d.request_keyframe_wide);

  // 落点 = 事件后第一个提交帧（新序列第 0 帧）
  SchedStep s3 = s.on_frame_submit(FrameIdx{3});
  CHECK(s3.road_idr && s3.wide_idr && !s3.drop_detected);
  for (uint32_t i = 4; i <= 12; i++) CHECK(!s.on_frame_submit(FrameIdx{i}).any());
  SchedStep s13 = s.on_frame_submit(FrameIdx{13});  // 新序列第 10 帧
  CHECK(s13.request_keyframe_wide && s13.wide_idr);
  for (uint32_t i = 14; i <= 22; i++) CHECK(!s.on_frame_submit(FrameIdx{i}).any());
  SchedStep s23 = s.on_frame_submit(FrameIdx{23});  // 新序列第 20 帧
  CHECK(s23.road_idr && !s23.wide_idr);
}

// 同一时间槽重复显式丢弃只恢复一次（例如同帧两路编码段都超限）
static void test_sched_drop_dedup() {
  FrameScheduler s;
  s.on_connect();
  s.on_frame_submit(FrameIdx{0});

  SchedStep d1 = s.on_frame_dropped(FrameIdx{2});
  CHECK(d1.drop_detected && d1.request_keyframe_road && d1.request_keyframe_wide);
  SchedStep d2 = s.on_frame_dropped(FrameIdx{2});  // 重复上报：静默
  CHECK(!d2.any());
  SchedStep d3 = s.on_frame_dropped(FrameIdx{3});  // 不同帧照常
  CHECK(d3.drop_detected && d3.request_keyframe_road && d3.request_keyframe_wide);
  SchedStep d4 = s.on_frame_dropped(FrameIdx{3});  // 同一槽第二次上报：同样静默
  CHECK(!d4.any());

  // 重连后 frame_idx 重新编号：去重窗口作废
  s.on_connect();
  SchedStep d5 = s.on_frame_dropped(FrameIdx{3});
  CHECK(d5.drop_detected);
}

// 19 号 #2 回归：编码输出被吞（kStreamGap）无键、不去重——两路 frame_id 同值时
// 两次断档上报都必须生效（旧版 0x80000000u|frame_id 魔法位键会吞掉第二路）
static void test_sched_stream_gap_no_dedup() {
  FrameScheduler s;
  s.on_connect();
  s.on_frame_submit(FrameIdx{0});

  SchedStep g1 = s.on_stream_gap();  // road 输出被吞
  CHECK(g1.drop_detected && g1.request_keyframe_road && g1.request_keyframe_wide);
  SchedStep g2 = s.on_stream_gap();  // wide 同 frame_id 输出同样被吞：不得被去重压掉
  CHECK(g2.drop_detected && g2.request_keyframe_road && g2.request_keyframe_wide);
}

// frame_idx 是时间槽：提交序出现空槽不恢复，GOP 相位仍按实际提交帧数推进
static void test_sched_submit_time_slot_hole() {
  FrameScheduler s;
  s.on_connect();
  const uint32_t slots[] = {0, 1, 3, 5, 6, 8, 9, 11, 12, 13, 15};
  for (size_t i = 0; i < sizeof(slots) / sizeof(slots[0]); i++) {
    SchedStep step = s.on_frame_submit(FrameIdx{slots[i]});
    if (i == 0) {
      CHECK(step.road_idr && step.wide_idr);
    } else if (i == 10) {
      CHECK(step.request_keyframe_wide && step.wide_idr);
    } else {
      CHECK(!step.any());
    }
    CHECK(!step.drop_detected);
  }
}

// 50 ms 一个时间槽；SOF 100 ms 跳跃对应 +2。重连重置时间基准。
static void test_frame_indexer() {
  constexpr uint64_t t0 = 1000000000ULL;
  constexpr uint64_t ms = 1000000ULL;
  FrameIndexer indexer;

  FrameIdx idx = indexer.index(t0);
  CHECK(u32(idx) == 0);
  uint32_t prev = u32(idx);
  idx = indexer.index(t0 + 50 * ms);
  CHECK(u32(idx) == prev + 1);
  prev = u32(idx);
  idx = indexer.index(t0 + 150 * ms);  // 相邻 SOF 相差 100 ms，跨两个槽
  CHECK(u32(idx) == prev + 2);
  prev = u32(idx);
  idx = indexer.index(t0 + 200 * ms);
  CHECK(u32(idx) == prev + 1);

  // 连接重置后，首个被编号 SOF 成为新基准，即使绝对 SOF 已向前很久也从 0 起。
  indexer.reset();
  CHECK(indexer.index(t0 + 10000 * ms) == FrameIdx{0});
  CHECK(indexer.index(t0 + 10050 * ms) == FrameIdx{1});
}

// =====================================================================
// UplinkSender：假 socket / 假时钟
// =====================================================================

struct FakeSock : public UplinkSocket {
  enum Mode { kWrite, kBlock, kErr };
  bool connect_ok = true;
  int connect_calls = 0;
  Mode mode = kWrite;
  size_t max_write = (size_t)-1;  // 每次 write_some 最多写多少（模拟部分写）
  std::function<void()> on_write;  // 每次成功写出后回调（推进假时钟模拟写入耗时）
  std::vector<uint8_t> written;
  bool closed = false;

  bool connect(int timeout_ms) override {
    (void)timeout_ms;
    connect_calls++;
    closed = false;
    return connect_ok;
  }
  int write_some(const uint8_t* data, size_t len) override {
    if (mode == kBlock) return 0;
    if (mode == kErr) return -1;
    size_t n = std::min(len, max_write);
    written.insert(written.end(), data, data + n);
    if (on_write) on_write();
    return (int)n;
  }
  int read_some(uint8_t* data, size_t len) override {
    (void)data;
    (void)len;
    return 0;
  }
  void close() override { closed = true; }
};

struct Ev {
  UplinkEvent ev;
  uint32_t idx;
  uint32_t ep;  // kNewConnection 的连接代号
};

static bgm1::FrameHeader test_hdr() {
  bgm1::FrameHeader h;
  h.t_eof = 123456789ULL;
  h.desire[0] = 0.5f;
  h.traffic_convention[0] = 1.f;
  h.action_t[0] = 0.1f;
  for (int i = 0; i < 9; i++) {
    h.warp_road[i] = (float)(i + 1);
    h.warp_wide[i] = (float)(10 + i);
  }
  return h;
}

// 序列头门下的标准开局：预测 + 实测双路 IDR 头对开门（唯一开流条件）
static void open_head(UplinkSender& s, uint32_t ep, uint32_t idx,
                      const uint8_t* road, size_t road_len,
                      const uint8_t* wide, size_t wide_len) {
  s.submit_road(ConnEpoch{ep}, FrameIdx{idx}, test_hdr(), road, road_len, /*road_actual=*/true,
                /*road_pred=*/true, /*wide_pred=*/true);
  s.submit_wide(ConnEpoch{ep}, FrameIdx{idx}, wide, wide_len, /*wide_actual=*/true);
}

// 出包即发：序列头门放行后（头对 = 预测+实测双路 IDR），中流 road 即可先发 chunk1，
// wide 到齐才发 chunk2 报 kFrameSent
static void test_sender_road_out_goes() {
  FakeSock sock;
  uint64_t now = 1000;
  std::vector<Ev> evs;
  UplinkSender s(&sock, [&] { return now; }, UplinkSenderConfig{},
                 [&](const UplinkEventInfo& e) { evs.push_back({e.ev, u32(e.frame_idx), u32(e.conn_epoch)}); });

  CHECK(s.step());  // 建连
  CHECK(s.connected() && sock.connect_calls == 1);
  CHECK(evs.size() == 1 && evs[0].ev == UplinkEvent::kNewConnection && evs[0].ep == 1);

  const uint8_t road[7] = {1, 2, 3, 4, 5, 6, 7};
  const uint8_t wide[5] = {9, 8, 7, 6, 5};

  // 头对（帧 0）：预测 + 实测双路 IDR 开流；两段到齐一次发完
  open_head(s, evs[0].ep, 0, road, sizeof road, wide, sizeof wide);
  CHECK(s.step());
  CHECK(evs.size() == 2 && evs[1].ev == UplinkEvent::kFrameSent && evs[1].idx == 0);
  CHECK(sock.written.size() == bgm1::frame_wire_size(sizeof road, sizeof wide));

  // 字节布局：头 ‖ road ‖ wide_len ‖ wide ‖ MAC(零)
  bgm1::FrameView fv;
  CHECK(bgm1::parse_frame(sock.written.data(), sock.written.size(), &fv) == bgm1::Err::kOk);
  CHECK(fv.hdr.frame_idx == 0 && fv.hdr.road_len == sizeof road && fv.hdr.wide_len == sizeof wide);
  CHECK(fv.hdr.t_eof == 123456789ULL);
  CHECK_NEAR(fv.hdr.desire[0], 0.5, 1e-6);
  CHECK_NEAR(fv.hdr.warp_road[8], 9.0, 1e-6);
  CHECK(std::memcmp(fv.road, road, sizeof road) == 0);
  CHECK(std::memcmp(fv.wide, wide, sizeof wide) == 0);
  for (size_t i = sock.written.size() - bgm1::kMacSize; i < sock.written.size(); i++) {
    CHECK(sock.written[i] == 0);
  }

  // 中流（帧 1）：road 出包即发——chunk1 先走，未报 kFrameSent（wide 缺）
  s.submit_road(ConnEpoch{evs[0].ep}, FrameIdx{1}, test_hdr(), road, sizeof road, false, false, false);
  CHECK(s.step());
  CHECK(sock.written.size() == bgm1::frame_wire_size(sizeof road, sizeof wide) +
                               bgm1::kFrameHdrSize + sizeof road);
  CHECK(evs.size() == 2);

  s.submit_wide(ConnEpoch{evs[0].ep}, FrameIdx{1}, wide, sizeof wide, false);
  CHECK(s.step());
  CHECK(evs.size() == 3 && evs[2].ev == UplinkEvent::kFrameSent && evs[2].idx == 1);
}

// flags：bit0 = road 实际 keyframe 位（精确）、bit1 = wide IDR 预测
static void test_sender_flags() {
  FakeSock sock;
  uint64_t now = 0;
  std::vector<Ev> evs;
  UplinkSender s(&sock, [&] { return now; }, UplinkSenderConfig{},
                 [&](const UplinkEventInfo& e) { evs.push_back({e.ev, u32(e.frame_idx), u32(e.conn_epoch)}); });
  s.step();

  const uint8_t p[3] = {1, 2, 3};
  // 头对（帧 7）：预测 + 实测双路 IDR 开流
  open_head(s, 1, 7, p, sizeof p, p, sizeof p);
  s.step();
  s.step();

  bgm1::FrameHeader h;
  CHECK(bgm1::parse_frame_header(sock.written.data(), &h) == bgm1::Err::kOk);
  CHECK(h.flags == (bgm1::kFlagRoadIdr | bgm1::kFlagWideIdr));
  CHECK(h.frame_idx == 7);
}

// 在途槽 + 排队槽：排队只留最新，新包覆盖 = 丢旧包；门开着丢 = kDrop（断档）→
// 门重新关门（断档后首帧必须双路 IDR，18 号 #1：替代帧若是 P 帧不得开流）；
// 迟到旧包静默丢
static void test_sender_queue_overwrite() {
  FakeSock sock;
  uint64_t now = 0;
  std::vector<Ev> evs;
  FrameScheduler sched;
  SchedStep connect_step;
  SchedStep drop_step;
  bool saw_drop = false;
  UplinkSender s(&sock, [&] { return now; }, UplinkSenderConfig{},
                 [&](const UplinkEventInfo& e) {
                   evs.push_back({e.ev, u32(e.frame_idx), u32(e.conn_epoch)});
                   if (e.ev == UplinkEvent::kNewConnection) {
                     connect_step = sched.on_connect();
                   } else if (e.ev == UplinkEvent::kDrop) {
                     saw_drop = true;
                     drop_step = sched.on_frame_dropped(e.frame_idx);
                   } else if (e.ev == UplinkEvent::kHeadBarrier) {
                     sched.on_frame_dropped(e.frame_idx);
                   }
                 });
  CHECK(s.step());
  CHECK(connect_step.request_keyframe_road && connect_step.request_keyframe_wide);
  evs.clear();

  sock.mode = FakeSock::kBlock;  // 写不动 → 头对帧 0 停在在途
  const uint8_t p[2] = {0xaa, 0xbb};
  SchedStep s0 = sched.on_frame_submit(FrameIdx{0});
  CHECK(s0.road_idr && s0.wide_idr);
  open_head(s, 1, 0, p, sizeof p, p, sizeof p);  // 头对开流
  s.step();  // queued → inflight，写阻塞

  CHECK(!sched.on_frame_submit(FrameIdx{1}).any());
  s.submit_road(ConnEpoch{1}, FrameIdx{1}, test_hdr(), p, sizeof p, false, false, false);
  s.submit_wide(ConnEpoch{1}, FrameIdx{1}, p, sizeof p, false);
  s.step();  // 进排队槽
  CHECK(evs.empty());

  // 新包覆盖排队槽 = 丢弃帧 1（kDrop，码流断档）→ 序列头门关门
  CHECK(!sched.on_frame_submit(FrameIdx{2}).any());
  s.submit_road(ConnEpoch{1}, FrameIdx{2}, test_hdr(), p, sizeof p, false, false, false);
  CHECK(evs.size() == 1 && evs[0].ev == UplinkEvent::kDrop && evs[0].idx == 1);
  CHECK(saw_drop && drop_step.drop_detected);
  CHECK(drop_step.request_keyframe_road && drop_step.request_keyframe_wide);

  // 替代帧 2 是 P 对 → 序列头门丢弃（kHeadBarrier），不得成为断档后首帧
  s.submit_wide(ConnEpoch{1}, FrameIdx{2}, p, sizeof p, false);
  s.step();
  CHECK(evs.size() == 2 && evs[1].ev == UplinkEvent::kHeadBarrier && evs[1].idx == 2);

  // 事件落状态机：断档/屏障后下一个提交对 = 双路 IDR（预测）
  SchedStep after = sched.on_frame_submit(FrameIdx{3});
  CHECK(after.road_idr && after.wide_idr);

  // 帧 1 的迟到包（如 wide）静默丢弃，不重建已丢帧
  s.submit_wide(ConnEpoch{1}, FrameIdx{1}, p, sizeof p, false);
  CHECK(evs.size() == 2);

  // 双路 IDR 对（帧 3）重新开流 → 通了以后发的是帧 0、帧 3
  open_head(s, 1, 3, p, sizeof p, p, sizeof p);
  sock.mode = FakeSock::kWrite;
  s.step();  // 在途帧 0 写完（kFrameSent）
  s.step();  // 帧 3 进在途并写完
  bool sent0 = false, sent3 = false;
  for (auto& e : evs) {
    if (e.ev == UplinkEvent::kFrameSent && e.idx == 0) sent0 = true;
    if (e.ev == UplinkEvent::kFrameSent && e.idx == 3) sent3 = true;
  }
  CHECK(sent0 && sent3);
}

// 假死（≥200 ms 无进展）：上报 kStall，写得动就发完在途帧再重连
static void test_sender_stall_finish() {
  FakeSock sock;
  uint64_t now = 0;
  std::vector<Ev> evs;
  UplinkSender s(&sock, [&] { return now; }, UplinkSenderConfig{},
                 [&](const UplinkEventInfo& e) {
                   evs.push_back({e.ev, u32(e.frame_idx), u32(e.conn_epoch)});
                   if (e.ev == UplinkEvent::kStall) {
                     // 假死时刻突然可写、但只能短写：补发必须连续推进直到发完（不撕裂）
                     sock.mode = FakeSock::kWrite;
                     sock.max_write = 3;
                   }
                 });
  s.step();
  evs.clear();

  sock.mode = FakeSock::kBlock;
  const uint8_t p[2] = {1, 2};
  open_head(s, 1, 0, p, sizeof p, p, sizeof p);  // 头对开流
  s.step();
  CHECK(evs.empty());

  now += 199;
  s.step();  // 未到阈值
  CHECK(evs.empty());

  now += 1;
  s.step();  // 达阈值：kStall → 尝试发完（可写 → 完整发出）→ 断开
  CHECK(evs.size() == 2);
  CHECK(evs[0].ev == UplinkEvent::kStall && evs[0].idx == 0);
  CHECK(evs[1].ev == UplinkEvent::kFrameSent && evs[1].idx == 0);
  CHECK(!s.connected());
  CHECK(sock.closed);

  CHECK(s.step());  // 下一次 step 重连（新连接代号 2，序列头门重新关门）
  CHECK(evs.size() == 3 && evs[2].ev == UplinkEvent::kNewConnection && evs[2].ep == 2);
}

// 假死且写不动：截断（不发 kFrameSent），重连清空
static void test_sender_stall_truncate() {
  FakeSock sock;
  uint64_t now = 0;
  std::vector<Ev> evs;
  UplinkSender s(&sock, [&] { return now; }, UplinkSenderConfig{},
                 [&](const UplinkEventInfo& e) { evs.push_back({e.ev, u32(e.frame_idx), u32(e.conn_epoch)}); });
  s.step();
  evs.clear();

  sock.mode = FakeSock::kBlock;
  const uint8_t p[2] = {1, 2};
  open_head(s, 1, 0, p, sizeof p, p, sizeof p);
  s.step();
  now += 200;
  s.step();  // kStall → 截断 → 重连（sock 仍 block）
  CHECK(evs.size() == 1 && evs[0].ev == UplinkEvent::kStall);

  sock.mode = FakeSock::kWrite;
  CHECK(s.step());  // 重连成功 → kNewConnection；旧帧不带进新连接
  CHECK(evs.size() == 2 && evs[1].ev == UplinkEvent::kNewConnection);

  const size_t before = sock.written.size();
  s.step();
  CHECK(sock.written.size() == before);  // 队列已清空，无残帧可发
}

// 重连：每次 connect 1 s 超时；连败 3 次升级 kLinkLost；此后持续重试、成功复位
static void test_sender_reconnect_linklost() {
  FakeSock sock;
  uint64_t now = 0;
  std::vector<Ev> evs;
  UplinkSender s(&sock, [&] { return now; }, UplinkSenderConfig{},
                 [&](const UplinkEventInfo& e) { evs.push_back({e.ev, u32(e.frame_idx), u32(e.conn_epoch)}); });
  s.step();
  CHECK(evs.size() == 1 && evs[0].ev == UplinkEvent::kNewConnection && evs[0].ep == 1);

  // 读侧发现断连 → 重连路径
  s.notify_disconnect();
  CHECK(!s.connected());

  sock.connect_ok = false;
  CHECK(!s.step());
  CHECK(!s.step());
  CHECK(evs.size() == 1);  // 前两次失败未升级
  CHECK(!s.step());  // 第 3 次失败 → 升级「连接丢失」
  CHECK(evs.size() == 2 && evs[1].ev == UplinkEvent::kLinkLost);

  CHECK(!s.step());  // 之后继续重试，不重复升级
  CHECK(evs.size() == 2);

  sock.connect_ok = true;
  CHECK(s.step());
  CHECK(evs.size() == 3 && evs[2].ev == UplinkEvent::kNewConnection && evs[2].ep == 2);
  CHECK(s.connected());
}

// 断连/未连接：submit_* 静默丢（不阻塞编码回调），队列不带进新连接
static void test_sender_submit_while_down() {
  FakeSock sock;
  uint64_t now = 0;
  std::vector<Ev> evs;
  UplinkSender s(&sock, [&] { return now; }, UplinkSenderConfig{},
                 [&](const UplinkEventInfo& e) { evs.push_back({e.ev, u32(e.frame_idx), u32(e.conn_epoch)}); });
  const uint8_t p[2] = {1, 2};

  s.submit_road(ConnEpoch{0}, FrameIdx{0}, test_hdr(), p, sizeof p, true, true, true);  // 未连接 → 丢
  s.submit_wide(ConnEpoch{0}, FrameIdx{0}, p, sizeof p, true);
  CHECK(evs.empty());
  CHECK(s.step());
  s.step();
  CHECK(evs.size() == 1);  // 只有 kNewConnection，无 kFrameSent

  // 建连后入队一帧（头对开流），再断连 → 旧帧不发
  sock.mode = FakeSock::kBlock;
  open_head(s, 1, 1, p, sizeof p, p, sizeof p);
  s.notify_disconnect();
  CHECK(!s.connected());
  sock.mode = FakeSock::kWrite;
  CHECK(s.step());
  s.step();
  CHECK(evs.size() == 2);  // kNewConnection ×2，帧 1 已随断连清空
}

// 部分写不撕裂：chunk1/chunk2 分多次 write_some 拼完整
static void test_sender_partial_writes() {
  FakeSock sock;
  sock.max_write = 3;
  uint64_t now = 0;
  std::vector<Ev> evs;
  UplinkSender s(&sock, [&] { return now; }, UplinkSenderConfig{},
                 [&](const UplinkEventInfo& e) { evs.push_back({e.ev, u32(e.frame_idx), u32(e.conn_epoch)}); });
  s.step();

  const uint8_t road[10] = {0};
  const uint8_t wide[7] = {1};
  open_head(s, 1, 0, road, sizeof road, wide, sizeof wide);
  for (int i = 0; i < 100 && evs.size() < 2; i++) {
    now++;
    s.step();  // 假时钟推进避免中途判假死
  }
  CHECK(evs.size() == 2 && evs[1].ev == UplinkEvent::kFrameSent && evs[1].idx == 0);
  CHECK(sock.written.size() == bgm1::frame_wire_size(sizeof road, sizeof wide));
}

// 写错误 → 重连
// 发送线程空闲等待：提交即唤醒（不睡满轮询周期），无提交则超时
static void test_sender_wait_for_work_wakes_on_submit() {
  using namespace std::chrono;
  FakeSock sock;
  uint64_t now = 0;
  UplinkSender s(&sock, [&] { return now; }, UplinkSenderConfig{}, nullptr);
  s.step();
  CHECK(!s.wait_for_work(1));

  const uint8_t p[3] = {1, 2, 3};
  std::thread submitter([&] {
    std::this_thread::sleep_for(milliseconds(20));
    s.submit_road(s.conn_epoch(), FrameIdx{0}, test_hdr(), p, sizeof p, true, true, true);
  });
  const auto t0 = steady_clock::now();
  CHECK(s.wait_for_work(2000));
  CHECK(steady_clock::now() - t0 < milliseconds(1000));
  submitter.join();
  CHECK(!s.wait_for_work(0));  // 唤醒已消费

  s.submit_wide(s.conn_epoch(), FrameIdx{0}, p, sizeof p, true);  // 等待前已提交也不丢
  CHECK(s.wait_for_work(0));
}

static void test_sender_write_error() {
  FakeSock sock;
  uint64_t now = 0;
  std::vector<Ev> evs;
  UplinkSender s(&sock, [&] { return now; }, UplinkSenderConfig{},
                 [&](const UplinkEventInfo& e) { evs.push_back({e.ev, u32(e.frame_idx), u32(e.conn_epoch)}); });
  s.step();

  sock.mode = FakeSock::kErr;
  const uint8_t p[2] = {1, 2};
  open_head(s, 1, 0, p, sizeof p, p, sizeof p);
  s.step();
  CHECK(!s.connected());
  sock.mode = FakeSock::kWrite;
  CHECK(s.step());
  CHECK(s.connected());
}

// =====================================================================
// LinkStateTracker（06 号）：HELLO instance_id 对比区分服务端重启/网络闪断，
// 对外（Param "BigmodelLinkState"）报状态串：connecting/connected/blip/restart/lost
// =====================================================================

static void test_link_state() {
  LinkStateTracker t;
  CHECK(t.value() == "connecting");  // 起步 = 连接中

  t.on_hello(0xA);
  CHECK(t.value() == "connected");   // 首次见到 instance = 已连接

  t.on_connecting();
  CHECK(t.value() == "connecting");  // 断连重连中
  t.on_hello(0xA);
  CHECK(t.value() == "blip");        // 同 instance 重连 = 网络闪断

  t.on_connecting();
  t.on_hello(0xB);
  CHECK(t.value() == "restart");     // instance 变了 = 服务端重启

  t.on_connecting();
  t.on_hello(0xB);
  CHECK(t.value() == "blip");        // 重启后再断再连同 instance = 闪断

  t.on_lost();
  CHECK(t.value() == "lost");        // 连败升级「连接丢失」
  t.on_hello(0xB);
  CHECK(t.value() == "blip");        // lost 后连回同 instance 仍算闪断

  t.on_connecting();
  t.on_hello(0xC);
  CHECK(t.value() == "restart");     // lost 后换 instance = 重启
}

// =====================================================================
// 06 号：mDNS 发现（avahi-browse 行解析 + 发现节流策略）
// =====================================================================

static void test_avahi_parse() {
  ServerEndpoint ep;
  // avahi-browse -rpt 可解析行：=;iface;IPv4;name;type;domain;host;ip;port;txt...
  CHECK(parse_avahi_browse_line(
      "=;wlan0;IPv4;8e5-uuid;_bigmodel._tcp;local;8e5.local;172.20.10.2;7070;\"ver\"=\"1\";\"name\"=\"8E5\"",
      &ep));
  CHECK(ep.host == "172.20.10.2" && ep.port == 7070 && ep.name == "8e5-uuid" && ep.iface == "wlan0");
  CHECK(!parse_avahi_browse_line("+;wlan0;IPv4;8e5;_bigmodel._tcp;local;8e5.local;172.20.10.2;7070", &ep));
  CHECK(!parse_avahi_browse_line("=;wlan0;IPv6;8e5;_bigmodel._tcp;local;8e5.local;fe80::1;7070", &ep));
  CHECK(!parse_avahi_browse_line("=;wlan0;IPv4;x;_http._tcp;local;x.local;1.2.3.4;80", &ep));
  CHECK(!parse_avahi_browse_line("garbage", &ep));
  CHECK(!parse_avahi_browse_line("=;wlan0;IPv4;8e5;_bigmodel._tcp;local;8e5.local;172.20.10.2", &ep));
}

static void test_avahi_scope() {
  // 06 号：只在 C4 当前连接的 Wi-Fi 子网内发现——接口对、IP 落在 wlan0 子网内才收
  SubnetScope scope;
  scope.iface = "wlan0";
  CHECK(parse_ipv4("172.20.10.2", &scope.local));
  CHECK(parse_ipv4("255.255.255.0", &scope.mask));

  ServerEndpoint e;
  e.host = "172.20.10.9"; e.iface = "wlan0";
  CHECK(in_scope(e, scope));            // 同接口同子网（手机热点）
  e.host = "172.20.10.254";
  CHECK(in_scope(e, scope));
  e.host = "192.168.1.7";
  CHECK(!in_scope(e, scope));           // 别的子网（家里路由/陈旧缓存）不收
  e.host = "172.20.11.9";
  CHECK(!in_scope(e, scope));           // 相邻子网不收
  e.host = "172.20.10.9"; e.iface = "eth0";
  CHECK(!in_scope(e, scope));           // 别的接口不收

  CHECK(!parse_ipv4("garbage", &scope.local));
  CHECK(!parse_ipv4("1.2.3", &scope.local));
}

static void test_avahi_locator() {
  int browses = 0;
  ServerEndpoint found{"10.0.0.5", 7070, "a"};
  AvahiLocator loc([&](ServerEndpoint* out) { browses++; *out = found; return true; }, 3);
  ServerEndpoint ep;
  CHECK(loc.resolve(&ep) && ep.host == "10.0.0.5" && browses == 1);  // 首次现查
  CHECK(loc.resolve(&ep) && browses == 1);                           // 缓存直连（闪断快速重连）
  loc.on_connect_failure();
  loc.on_connect_failure();
  CHECK(loc.resolve(&ep) && browses == 1);                           // 连败 < 3 仍用缓存
  loc.on_connect_failure();                                          // 第 3 次 = kLinkLost 升级点
  CHECK(loc.resolve(&ep) && browses == 2);                           // 回到 mDNS 发现
  found = {"10.0.0.9", 7070, "a"};
  loc.on_connect_failure();
  loc.on_connect_failure();
  loc.on_connect_failure();
  CHECK(loc.resolve(&ep) && ep.host == "10.0.0.9" && browses == 3);  // 手机换 IP 跟上

  AvahiLocator none([&](ServerEndpoint*) { return false; }, 3);      // 热点没开/App 没跑
  CHECK(!none.resolve(&ep));
  CHECK(!none.resolve(&ep));                                         // 没缓存，持续重试

  FixedLocator fix({"192.168.1.7", 7070, ""});                       // 手动 IP 兜底
  CHECK(fix.resolve(&ep) && ep.host == "192.168.1.7" && ep.port == 7070);
  fix.on_connect_failure();
  CHECK(fix.resolve(&ep) && ep.host == "192.168.1.7");               // 手动恒给定值，不发现
}

// =====================================================================
// frame_codec：往返 + 坏输入（冒烟）
// =====================================================================

static void test_frame_codec_roundtrip() {
  bgm1::FrameHeader h = test_hdr();
  h.flags = bgm1::kFlagRoadIdr;
  h.frame_idx = 33;
  h.road_len = 11;
  h.wide_len = 5;
  const uint8_t road[11] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
  const uint8_t wide[5] = {9, 9, 9, 9, 9};

  std::vector<uint8_t> buf(bgm1::frame_wire_size(h.road_len, h.wide_len));
  CHECK(bgm1::pack_frame(h, road, wide, buf.data(), buf.size()) == buf.size());
  CHECK(bgm1::pack_frame(h, road, wide, buf.data(), buf.size() - 1) == 0);  // cap 不足

  bgm1::FrameView fv;
  CHECK(bgm1::parse_frame(buf.data(), buf.size(), &fv) == bgm1::Err::kOk);
  CHECK(fv.hdr.flags == bgm1::kFlagRoadIdr && fv.hdr.road_idr() && !fv.hdr.wide_idr());
  CHECK(fv.hdr.frame_idx == 33 && fv.hdr.road_len == 11 && fv.hdr.wide_len == 5);
  CHECK(fv.hdr.t_eof == h.t_eof);
  for (int i = 0; i < 8; i++) CHECK_NEAR(fv.hdr.desire[i], h.desire[i], 1e-6);
  for (int i = 0; i < 9; i++) {
    CHECK_NEAR(fv.hdr.warp_road[i], h.warp_road[i], 1e-6);
    CHECK_NEAR(fv.hdr.warp_wide[i], h.warp_wide[i], 1e-6);
  }
  CHECK(std::memcmp(fv.road, road, sizeof road) == 0);
  CHECK(std::memcmp(fv.wide, wide, sizeof wide) == 0);

  // 坏输入
  std::vector<uint8_t> bad = buf;
  bad[0] ^= 0xff;
  CHECK(bgm1::parse_frame(bad.data(), bad.size(), &fv) == bgm1::Err::kBadMagic);
  bad = buf;
  bad[4] = 9;
  CHECK(bgm1::parse_frame(bad.data(), bad.size(), &fv) == bgm1::Err::kBadVersion);
  bad = buf;
  bad[5] = bgm1::kTypeReply;
  CHECK(bgm1::parse_frame(bad.data(), bad.size(), &fv) == bgm1::Err::kBadType);
  CHECK(bgm1::parse_frame(buf.data(), buf.size() - 1, &fv) == bgm1::Err::kLenMismatch);
  CHECK(bgm1::parse_frame(buf.data(), bgm1::kFrameMinSize - 1, &fv) == bgm1::Err::kTruncated);
  // road_len 超上限 → kLenTooLarge（不是截断）
  bad = buf;
  bad[12] = 0xff; bad[13] = 0xff; bad[14] = 0xff; bad[15] = 0xff;
  CHECK(bgm1::parse_frame(bad.data(), bad.size(), &fv) == bgm1::Err::kLenTooLarge);

  // HELLO 往返（06 号；同源副本守卫：与 android/ 的 frame_codec 同一实现）
  bgm1::Hello hello;
  hello.instance_id = 0x0123456789ABCDEFULL;
  hello.max_frame = 1u << 20;
  std::vector<uint8_t> hbuf(bgm1::kHelloWireSize);
  CHECK(bgm1::pack_hello(hello, hbuf.data(), hbuf.size()) == bgm1::kHelloWireSize);
  bgm1::Hello h2;
  CHECK(bgm1::parse_hello(hbuf.data(), hbuf.size(), &h2) == bgm1::Err::kOk);
  CHECK(h2.instance_id == hello.instance_id && h2.max_frame == hello.max_frame);
  CHECK(bgm1::parse_hello(hbuf.data(), bgm1::kHelloWireSize - 1, &h2) == bgm1::Err::kTruncated);
  // golden 逐字节钉死（android/tests/fixtures/hello_golden.bin 同一样例，跨仓库镜像互证）
  static const uint8_t kHelloGolden[bgm1::kHelloWireSize] = {
      0x42, 0x47, 0x4D, 0x31, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x10, 0x00, 0x00, 0x00, 0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01,
      0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  };
  CHECK(std::memcmp(hbuf.data(), kHelloGolden, bgm1::kHelloWireSize) == 0);
}

static void test_reply_err_roundtrip() {
  bgm1::Reply r;
  r.flags = bgm1::kFlagSeqReset | bgm1::kFlagZeroPair;
  r.frame_idx = 77;
  r.t_eof = 987654321ULL;
  for (size_t i = 0; i < bgm1::kReplyOutputsCount; i++) r.outputs[i] = (float)i * 0.25f;
  for (size_t i = 0; i < bgm1::kReplyTelemetryCount; i++) r.telemetry[i] = 1000 * (uint32_t)(i + 1);

  std::vector<uint8_t> buf(bgm1::kReplyWireSize);
  CHECK(bgm1::pack_reply(r, buf.data(), buf.size()) == bgm1::kReplyWireSize);
  bgm1::Reply out;
  CHECK(bgm1::parse_reply(buf.data(), buf.size(), &out) == bgm1::Err::kOk);
  CHECK(out.flags == r.flags && out.frame_idx == 77 && out.t_eof == r.t_eof);
  CHECK(out.seq_reset() && out.zero_pair());
  bool outputs_ok = true;
  for (size_t i = 0; i < bgm1::kReplyOutputsCount; i++) outputs_ok &= (out.outputs[i] == r.outputs[i]);
  CHECK(outputs_ok);
  for (size_t i = 0; i < bgm1::kReplyTelemetryCount; i++) CHECK(out.telemetry[i] == r.telemetry[i]);

  CHECK(bgm1::parse_reply(buf.data(), buf.size() - 1, &out) == bgm1::Err::kTruncated);
  std::vector<uint8_t> bad = buf;
  bad[5] = bgm1::kTypeFrame;
  CHECK(bgm1::parse_reply(bad.data(), bad.size(), &out) == bgm1::Err::kBadType);

  bgm1::ErrMsg e;
  e.code = bgm1::kErrVersionMismatch;
  e.detail = 2;
  e.frame_idx = 13;
  std::vector<uint8_t> ebuf(bgm1::kErrWireSize);
  CHECK(bgm1::pack_err(e, ebuf.data(), ebuf.size()) == bgm1::kErrWireSize);
  bgm1::ErrMsg eout;
  CHECK(bgm1::parse_err(ebuf.data(), ebuf.size(), &eout) == bgm1::Err::kOk);
  CHECK(eout.code == bgm1::kErrVersionMismatch && eout.detail == 2 && eout.frame_idx == 13);
  CHECK(bgm1::parse_err(ebuf.data(), ebuf.size() - 1, &eout) == bgm1::Err::kTruncated);
}

// =====================================================================
// warp golden（gen_warp_golden.py 生成）+ MetaProvider
// =====================================================================

// gen_warp_golden.py 生成（勿手改）：rpy 样本 + get_warp_matrix 的 warp_road/warp_wide
// 数值口径：rpy 按 float32 取值（与 extrinsicsCalibration.rpyCalib 同）后以 double 精度
// 算同一公式（rot_from_euler + matmul + inv），与 frame_meta.cpp 的 double 实现对得上；
// modeld 实际路径的 float32 中间精度另带 ~3e-5 绝对噪声（相对 ~1e-7），不进本判据。
// 内参（DEVICE_CAMERAS[('mici','os04c10')]）：narrow fl=1141.5 wide fl=425.25 size=(1344, 760)
struct WarpGolden { float rpy[3]; float road[9]; float wide[9]; };
static const WarpGolden kWarpGolden[] = {
  {{0.0f, 0.0f, 0.0f}, {1.2543956f, 1.63676136e-15f, 350.874725f, 0.0f, 1.2543956f, 320.290771f, 0.0f, 4.66480276e-18f, 1.0f}, {0.934615374f, 0.0f, 432.738464f, 0.0f, 0.934615374f, 238.125381f, 0.0f, 0.0f, 1.0f}},
  {{0.00999999978f, -0.0199999996f, 0.00499999989f}, {1.25047612f, -0.0273994152f, 358.74585f, 0.0103699304f, 1.24575233f, 340.795227f, -5.71396686e-06f, -2.19202393e-05f, 1.00229371f}, {0.926876485f, -0.0389001332f, 442.607697f, 0.00500151375f, 0.917722344f, 247.833038f, -1.14279337e-05f, -4.38404786e-05f, 1.00936806f}},
  {{-0.0299999993f, 0.0399999991f, -0.00999999978f}, {1.26027894f, 0.0668602735f, 334.209259f, -0.0339231156f, 1.26964402f, 282.278503f, 9.66581683e-06f, 4.42519668e-05f, 0.994569302f}, {0.947150171f, 0.0871339291f, 411.482483f, -0.0206658095f, 0.967079103f, 221.159439f, 1.93316337e-05f, 8.85039335e-05f, 0.980766356f}},
  {{0.0500000007f, 0.0500000007f, 0.0500000007f}, {1.21639955f, -0.0208257213f, 416.894348f, 0.0428127423f, 1.27312362f, 250.438828f, -5.2111991e-05f, 5.75299382e-05f, 1.00810432f}, {0.862358928f, 0.0329989865f, 465.77533f, 0.00704780966f, 0.976003528f, 207.835571f, -0.000104223982f, 0.000115059876f, 1.00671732f}},
  {{-0.100000001f, 0.0f, 0.100000001f}, {1.16853857f, 0.117244937f, 477.875916f, -0.166711017f, 1.24396694f, 361.566772f, -0.000109158973f, -1.09524299e-05f, 1.02347016f}, {0.778590679f, 0.078119643f, 499.919189f, -0.17626667f, 0.921622336f, 283.323578f, -0.000218317946f, -2.19048598e-05f, 1.05421877f}},
  {{0.0f, -0.150000006f, 0.0f}, {1.2543956f, -0.110354319f, 348.581757f, 0.0f, 1.17790735f, 490.24826f, 0.0f, -0.000164217738f, 0.996587813f}, {0.934615374f, -0.220708638f, 458.696198f, 0.0f, 0.799315155f, 317.945526f, 0.0f, -0.000328435475f, 1.03862762f}},
  {{0.200000003f, -0.200000003f, 0.300000012f}, {0.918125808f, -0.404265493f, 744.004028f, 0.107552513f, 1.15172613f, 500.216919f, -0.000359710044f, -0.000139892276f, 1.03503799f}, {0.380722493f, -0.419179767f, 718.52063f, -0.0914014503f, 0.79140842f, 343.538574f, -0.000719420088f, -0.000279784552f, 1.16293621f}},
  {{9.99999975e-05f, 0.000199999995f, -0.000300000014f}, {1.25461709f, 2.21553273e-05f, 350.474487f, 0.000250722631f, 1.25447905f, 319.994293f, 3.29692313e-07f, 2.19747236e-07f, 0.99990505f}, {0.935058415f, 0.000201822681f, 432.466797f, 0.000344027707f, 0.934782386f, 237.926895f, 6.59384625e-07f, 4.39494471e-07f, 0.999764442f}},
};
// 内参核对（与 frame_meta.cpp 对读）：
//   kNarrowRoadIntrinsics[9] = {1141.5f, 0.0f, 672.0f, 0.0f, 1141.5f, 380.0f, 0.0f, 0.0f, 1.0f}
//   kWideRoadIntrinsics[9] = {425.25f, 0.0f, 672.0f, 0.0f, 425.25f, 380.0f, 0.0f, 0.0f, 1.0f}

static void test_warp_golden() {
  // 内参常量与 camera.py _os_config（os04c10）逐位一致
  const float narrow[9] = {1141.5f, 0.f, 672.f, 0.f, 1141.5f, 380.f, 0.f, 0.f, 1.f};
  const float wide[9] = {425.25f, 0.f, 672.f, 0.f, 425.25f, 380.f, 0.f, 0.f, 1.f};
  for (int i = 0; i < 9; i++) {
    CHECK(kNarrowRoadIntrinsics[i] == narrow[i]);
    CHECK(kWideRoadIntrinsics[i] == wide[i]);
  }

  // rpy=0 标定性抽查：road[0] = fl_narrow/910、road[2] = cx − road[0]·256
  CHECK_NEAR(kWarpGolden[0].road[0], 1141.5 / 910.0, 1e-6);
  CHECK_NEAR(kWarpGolden[0].road[2], 672.0 - (1141.5 / 910.0) * 256.0, 1e-3);

  for (const WarpGolden& g : kWarpGolden) {
    float road[9], wwide[9];
    get_warp_matrix(g.rpy, kNarrowRoadIntrinsics, false, road);
    get_warp_matrix(g.rpy, kWideRoadIntrinsics, true, wwide);
    for (int i = 0; i < 9; i++) {
      CHECK_NEAR(road[i], g.road[i], 1e-5);
      CHECK_NEAR(wwide[i], g.wide[i], 1e-5);
    }
  }
}

static void test_meta_provider() {
  MetaProvider mp;
  bgm1::FrameHeader h;
  std::memset(&h, 0x5a, sizeof h);  // 脏内存，验证 fill 全覆盖
  mp.fill(42, &h);

  CHECK(h.t_eof == 42);
  CHECK(h.traffic_convention[0] == 1.f && h.traffic_convention[1] == 0.f);
  for (int i = 0; i < 8; i++) CHECK(h.desire[i] == 0.f);
  CHECK(h.action_t[0] == 0.f && h.action_t[1] == 0.f);
  // 默认 rpy=0 → warp 与 golden 首样本一致
  for (int i = 0; i < 9; i++) {
    CHECK_NEAR(h.warp_road[i], kWarpGolden[0].road[i], 1e-5);
    CHECK_NEAR(h.warp_wide[i], kWarpGolden[0].wide[i], 1e-5);
  }

  // 标定更新 → warp 跟随
  const WarpGolden& g = kWarpGolden[3];
  mp.set_rpy(g.rpy);
  mp.fill(43, &h);
  for (int i = 0; i < 9; i++) {
    CHECK_NEAR(h.warp_road[i], g.road[i], 1e-5);
    CHECK_NEAR(h.warp_wide[i], g.wide[i], 1e-5);
  }
  CHECK(h.t_eof == 43);

  // capture 线程 warp 用的矩阵 == 帧头矩阵（协议 v2：帧头记录 C4 已应用的矩阵）
  float road[9], wide[9];
  mp.warp(false, road);
  mp.warp(true, wide);
  CHECK(std::memcmp(road, h.warp_road, sizeof road) == 0);
  CHECK(std::memcmp(wide, h.warp_wide, sizeof wide) == 0);
}

// 04 号 C-2：modeld 上行元数据（modelDataV2SP）经 MetaProvider 进帧头——
// action_t 直通 + desire 电平→pulse 边沿（MODEL_ABI §4.2）
static void test_meta_provider_model_inputs() {
  MetaProvider mp;
  bgm1::FrameHeader h;
  std::memset(&h, 0x5a, sizeof h);

  // action_t 从 modeld 直通（chestnut 公式在 modeld 侧算，本模块不推导）
  const float at[2] = {0.125f, 0.45f};
  mp.set_model_inputs(at, 3);
  mp.fill(1, &h);
  CHECK(h.action_t[0] == 0.125f && h.action_t[1] == 0.45f);

  // 更新跟随
  const float at2[2] = {0.2f, 0.5f};
  mp.set_model_inputs(at2, 0);
  mp.fill(2, &h);
  CHECK(h.action_t[0] == 0.2f && h.action_t[1] == 0.5f);

  // desire 电平 → pulse 边沿（MODEL_ABI §4.2：通道 0 恒 0、持续同 desire 只 pulse 一次）
  MetaProvider mp2;
  mp2.set_model_inputs(at, 3);
  mp2.fill(10, &h);
  CHECK(h.desire[3] == 1.f);
  for (int i = 0; i < 8; i++) if (i != 3) CHECK(h.desire[i] == 0.f);

  mp2.fill(11, &h);                       // 同 desire 持续 → 不再 pulse
  for (int i = 0; i < 8; i++) CHECK(h.desire[i] == 0.f);

  mp2.set_model_inputs(at, 0);            // 回 0 → 无 pulse
  mp2.fill(12, &h);
  for (int i = 0; i < 8; i++) CHECK(h.desire[i] == 0.f);

  mp2.set_model_inputs(at, 3);            // 0→3 再现 → 再 pulse
  mp2.fill(13, &h);
  CHECK(h.desire[3] == 1.f);

  mp2.set_model_inputs(at, 5);            // 3→5 直接切换 → pulse 到新通道
  mp2.fill(14, &h);
  CHECK(h.desire[5] == 1.f && h.desire[3] == 0.f);

  mp2.set_model_inputs(at, 0);
  mp2.fill(15, &h);
  for (int i = 0; i < 8; i++) CHECK(h.desire[i] == 0.f);  // 通道 0 恒 0

  mp2.set_model_inputs(at, 9);            // 越界电平（msgq 垃圾）→ 不写任何通道
  mp2.fill(16, &h);
  for (int i = 0; i < 8; i++) CHECK(h.desire[i] == 0.f);

  // 新序列清 previousDesire（MODEL_ABI §4.3）：持续中的 desire 在新序列首帧重出 pulse
  MetaProvider mp3;
  mp3.set_model_inputs(at, 3);
  mp3.fill(20, &h);
  CHECK(h.desire[3] == 1.f);
  mp3.fill(21, &h);
  for (int i = 0; i < 8; i++) CHECK(h.desire[i] == 0.f);  // 持续同 desire 不再 pulse
  mp3.reset_desire_latch();                               // 双路 IDR/重连 → 新序列
  mp3.fill(22, &h);
  CHECK(h.desire[3] == 1.f);                              // 重出 pulse（d→0→d）
}

// ---- 配对状态机（16 号诊断修复：按 timestamp_sof 配对，不按 frame_id）----
using PM = bgm::PairMatcher<int>;

// 配对杀帧只记账：road SOF 进入时间槽编号，但不送入调度器开新序列。
static void test_pair_kill_does_not_recover() {
  FrameScheduler sched;
  sched.on_connect();
  FrameIndexer indexer;
  PM matcher;
  const uint64_t t0 = 4000000000ULL;

  matcher.push(true, {1, t0, 0});
  PM::Actions first = matcher.push(false, {5001, t0 + 70000, 1});
  CHECK(first.pair);
  SchedStep first_submit = sched.on_frame_submit(indexer.index(first.road.timestamp_sof));
  CHECK(first_submit.road_idr && first_submit.wide_idr);

  matcher.push(true, {2, t0 + 50000000ULL, 2});
  PM::Actions killed = matcher.push(true, {3, t0 + 100000000ULL, 3});
  CHECK(killed.kill_road && killed.road_dead.frame_id == 2);
  // 对照 main.cc::place_frame：只为统计/日志编号，不调用 sched.on_frame_dropped。
  CHECK(indexer.index(killed.road_dead.timestamp_sof) == FrameIdx{1});

  PM::Actions next = matcher.push(false, {5003, t0 + 100070000ULL, 4});
  CHECK(next.pair);
  const FrameIdx next_idx = indexer.index(next.road.timestamp_sof);
  CHECK(u32(next_idx) == 2);
  SchedStep next_submit = sched.on_frame_submit(FrameIdx{next_idx});
  CHECK(!next_submit.any());  // 配对杀帧没有插入新序列/request
}

// bug 最小复现（真机 2026-09-28：两路 frame_id 恒差 3372 且持续漂移）：
// frame_id 不等但 SOF 对齐 → 必须成对
static void test_pair_off_fid_aligned_sof() {
  PM m;
  uint64_t t = 1000000000ULL;
  for (int k = 0; k < 5; k++) {
    PM::Actions a1 = m.push(true, {100 + (uint32_t)k, t + 50000000ULL * k, k});
    CHECK(!a1.pair);
    PM::Actions a2 = m.push(false, {5000 + (uint32_t)k, t + 50000000ULL * k + 70000, k});
    CHECK(a2.pair);
    CHECK(a2.road.frame_id == 100 + k);
    CHECK(a2.wide.frame_id == 5000 + k);
    CHECK(!a2.kill_road && !a2.kill_wide);
  }
}

// 反例锁定：frame_id 相等不是配对键，SOF 错开 30 ms 不许成对（且旧帧超窗驱逐）
static void test_pair_same_fid_offset_sof() {
  PM m;
  PM::Actions a1 = m.push(true, {7, 1000000000ULL, 0});
  CHECK(!a1.pair);
  PM::Actions a2 = m.push(false, {7, 1000000000ULL + 30000000ULL, 1});
  CHECK(!a2.pair);
  CHECK(a2.kill_road && a2.road_dead.frame_id == 7);
  CHECK(!a2.kill_wide);
}

// 容差边界：10 ms 成对、11 ms 不成对
static void test_pair_tolerance_boundary() {
  PM m;
  m.push(true, {1, 5000000000ULL, 0});
  PM::Actions a = m.push(false, {2, 5000000000ULL + bgm::kPairToleranceNs, 1});
  CHECK(a.pair);

  PM m2;
  m2.push(true, {1, 5000000000ULL, 0});
  PM::Actions b = m2.push(false, {2, 5000000000ULL + bgm::kPairToleranceNs + 1, 1});
  CHECK(!b.pair);
  CHECK(b.kill_road && b.road_dead.frame_id == 1);  // road 超窗驱逐
}

// 置换杀旧（对路缺帧）+ wide 侧死亡与 road 侧死亡分开标记
static void test_pair_replace_and_evict() {
  PM m;
  uint64_t t = 2000000000ULL;
  m.push(true, {10, t, 0});
  PM::Actions a = m.push(true, {11, t + 50000000ULL, 1});  // road 连发，wide 缺帧
  CHECK(!a.pair);
  CHECK(a.kill_road && a.road_dead.frame_id == 10);
  CHECK(!a.kill_wide);

  PM::Actions b = m.push(true, {12, t + 100000000ULL, 2});
  CHECK(b.kill_road && b.road_dead.frame_id == 11);

  // wide 侧死亡只标 kill_wide（不上报调度器）
  PM m2;
  m2.push(false, {20, t, 10});
  PM::Actions c = m2.push(false, {21, t + 50000000ULL, 11});  // wide 连发 → 置换杀旧
  CHECK(c.kill_wide && c.wide_dead.frame_id == 20 && !c.kill_road);
  PM::Actions d = m2.push(true, {30, t + 100000000ULL, 12});  // road 晚到 → wide 21 超窗驱逐
  CHECK(d.kill_wide && d.wide_dead.frame_id == 21 && !d.kill_road && !d.pair);
}

// 同步组仿真（真机形态）：两路 SOF 同 50 ms 网格对齐（stagger 70 µs），各自跳过
// 不同槽位（road 每 37 槽缺 1、wide 每 12 槽缺 1）。断言：同槽成对恰好一次、
// 缺槽单侧死亡、每帧恰好退出一次、无错配。
static void test_pair_rate_mismatch_sim() {
  const int N = 120;
  const uint64_t t0 = 3000000000ULL, period = 50000000ULL;
  auto road_has = [](int k) { return k % 37 != 0; };
  auto wide_has = [](int k) { return k % 12 != 0; };

  PM m;
  std::vector<std::pair<uint32_t, uint32_t>> pairs;
  std::vector<uint32_t> dead;
  int expected_pairs = 0;

  for (int k = 0; k < N; k++) {
    if (road_has(k) && wide_has(k)) expected_pairs++;
    // 到达顺序 = SOF 顺序：road 先（stagger 早 70 µs）
    if (road_has(k)) {
      PM::Actions a = m.push(true, {(uint32_t)(100 + k), t0 + period * k, k});
      if (a.pair) pairs.push_back({a.road.frame_id, a.wide.frame_id});
      if (a.kill_road) dead.push_back(a.road_dead.frame_id);
      if (a.kill_wide) dead.push_back(a.wide_dead.frame_id);
    }
    if (wide_has(k)) {
      PM::Actions a = m.push(false, {(uint32_t)(5000 + k), t0 + period * k + 70000, 1000 + k});
      if (a.pair) pairs.push_back({a.road.frame_id, a.wide.frame_id});
      if (a.kill_road) dead.push_back(a.road_dead.frame_id);
      if (a.kill_wide) dead.push_back(a.wide_dead.frame_id);
    }
  }

  CHECK((int)pairs.size() == expected_pairs);  // 同槽全配上，无错配无漏配
  int total = 0;
  for (int k = 0; k < N; k++) total += (road_has(k) ? 1 : 0) + (wide_has(k) ? 1 : 0);
  int leftover = (m.has_pending(true) ? 1 : 0) + (m.has_pending(false) ? 1 : 0);
  CHECK(2 * (int)pairs.size() + (int)dead.size() + leftover == total);  // 每帧恰好退出一次

  std::vector<uint32_t> used;
  for (auto& p : pairs) {
    used.push_back(p.first);
    used.push_back(p.second);
  }
  used.insert(used.end(), dead.begin(), dead.end());
  std::sort(used.begin(), used.end());
  CHECK(std::unique(used.begin(), used.end()) == used.end());  // 无重复使用/重复杀死
}

// =====================================================================
// MetaCache（18 号 clean2 根因）：查不到不弹队、miss 分类
// =====================================================================

static OutMeta mk_om(uint32_t frame_id, uint32_t frame_idx) {
  OutMeta om;
  om.frame_id = CamFrameId{frame_id};
  om.frame_idx = FrameIdx{frame_idx};
  return om;
}

// clean2 回归（2026-09-28）：meta.clear()（重连）后旧连接输出迟到查不到键——
// 绝不许吃掉幸存的新条目（序列头双路 IDR 对曾因此整对消失）
static void test_meta_cache_clear_then_stale_take() {
  MetaCache m;
  m.push(mk_om(100, 0));  // 旧连接在途
  m.clear();              // kNewConnection：清空
  m.push(mk_om(200, 0));  // 新序列头对（seq0）
  m.push(mk_om(201, 1));

  MetaCache::Result r = m.take(CamFrameId{100});  // 旧输出迟到
  CHECK(r.st == MetaTake::kStaleMiss && r.dead.empty());

  r = m.take(CamFrameId{200});  // 头对必须还在
  CHECK(r.st == MetaTake::kHit && r.om.frame_id == CamFrameId{200} && r.om.frame_idx == FrameIdx{0});
  r = m.take(CamFrameId{201});
  CHECK(r.st == MetaTake::kHit && r.om.frame_id == CamFrameId{201});
}

// clear 后尚无新提交就来旧输出：kStaleMiss（不是断档）；比 front 旧同理
static void test_meta_cache_stale_classification() {
  MetaCache m;
  m.push(mk_om(50, 3));
  m.clear();
  CHECK(m.take(CamFrameId{50}).st == MetaTake::kStaleMiss);  // clear 后无新提交
  m.push(mk_om(60, 0));
  CHECK(m.take(CamFrameId{59}).st == MetaTake::kStaleMiss);  // 比 front 旧 = 已清/已弹
  CHECK(m.take(CamFrameId{60}).st == MetaTake::kHit);
  CHECK(m.take(CamFrameId{60}).st == MetaTake::kStaleMiss);  // 已弹的迟到输出
}

// 命中前缀死条目 = 输出被吞（断档），随命中返回；查无且非旧 = kGapMiss（断档）
static void test_meta_cache_dead_prefix_and_gap() {
  MetaCache m;
  m.push(mk_om(10, 0));
  m.push(mk_om(11, 1));
  m.push(mk_om(12, 2));
  MetaCache::Result r = m.take(CamFrameId{12});
  CHECK(r.st == MetaTake::kHit && r.om.frame_id == CamFrameId{12});
  CHECK(r.dead.size() == 2 && r.dead[0].frame_id == CamFrameId{10} && r.dead[1].frame_id == CamFrameId{11});

  // 死条目自己的输出迟到：已弹 = kStaleMiss（不重复上报）
  CHECK(m.take(CamFrameId{10}).st == MetaTake::kStaleMiss);

  // 队内比 front 新却查无 = 输出被吞 = 断档
  m.push(mk_om(20, 3));
  CHECK(m.take(CamFrameId{21}).st == MetaTake::kGapMiss);
  // 关键回归：断档 miss 不弹队，幸存条目还在
  r = m.take(CamFrameId{20});
  CHECK(r.st == MetaTake::kHit && r.om.frame_id == CamFrameId{20});
}

// =====================================================================
// 序列头门 + 连接代号（18 号 #1/#2）
// =====================================================================

// P 对不开流：整对丢弃（kHeadBarrier）直到「预测+实测双路 IDR」对上线
static void test_head_barrier_p_pair_dropped() {
  FakeSock sock;
  uint64_t now = 0;
  std::vector<Ev> evs;
  UplinkSender s(&sock, [&] { return now; }, UplinkSenderConfig{},
                 [&](const UplinkEventInfo& e) { evs.push_back({e.ev, u32(e.frame_idx), u32(e.conn_epoch)}); });
  s.step();
  evs.clear();

  const uint8_t p[2] = {1, 2};
  // P 对（帧 0）：预测非 IDR → 不开流，什么字节都不许出
  s.submit_road(ConnEpoch{1}, FrameIdx{0}, test_hdr(), p, sizeof p, false, false, false);
  s.submit_wide(ConnEpoch{1}, FrameIdx{0}, p, sizeof p, false);
  s.step();
  s.step();
  CHECK(sock.written.empty());
  CHECK(evs.size() == 1 && evs[0].ev == UplinkEvent::kHeadBarrier && evs[0].idx == 0);

  // 预测双 IDR 但实测 P（request 没落上）→ 同样丢弃（实测才是开流条件）
  s.submit_road(ConnEpoch{1}, FrameIdx{1}, test_hdr(), p, sizeof p, false, true, true);
  s.submit_wide(ConnEpoch{1}, FrameIdx{1}, p, sizeof p, false);
  s.step();
  CHECK(sock.written.empty());
  CHECK(evs.size() == 2 && evs[1].ev == UplinkEvent::kHeadBarrier && evs[1].idx == 1);

  // 合格双 IDR 对开流
  open_head(s, 1, 2, p, sizeof p, p, sizeof p);
  s.step();
  CHECK(evs.size() == 3 && evs[2].ev == UplinkEvent::kFrameSent && evs[2].idx == 2);
  CHECK(sock.written.size() == bgm1::frame_wire_size(sizeof p, sizeof p));
}

// 码流断档（notify_stream_gap）重新关门：断档后首帧必须双路 IDR（18 号 #1）
static void test_head_barrier_after_stream_gap() {
  FakeSock sock;
  uint64_t now = 0;
  std::vector<Ev> evs;
  UplinkSender s(&sock, [&] { return now; }, UplinkSenderConfig{},
                 [&](const UplinkEventInfo& e) { evs.push_back({e.ev, u32(e.frame_idx), u32(e.conn_epoch)}); });
  s.step();
  evs.clear();

  const uint8_t p[2] = {1, 2};
  open_head(s, 1, 0, p, sizeof p, p, sizeof p);  // 头对开流
  s.step();
  CHECK(evs.size() == 1 && evs[0].ev == UplinkEvent::kFrameSent);

  // 门开着：中流 P 对正常放行（road 出包即发）
  s.submit_road(ConnEpoch{1}, FrameIdx{1}, test_hdr(), p, sizeof p, false, false, false);
  s.submit_wide(ConnEpoch{1}, FrameIdx{1}, p, sizeof p, false);
  s.step();
  s.step();
  CHECK(evs.size() == 2 && evs[1].ev == UplinkEvent::kFrameSent && evs[1].idx == 1);
  const size_t before = sock.written.size();

  // 主线上报断档（编码输出缺帧/段长超限）→ 关门
  s.notify_stream_gap();
  // 断档后紧跟的 P 对（帧 2）不得成为首帧：整对丢弃
  s.submit_road(ConnEpoch{1}, FrameIdx{2}, test_hdr(), p, sizeof p, false, false, false);
  s.submit_wide(ConnEpoch{1}, FrameIdx{2}, p, sizeof p, false);
  s.step();
  s.step();
  CHECK(sock.written.size() == before);
  CHECK(evs.size() == 3 && evs[2].ev == UplinkEvent::kHeadBarrier && evs[2].idx == 2);

  // 双 IDR 对（帧 3）重新开流
  open_head(s, 1, 3, p, sizeof p, p, sizeof p);
  s.step();
  CHECK(evs.size() == 4 && evs[3].ev == UplinkEvent::kFrameSent && evs[3].idx == 3);
}

// 连接代号：重连窗口内旧连接的迟到提交静默拒绝，不污染新连接
static void test_sender_conn_epoch_rejects_stale() {
  FakeSock sock;
  uint64_t now = 0;
  std::vector<Ev> evs;
  UplinkSender s(&sock, [&] { return now; }, UplinkSenderConfig{},
                 [&](const UplinkEventInfo& e) { evs.push_back({e.ev, u32(e.frame_idx), u32(e.conn_epoch)}); });
  s.step();  // 连接代号 1
  CHECK(evs[0].ep == 1 && s.conn_epoch() == ConnEpoch{1});
  evs.clear();

  const uint8_t p[2] = {1, 2};
  open_head(s, 1, 0, p, sizeof p, p, sizeof p);
  s.step();

  s.notify_disconnect();  // 断连
  sock.mode = FakeSock::kWrite;
  CHECK(s.step());  // 重连 → 连接代号 2
  CHECK(evs.back().ev == UplinkEvent::kNewConnection && evs.back().ep == 2);
  const size_t before = sock.written.size();

  // 旧代号提交（重连窗口内旧连接的编码在途输出）→ 静默丢弃
  s.submit_road(ConnEpoch{1}, FrameIdx{5}, test_hdr(), p, sizeof p, true, true, true);
  s.submit_wide(ConnEpoch{1}, FrameIdx{5}, p, sizeof p, true);
  s.step();
  s.step();
  CHECK(sock.written.size() == before);  // 帧 5 不得进新连接
  for (auto& e : evs) CHECK(!(e.ev == UplinkEvent::kFrameSent && e.idx == 5));

  // 新代号头对正常开流（frame_idx 从 0 重新编号，无旧编号污染）
  open_head(s, 2, 0, p, sizeof p, p, sizeof p);
  s.step();
  CHECK(evs.back().ev == UplinkEvent::kFrameSent && evs.back().idx == 0);
}

// stall3 回归（2026-09-29）：已丢帧 tombstone 窗口不得跨连接——frame_idx 每连接重新
// 编号，旧连接的丢帧 id 会静默吃掉新连接的同 id 帧（GOP 相位偏移 + 解码断链，无上报）
static void test_sender_dropped_window_not_cross_connection() {
  FakeSock sock;
  uint64_t now = 0;
  std::vector<Ev> evs;
  UplinkSender s(&sock, [&] { return now; }, UplinkSenderConfig{},
                 [&](const UplinkEventInfo& e) { evs.push_back({e.ev, u32(e.frame_idx), u32(e.conn_epoch)}); });
  s.step();  // 连接代号 1
  evs.clear();

  const uint8_t p[2] = {1, 2};
  open_head(s, 1, 0, p, sizeof p, p, sizeof p);
  s.step();
  CHECK(evs.back().ev == UplinkEvent::kFrameSent && evs.back().idx == 0);
  evs.clear();

  // 旧连接制造 tombstone：帧 1 进排队槽被帧 2 覆盖（kDrop id 1），替代帧 2 是
  // P 对 → 序列头门丢弃（kHeadBarrier id 2）
  s.submit_road(ConnEpoch{1}, FrameIdx{1}, test_hdr(), p, sizeof p, false, false, false);
  s.submit_wide(ConnEpoch{1}, FrameIdx{1}, p, sizeof p, false);
  s.submit_road(ConnEpoch{1}, FrameIdx{2}, test_hdr(), p, sizeof p, false, false, false);
  s.submit_wide(ConnEpoch{1}, FrameIdx{2}, p, sizeof p, false);
  CHECK(evs.size() == 2 && evs[0].ev == UplinkEvent::kDrop && evs[0].idx == 1 &&
        evs[1].ev == UplinkEvent::kHeadBarrier && evs[1].idx == 2);

  s.notify_disconnect();
  CHECK(s.step());  // 重连 → 连接代号 2
  evs.clear();
  open_head(s, 2, 0, p, sizeof p, p, sizeof p);
  s.step();
  CHECK(evs.back().ev == UplinkEvent::kFrameSent && evs.back().idx == 0);
  evs.clear();

  // 新连接 frame_idx 重新编号：id 1/2 是正常新帧，不得被旧 tombstone 静默吃掉
  s.submit_road(ConnEpoch{2}, FrameIdx{1}, test_hdr(), p, sizeof p, false, false, false);
  s.submit_wide(ConnEpoch{2}, FrameIdx{1}, p, sizeof p, false);
  s.step();
  s.step();
  s.submit_road(ConnEpoch{2}, FrameIdx{2}, test_hdr(), p, sizeof p, false, false, false);
  s.submit_wide(ConnEpoch{2}, FrameIdx{2}, p, sizeof p, false);
  s.step();
  s.step();
  bool sent1 = false, sent2 = false;
  for (auto& e : evs) {
    if (e.ev == UplinkEvent::kFrameSent && e.idx == 1) sent1 = true;
    if (e.ev == UplinkEvent::kFrameSent && e.idx == 2) sent2 = true;
  }
  CHECK(sent1 && sent2);
}

// =====================================================================
// FrameStageLog：C4 本机分段（road 关键路径，各段首尾相接）
// =====================================================================
static void test_frame_stages_telescope() {
  FrameStageLog log;
  const uint64_t ms = 1'000'000;
  const FrameIdx k{7};
  StageMs s;
  CHECK(!log.on_reply(k, 99 * ms, &s));  // 没提交过：不报
  log.on_submit(k, 100 * ms, 120 * ms, 121 * ms, 125 * ms);  // eof / recv / warp 完 / 配对提交
  log.on_encoded(k, 128 * ms);
  log.on_encoded(k, 127 * ms);  // 两路取晚的
  CHECK(!log.on_reply(k, 160 * ms, &s));  // 还没写完：不报
  log.on_sent(k, 130 * ms);
  CHECK(log.on_reply(k, 160 * ms, &s));
  CHECK_NEAR(s.capture, 20, 1e-6);
  CHECK_NEAR(s.warp, 1, 1e-6);
  CHECK_NEAR(s.pair_wait, 4, 1e-6);
  CHECK_NEAR(s.encode, 3, 1e-6);
  CHECK_NEAR(s.send, 2, 1e-6);
  CHECK_NEAR(s.reply_wait, 30, 1e-6);
  CHECK_NEAR(s.capture + s.warp + s.pair_wait + s.encode + s.send + s.reply_wait, 60, 1e-6);
  CHECK(!log.on_reply(k, 161 * ms, &s));  // 每帧只报一次
}

static void test_frame_stages_ring_no_mismatch() {
  FrameStageLog log;
  StageMs s;
  log.on_submit(FrameIdx{1}, 1, 2, 3, 4);
  log.on_encoded(FrameIdx{1}, 5);
  log.on_sent(FrameIdx{1}, 6);
  log.on_submit(FrameIdx{1 + FrameStageLog::kSlots}, 10, 20, 30, 40);  // 同槽新帧顶掉旧帧
  CHECK(!log.on_reply(FrameIdx{1}, 7, &s));                           // 旧帧 REPLY 不许配到新帧
  log.on_encoded(FrameIdx{1}, 8);                                     // 旧帧迟到事件不污染新帧
  log.on_sent(FrameIdx{1}, 9);
  CHECK(!log.on_reply(FrameIdx{1 + FrameStageLog::kSlots}, 50, &s));  // 新帧还没编码/写完
}

int main() {
  test_frame_stages_telescope();
  test_frame_stages_ring_no_mismatch();
  test_sched_gop();
  test_sched_drop();
  test_sched_drop_dedup();
  test_sched_stream_gap_no_dedup();
  test_sched_submit_time_slot_hole();
  test_frame_indexer();

  test_sender_road_out_goes();
  test_sender_flags();
  test_sender_queue_overwrite();
  test_sender_stall_finish();
  test_sender_stall_truncate();
  test_sender_reconnect_linklost();
  test_sender_submit_while_down();
  test_sender_partial_writes();
  test_sender_write_error();
  test_sender_wait_for_work_wakes_on_submit();
  test_link_state();
  test_avahi_parse();
  test_avahi_scope();
  test_avahi_locator();

  test_frame_codec_roundtrip();
  test_reply_err_roundtrip();

  test_warp_golden();
  test_meta_provider();
  test_meta_provider_model_inputs();

  test_pair_kill_does_not_recover();
  test_pair_off_fid_aligned_sof();
  test_pair_same_fid_offset_sof();
  test_pair_tolerance_boundary();
  test_pair_replace_and_evict();
  test_pair_rate_mismatch_sim();

  test_meta_cache_clear_then_stale_take();
  test_meta_cache_stale_classification();
  test_meta_cache_dead_prefix_and_gap();
  test_head_barrier_p_pair_dropped();
  test_head_barrier_after_stream_gap();
  test_sender_conn_epoch_rejects_stale();
  test_sender_dropped_window_not_cross_connection();

  printf("%d checks, %d fails\n", g_checks, g_fails);
  return g_fails == 0 ? 0 : 1;
}
