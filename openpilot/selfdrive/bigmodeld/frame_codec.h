// BGM1 线协议 FRAME 消息（HEVC 版）编解码 —— C4 上行进程与 App 服务端共用同一实现。
// 布局定稿（09 号 BGM1 + 21 号 HEVC 修订）如下，本注释即权威；原 spec 已归档 git 历史：
//
//   FRAME（type 0x10）线上布局，全小端：
//     0   16  MsgHdr：magic "BGM1"、ver=2、type=0x10、flags（bit0=road IDR、bit1=wide IDR）、
//                frame_idx u32、len u32 = road 段字节数
//     16   8  t_eof u64（timestamp_eof，ns）
//     24  32  desire f32[8]（本帧 pulse，全零=无事件）
//     56   8  traffic_convention f32[2]
//     64   8  action_t f32[2]（[lat, long] 秒）
//     72  36  warp_road f32[9]（3×3 透视矩阵，行主序，与 lean modeld 送进 frame_prepare 的同义；
//                v2 起为 C4 已应用的矩阵，仅遥测/dump，App 不再 warp）
//    108  36  warp_wide f32[9]
//    144  16  保留（置 0，解析端忽略）
//    160  ..  road 段（road_len = MsgHdr.len，HEVC Annex-B 码流；v2 = warp 后 512×256 NV12 编码）
//     ..   4  wide_len u32
//     ..  ..  wide 段（HEVC Annex-B 码流）
//     ..  16  MAC 位（HMAC-SHA256 截断 16 B；保留位，恒填零不校验；HMAC 已随 ADR-0003 撤销）
//
//   线长 = 160 + road_len + 4 + wide_len + 16。
//
//   REPLY（type 0x11，09 号 §5）线上布局，全小端：
//     0   16  MsgHdr：magic "BGM1"、ver=2、type=0x11、flags（bit0=SEQ_RESET、bit1=ZERO_PAIR）、
//                frame_idx u32（回显）、len u32 = 8280（payload 字节数，不含 t_eof）
//     16   8  t_eof u64（回显）
//     24 8264  outputs[0:2066) f32
//   8288  16  遥测 u32×4：srv_recv_us、srv_prep_us、srv_htp_us、srv_total_us
//   8304  16  MAC 位（保留位，恒填零不校验；HMAC 已随 ADR-0003 撤销）
//   线长 = 24 + 8280 + 16 = 8320。
//
//   ERR（type 0x7F，09 号 §2）：MsgHdr（len=8）‖ code u32 ‖ detail u32 ‖ MAC（零），线长 40。
//   code 数值（09 号只列名字，数值以此为准）：1 VERSION_MISMATCH、2 NOT_PAIRED、
//   3 PAIR_MISMATCH、4 BAD_FRAME、5 INTERNAL。
//
// 无 Android 依赖，可原样带去 sunnypilot 仓库；宿主单测见 android/tests/test_frame_codec.cpp，
// Python 镜像实现见 android/tools/bgm1_frame.py，两端对同一 golden 字节样例互证。
#pragma once

#include <cstddef>
#include <cstdint>

namespace bgm1 {

constexpr uint32_t kMagic = 0x314D4742u;  // "BGM1" 小端
constexpr uint8_t kVersion = 2;  // v2：段内容 = C4 已 warp 的 512×256 HEVC（v1 = 相机全幅）
constexpr uint8_t kTypeFrame = 0x10;
constexpr uint8_t kTypeReply = 0x11;
constexpr uint8_t kTypeErr = 0x7F;
constexpr uint8_t kTypeHello = 0x01;

constexpr size_t kMsgHdrSize = 16;
constexpr size_t kFrameHdrSize = 160;  // MsgHdr 16 + 扩展 144
constexpr size_t kWideLenSize = 4;
constexpr size_t kMacSize = 16;
constexpr size_t kFrameMinSize = kFrameHdrSize + kWideLenSize + kMacSize;

constexpr uint16_t kFlagRoadIdr = 1u << 0;
constexpr uint16_t kFlagWideIdr = 1u << 1;

// 小端读 u32（streaming 消费方读 wide_len 等散布字段用）
inline uint32_t read_u32_le(const uint8_t* p) {
  return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}

// 每段码流上限（HELLO.max_frame 同义），超过即长度错。10 Mb/s/路实测 I 帧峰值 ~225 KB。
constexpr uint32_t kDefaultMaxSegment = 1u << 20;

enum class Err {
  kOk = 0,
  kBadMagic,
  kBadVersion,
  kBadType,
  kTruncated,     // 字节数不足以容纳头/载荷
  kLenTooLarge,   // road/wide 段超上限
  kLenMismatch,   // 线长与 road_len/wide_len 声明不符
};

// FRAME 头（160 B）+ wide 长度。wide_len 不在 160 B 头内，序列化在 road 段之后。
struct FrameHeader {
  uint16_t flags = 0;
  uint32_t frame_idx = 0;
  uint32_t road_len = 0;
  uint32_t wide_len = 0;
  uint64_t t_eof = 0;
  float desire[8] = {0};
  float traffic_convention[2] = {0};
  float action_t[2] = {0};
  float warp_road[9] = {0};
  float warp_wide[9] = {0};

  bool road_idr() const { return flags & kFlagRoadIdr; }
  bool wide_idr() const { return flags & kFlagWideIdr; }
};

// 整条 FRAME 的拆帧视图：road/wide 指针指向被解析缓冲内部（不拷贝）。
struct FrameView {
  FrameHeader hdr;
  const uint8_t* road = nullptr;
  const uint8_t* wide = nullptr;
};

// 组帧头（160 B，小端）
void pack_frame_header(const FrameHeader& hdr, uint8_t out[kFrameHdrSize]);

// 解析 MsgHdr（16 B），校验 magic/ver/type
Err parse_msg_hdr(const uint8_t in[kMsgHdrSize], uint8_t* type, uint16_t* flags,
                  uint32_t* frame_idx, uint32_t* len);

// 解析 FRAME 头（160 B，含 MsgHdr；不校验载荷长度）
Err parse_frame_header(const uint8_t in[kFrameHdrSize], FrameHeader* out);

// FRAME 线长
inline size_t frame_wire_size(uint32_t road_len, uint32_t wide_len) {
  return kFrameHdrSize + road_len + kWideLenSize + wide_len + kMacSize;
}

// 组完整 FRAME：hdr ‖ road ‖ wide_len ‖ wide ‖ MAC（零）。返回写入字节数，
// cap 不足返回 0（写入不超过 cap 的部分）。hdr 的 road_len/wide_len 与实参缓冲必须一致。
size_t pack_frame(const FrameHeader& hdr, const uint8_t* road, const uint8_t* wide,
                  uint8_t* out, size_t cap);

// 解析完整 FRAME（含长度校验；MAC 位本阶段不校验）。size 不含在缓冲之外的任何字节。
Err parse_frame(const uint8_t* data, size_t size, FrameView* out,
                uint32_t max_segment = kDefaultMaxSegment);

// ---- REPLY（type 0x11，09 号 §5）----

constexpr uint16_t kFlagSeqReset = 1u << 0;  // REPLY flags bit0：本帧服务端新建序列
constexpr uint16_t kFlagZeroPair = 1u << 1;  // REPLY flags bit1：t-4 用了零图

constexpr size_t kReplyHdrSize = 24;  // MsgHdr 16 + t_eof 回显 8
constexpr size_t kReplyOutputsCount = 2066;
constexpr size_t kReplyOutputsSize = kReplyOutputsCount * 4;
constexpr size_t kReplyTelemetryCount = 4;  // recv/prep/htp/total us
constexpr size_t kReplyPayloadSize = kReplyOutputsSize + kReplyTelemetryCount * 4;  // 8280
constexpr size_t kReplyWireSize = kReplyHdrSize + kReplyPayloadSize + kMacSize;      // 8320

// REPLY 整条消息。outputs/telemetry 解析时拷贝（小端转主机序），组帧时原样写回。
struct Reply {
  uint16_t flags = 0;
  uint32_t frame_idx = 0;  // 回显
  uint64_t t_eof = 0;      // 回显
  float outputs[kReplyOutputsCount] = {};                     // outputs[0:2066)
  uint32_t telemetry[kReplyTelemetryCount] = {};              // 遥测 u32×4

  bool seq_reset() const { return flags & kFlagSeqReset; }
  bool zero_pair() const { return flags & kFlagZeroPair; }
};

// 组完整 REPLY：头 24 B ‖ outputs ‖ 遥测 ‖ MAC（零）。返回写入字节数，cap 不足返回 0。
size_t pack_reply(const Reply& r, uint8_t* out, size_t cap);

// 解析完整 REPLY（含长度校验；MAC 位本阶段不校验）。
Err parse_reply(const uint8_t* data, size_t size, Reply* out);

// ---- ERR（type 0x7F，09 号 §2）----

// ERR code 数值定义（09 号只列名字，数值以本表为准，与 bgm1_frame.py 的 ERR_CODES 一致）
constexpr uint32_t kErrVersionMismatch = 1;
constexpr uint32_t kErrNotPaired = 2;
constexpr uint32_t kErrPairMismatch = 3;
constexpr uint32_t kErrBadFrame = 4;
constexpr uint32_t kErrInternal = 5;

constexpr size_t kErrPayloadSize = 8;  // code u32 + detail u32
constexpr size_t kErrWireSize = kMsgHdrSize + kErrPayloadSize + kMacSize;  // 40

struct ErrMsg {
  uint32_t code = kErrBadFrame;
  uint32_t detail = 0;
  uint32_t frame_idx = 0;  // MsgHdr.frame_idx
};

// 组完整 ERR：MsgHdr（len=8）‖ code u32 ‖ detail u32 ‖ MAC（零）。返回写入字节数，cap 不足返回 0。
size_t pack_err(const ErrMsg& e, uint8_t* out, size_t cap);

// 解析完整 ERR（含长度校验；MAC 位本阶段不校验）。
Err parse_err(const uint8_t* data, size_t size, ErrMsg* out);

// ---- HELLO（type 0x01，06 号）：连接建立后服务端首条消息（无鉴权，ADR-0003）----
// 线上布局：MsgHdr（type=0x01，frame_idx=0，len=16）‖ instance_id u64 ‖ max_frame u32 ‖
// 保留 u32（置 0）‖ MAC（零），线长 48。instance_id 每次服务端启动随机，C4 用来区分
// 服务端重启与网络闪断；max_frame = 每段码流上限（kDefaultMaxSegment 同义）。

constexpr size_t kHelloPayloadSize = 16;
constexpr size_t kHelloWireSize = kMsgHdrSize + kHelloPayloadSize + kMacSize;  // 48

struct Hello {
  uint64_t instance_id = 0;
  uint32_t max_frame = kDefaultMaxSegment;
};

// 组完整 HELLO：MsgHdr ‖ payload ‖ MAC（零）。返回写入字节数，cap 不足返回 0。
size_t pack_hello(const Hello& h, uint8_t* out, size_t cap);

// 解析完整 HELLO（含长度校验；保留 u32 解析端忽略）。
Err parse_hello(const uint8_t* data, size_t size, Hello* out);

}  // namespace bgm1
