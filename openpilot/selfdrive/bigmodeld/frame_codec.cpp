#include "frame_codec.h"

#include <cstring>

namespace bgm1 {
namespace {

inline void put_u16(uint8_t* p, uint16_t v) {
  p[0] = uint8_t(v);
  p[1] = uint8_t(v >> 8);
}
inline void put_u32(uint8_t* p, uint32_t v) {
  p[0] = uint8_t(v);
  p[1] = uint8_t(v >> 8);
  p[2] = uint8_t(v >> 16);
  p[3] = uint8_t(v >> 24);
}
inline void put_u64(uint8_t* p, uint64_t v) {
  put_u32(p, uint32_t(v));
  put_u32(p + 4, uint32_t(v >> 32));
}
inline uint16_t get_u16(const uint8_t* p) {
  return uint16_t(p[0]) | (uint16_t(p[1]) << 8);
}
inline uint32_t get_u32(const uint8_t* p) {
  return read_u32_le(p);  // 实现收在 frame_codec.h（bgm1_server 等 streaming 消费方共用）
}
inline uint64_t get_u64(const uint8_t* p) {
  return uint64_t(get_u32(p)) | (uint64_t(get_u32(p + 4)) << 32);
}

inline void put_f32(uint8_t* p, float v) {
  uint32_t bits;
  std::memcpy(&bits, &v, 4);
  put_u32(p, bits);
}
inline float get_f32(const uint8_t* p) {
  uint32_t bits = get_u32(p);
  float v;
  std::memcpy(&v, &bits, 4);
  return v;
}

// 通用 MsgHdr 校验（magic/ver/type），REPLY/ERR 解析共用
Err parse_msg_hdr_typed(const uint8_t in[kMsgHdrSize], uint8_t expect_type,
                        uint16_t* flags, uint32_t* frame_idx, uint32_t* len) {
  if (get_u32(in) != kMagic) return Err::kBadMagic;
  if (in[4] != kVersion) return Err::kBadVersion;
  if (in[5] != expect_type) return Err::kBadType;
  if (flags) *flags = get_u16(in + 6);
  if (frame_idx) *frame_idx = get_u32(in + 8);
  if (len) *len = get_u32(in + 12);
  return Err::kOk;
}

void put_msg_hdr(uint8_t out[kMsgHdrSize], uint8_t type, uint16_t flags,
                 uint32_t frame_idx, uint32_t len) {
  std::memset(out, 0, kMsgHdrSize);
  put_u32(out + 0, kMagic);
  out[4] = kVersion;
  out[5] = type;
  put_u16(out + 6, flags);
  put_u32(out + 8, frame_idx);
  put_u32(out + 12, len);
}

}  // namespace

void pack_frame_header(const FrameHeader& hdr, uint8_t out[kFrameHdrSize]) {
  std::memset(out, 0, kFrameHdrSize);
  put_msg_hdr(out, kTypeFrame, hdr.flags, hdr.frame_idx, hdr.road_len);
  put_u64(out + 16, hdr.t_eof);
  for (int i = 0; i < 8; i++) put_f32(out + 24 + 4 * i, hdr.desire[i]);
  for (int i = 0; i < 2; i++) put_f32(out + 56 + 4 * i, hdr.traffic_convention[i]);
  for (int i = 0; i < 2; i++) put_f32(out + 64 + 4 * i, hdr.action_t[i]);
  for (int i = 0; i < 9; i++) put_f32(out + 72 + 4 * i, hdr.warp_road[i]);
  for (int i = 0; i < 9; i++) put_f32(out + 108 + 4 * i, hdr.warp_wide[i]);
  // 144..159 保留，置 0
}

Err parse_msg_hdr(const uint8_t in[kMsgHdrSize], uint8_t* type, uint16_t* flags,
                  uint32_t* frame_idx, uint32_t* len) {
  Err err = parse_msg_hdr_typed(in, kTypeFrame, flags, frame_idx, len);
  if (type) *type = in[5];
  return err;
}

Err parse_frame_header(const uint8_t in[kFrameHdrSize], FrameHeader* out) {
  uint8_t type;
  uint16_t flags;
  uint32_t frame_idx, road_len;
  Err err = parse_msg_hdr(in, &type, &flags, &frame_idx, &road_len);
  if (err != Err::kOk) return err;

  FrameHeader h;
  h.flags = flags;
  h.frame_idx = frame_idx;
  h.road_len = road_len;
  h.t_eof = get_u64(in + 16);
  for (int i = 0; i < 8; i++) h.desire[i] = get_f32(in + 24 + 4 * i);
  for (int i = 0; i < 2; i++) h.traffic_convention[i] = get_f32(in + 56 + 4 * i);
  for (int i = 0; i < 2; i++) h.action_t[i] = get_f32(in + 64 + 4 * i);
  for (int i = 0; i < 9; i++) h.warp_road[i] = get_f32(in + 72 + 4 * i);
  for (int i = 0; i < 9; i++) h.warp_wide[i] = get_f32(in + 108 + 4 * i);
  if (out) *out = h;
  return Err::kOk;
}

size_t pack_frame(const FrameHeader& hdr, const uint8_t* road, const uint8_t* wide,
                  uint8_t* out, size_t cap) {
  const size_t total = frame_wire_size(hdr.road_len, hdr.wide_len);
  if (cap < total) return 0;
  pack_frame_header(hdr, out);
  std::memcpy(out + kFrameHdrSize, road, hdr.road_len);
  put_u32(out + kFrameHdrSize + hdr.road_len, hdr.wide_len);
  std::memcpy(out + kFrameHdrSize + hdr.road_len + kWideLenSize, wide, hdr.wide_len);
  std::memset(out + kFrameHdrSize + hdr.road_len + kWideLenSize + hdr.wide_len, 0, kMacSize);
  return total;
}

Err parse_frame(const uint8_t* data, size_t size, FrameView* out,
                uint32_t max_segment) {
  if (size < kFrameMinSize) return Err::kTruncated;

  FrameHeader h;
  Err err = parse_frame_header(data, &h);
  if (err != Err::kOk) return err;

  // 载荷长度：先按声明校验上限（坏 road_len 不应误报截断），再校验实际线长
  if (h.road_len > max_segment) return Err::kLenTooLarge;
  if (size < kFrameHdrSize + h.road_len + kWideLenSize) return Err::kTruncated;
  const uint32_t wide_len = get_u32(data + kFrameHdrSize + h.road_len);
  if (wide_len > max_segment) return Err::kLenTooLarge;
  h.wide_len = wide_len;
  if (size != frame_wire_size(h.road_len, wide_len)) return Err::kLenMismatch;

  if (out) {
    out->hdr = h;
    out->road = data + kFrameHdrSize;
    out->wide = data + kFrameHdrSize + h.road_len + kWideLenSize;
  }
  return Err::kOk;
}

size_t pack_reply(const Reply& r, uint8_t* out, size_t cap) {
  if (cap < kReplyWireSize) return 0;
  std::memset(out, 0, kReplyWireSize);
  put_msg_hdr(out, kTypeReply, r.flags, r.frame_idx, uint32_t(kReplyPayloadSize));
  put_u64(out + 16, r.t_eof);
  for (size_t i = 0; i < kReplyOutputsCount; i++) put_f32(out + kReplyHdrSize + 4 * i, r.outputs[i]);
  for (size_t i = 0; i < kReplyTelemetryCount; i++)
    put_u32(out + kReplyHdrSize + kReplyOutputsSize + 4 * i, r.telemetry[i]);
  // 尾部 16 B MAC 置零
  return kReplyWireSize;
}

Err parse_reply(const uint8_t* data, size_t size, Reply* out) {
  if (size < kReplyWireSize) return Err::kTruncated;
  uint16_t flags;
  uint32_t frame_idx, len;
  Err err = parse_msg_hdr_typed(data, kTypeReply, &flags, &frame_idx, &len);
  if (err != Err::kOk) return err;
  if (len != kReplyPayloadSize || size != kReplyWireSize) return Err::kLenMismatch;

  Reply r;
  r.flags = flags;
  r.frame_idx = frame_idx;
  r.t_eof = get_u64(data + 16);
  for (size_t i = 0; i < kReplyOutputsCount; i++) r.outputs[i] = get_f32(data + kReplyHdrSize + 4 * i);
  for (size_t i = 0; i < kReplyTelemetryCount; i++)
    r.telemetry[i] = get_u32(data + kReplyHdrSize + kReplyOutputsSize + 4 * i);
  if (out) *out = r;
  return Err::kOk;
}

size_t pack_err(const ErrMsg& e, uint8_t* out, size_t cap) {
  if (cap < kErrWireSize) return 0;
  std::memset(out, 0, kErrWireSize);
  put_msg_hdr(out, kTypeErr, 0, e.frame_idx, uint32_t(kErrPayloadSize));
  put_u32(out + kMsgHdrSize, e.code);
  put_u32(out + kMsgHdrSize + 4, e.detail);
  // 尾部 16 B MAC 置零
  return kErrWireSize;
}

Err parse_err(const uint8_t* data, size_t size, ErrMsg* out) {
  if (size < kErrWireSize) return Err::kTruncated;
  uint16_t flags;
  uint32_t frame_idx, len;
  Err err = parse_msg_hdr_typed(data, kTypeErr, &flags, &frame_idx, &len);
  if (err != Err::kOk) return err;
  if (len != kErrPayloadSize || size != kErrWireSize) return Err::kLenMismatch;

  ErrMsg e;
  e.code = get_u32(data + kMsgHdrSize);
  e.detail = get_u32(data + kMsgHdrSize + 4);
  e.frame_idx = frame_idx;
  if (out) *out = e;
  return Err::kOk;
}

size_t pack_hello(const Hello& h, uint8_t* out, size_t cap) {
  if (cap < kHelloWireSize) return 0;
  std::memset(out, 0, kHelloWireSize);
  put_msg_hdr(out, kTypeHello, 0, 0, uint32_t(kHelloPayloadSize));
  put_u64(out + kMsgHdrSize, h.instance_id);
  put_u32(out + kMsgHdrSize + 8, h.max_frame);
  // 保留 u32 与尾部 16 B MAC 置零
  return kHelloWireSize;
}

Err parse_hello(const uint8_t* data, size_t size, Hello* out) {
  if (size < kHelloWireSize) return Err::kTruncated;
  uint16_t flags;
  uint32_t frame_idx, len;
  Err err = parse_msg_hdr_typed(data, kTypeHello, &flags, &frame_idx, &len);
  if (err != Err::kOk) return err;
  if (len != kHelloPayloadSize || size != kHelloWireSize) return Err::kLenMismatch;

  Hello h;
  h.instance_id = get_u64(data + kMsgHdrSize);
  h.max_frame = get_u32(data + kMsgHdrSize + 8);
  if (out) *out = h;
  return Err::kOk;
}

}  // namespace bgm1
