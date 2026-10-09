#pragma once

// 服务端定位（06 号）：C4 怎么找到手机上的推理服务器。
//   手动 IP（FixedLocator）：Params "BigmodelServerHost" 填了 IP → 恒连该地址，只跳过发现；
//   自动（AvahiLocator）：mDNS 发现 _bigmodel._tcp（设备自带 avahi-daemon/avahi-browse）。
//
// 发现范围（用户拍板）：只在 C4 当前连接的 Wi-Fi 子网内发现——记录的接口必须是
// wlan0（C4 的 Wi-Fi 接口，见 spec「部署条件」），且解析出的 IP 落在 wlan0 的
// IPv4 子网内；其他接口、别的局域网、avahi 陈旧缓存记录都不收。
//
// 发现节流：平时用缓存地址直连（网络闪断毫秒级重连），连败 rediscover_after 次
//（默认 3，与 UplinkSender 的 kLinkLost 升级点同刻）重新发现，覆盖手机 IP 变化；
// 没发现到服务时没有缓存，每次 resolve 都重试（持续重试）。
//
// 发现过程可注入（BrowseFn），宿主测试用假发现；真发现 = browse_avahi()。
// 纯逻辑（行解析/子网判定）宿主单测见 test_bigmodeld.cc。

#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

struct ServerEndpoint {
  std::string host;   // IPv4 点分（发现拿到的是 IP）
  uint16_t port = 7070;
  std::string name;   // mDNS 实例名（App 安装时 UUID；诊断用）
  std::string iface;  // 发现到的接口（C4 Wi-Fi = wlan0）
};

// 发现范围：当前 Wi-Fi 接口及其 IPv4 子网（detect_wifi_scope 填，测试手填）
struct SubnetScope {
  std::string iface;
  uint32_t local = 0;  // 本机 IPv4（parse_ipv4 同序：网络字节序 u32）
  uint32_t mask = 0;
};

// 点分四段 → u32；坏格式返回 false
bool parse_ipv4(const std::string& s, uint32_t* out);

// 接口一致且 IP 落在子网内才算发现范围内
bool in_scope(const ServerEndpoint& ep, const SubnetScope& scope);

// 解析 avahi-browse -rp 一行。只认 `=`（已解析）IPv4 _bigmodel._tcp 记录：
//   =;iface;IPv4;name;_bigmodel._tcp;domain;hostname;ip;port;txt...
// 其余行（+/-/IPv6/别的服务/坏行）返回 false。name 带八进制转义（\032 等），原样存。
bool parse_avahi_browse_line(const std::string& line, ServerEndpoint* out);

class ServerLocator {
 public:
  virtual ~ServerLocator() = default;
  // 取连接目标；每次建连都调。false = 现在找不到（调用方按连接失败处理）。
  virtual bool resolve(ServerEndpoint* out) = 0;
  // 一次连接失败（建连超时/被拒/EOF）；发现节流用。
  virtual void on_connect_failure() {}
};

// 手动 IP：恒给定端点（06 号「只跳过发现，其余照常」）。
class FixedLocator : public ServerLocator {
 public:
  explicit FixedLocator(ServerEndpoint ep) : ep_(std::move(ep)) {}
  bool resolve(ServerEndpoint* out) override { *out = ep_; return true; }

 private:
  ServerEndpoint ep_;
};

// 自动发现：browse() 为发现接缝（真 = browse_avahi 带范围；测试注入假）。
class AvahiLocator : public ServerLocator {
 public:
  using BrowseFn = std::function<bool(ServerEndpoint*)>;
  AvahiLocator(BrowseFn browse, int rediscover_after = 3)
      : browse_(std::move(browse)), rediscover_after_(rediscover_after) {}
  bool resolve(ServerEndpoint* out) override;
  void on_connect_failure() override;

 private:
  BrowseFn browse_;
  int rediscover_after_;
  int failures_ = 0;  // 自上次成功发现起的连败数
  bool have_cache_ = false;
  ServerEndpoint cache_;
};

// 当前 Wi-Fi 范围探测（getifaddrs）：取 wlan0 的 IPv4 地址/掩码。没有（Wi-Fi 没起）
// 返回 false → 发现不到任何服务（发现范围为空）。
bool detect_wifi_scope(SubnetScope* out);

// 真发现：`timeout 3 stdbuf -oL avahi-browse -rp _bigmodel._tcp`（不带 -t：-t 会在解析完前
// 退出；stdbuf 保行缓冲，SIGTERM 截杀不丢输出），取第一条范围内的 IPv4 记录。
bool browse_avahi(ServerEndpoint* out, const SubnetScope& scope);
