// 服务端定位实现（06 号）。纯逻辑部分（parse_ipv4/in_scope/parse_avahi_browse_line/
// AvahiLocator 节流）宿主单测覆盖；getifaddrs/popen 是薄胶水，设备验证。

#include "server_locator.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>

bool parse_ipv4(const std::string& s, uint32_t* out) {
  in_addr a{};
  if (inet_pton(AF_INET, s.c_str(), &a) != 1) return false;
  if (out) *out = ntohl(a.s_addr);
  return true;
}

bool in_scope(const ServerEndpoint& ep, const SubnetScope& scope) {
  if (ep.iface != scope.iface) return false;
  uint32_t ip = 0;
  if (!parse_ipv4(ep.host, &ip)) return false;
  return (ip & scope.mask) == (scope.local & scope.mask);
}

// 按分隔符取前 n 个字段（TXT 值里可能有 ';'，只取到第 9 个字段就停）
static std::vector<std::string> split_n(const std::string& s, char sep, int n) {
  std::vector<std::string> out;
  size_t pos = 0;
  while (int(out.size()) < n) {
    size_t q = s.find(sep, pos);
    if (q == std::string::npos) {
      out.push_back(s.substr(pos));
      break;
    }
    out.push_back(s.substr(pos, q - pos));
    pos = q + 1;
  }
  return out;
}

bool parse_avahi_browse_line(const std::string& line, ServerEndpoint* out) {
  // =;iface;IPv4;name;type;domain;hostname;ip;port;txt...
  if (line.size() < 3 || line[0] != '=') return false;
  const std::vector<std::string> f = split_n(line, ';', 9);
  if (f.size() < 9) return false;
  if (f[2] != "IPv4") return false;
  if (f[4] != "_bigmodel._tcp") return false;
  if (f[7].empty() || f[3].empty()) return false;
  char* end = nullptr;
  unsigned long port = strtoul(f[8].c_str(), &end, 10);
  if (end == f[8].c_str() || *end != '\0' || port == 0 || port > 65535) return false;

  ServerEndpoint ep;
  ep.iface = f[1];
  ep.name = f[3];
  ep.host = f[7];
  ep.port = uint16_t(port);
  if (out) *out = ep;
  return true;
}

bool AvahiLocator::resolve(ServerEndpoint* out) {
  if (have_cache_ && failures_ < rediscover_after_) {
    *out = cache_;
    return true;
  }
  ServerEndpoint ep;
  if (!browse_(&ep)) {
    have_cache_ = false;  // 没发现到服务：不缓存，下次 resolve 继续试
    return false;
  }
  cache_ = ep;
  have_cache_ = true;
  failures_ = 0;
  *out = ep;
  return true;
}

void AvahiLocator::on_connect_failure() {
  if (failures_ < rediscover_after_) failures_++;
}

bool detect_wifi_scope(SubnetScope* out) {
  // C4 的 Wi-Fi 接口 = wlan0（spec「部署条件」：iwpriv wlan0 setPower 2）
  ifaddrs* ifa = nullptr;
  if (getifaddrs(&ifa) != 0) return false;
  bool ok = false;
  for (ifaddrs* p = ifa; p != nullptr; p = p->ifa_next) {
    if (p->ifa_addr == nullptr || p->ifa_netmask == nullptr) continue;
    if (p->ifa_addr->sa_family != AF_INET || p->ifa_netmask->sa_family != AF_INET) continue;
    if (strcmp(p->ifa_name, "wlan0") != 0) continue;
    if (!(p->ifa_flags & IFF_UP)) continue;
    SubnetScope s;
    s.iface = p->ifa_name;
    s.local = ntohl(((sockaddr_in*)p->ifa_addr)->sin_addr.s_addr);
    s.mask = ntohl(((sockaddr_in*)p->ifa_netmask)->sin_addr.s_addr);
    if (out) *out = s;
    ok = true;
    break;
  }
  freeifaddrs(ifa);
  return ok;
}

bool browse_avahi(ServerEndpoint* out, const SubnetScope& scope) {
  if (scope.iface.empty()) return false;  // Wi-Fi 没起：发现范围为空
  // 不带 -t（-t 会在解析完前退出）；stdbuf -oL 保行缓冲，timeout 截杀不丢输出
  std::string cmd = "timeout 3 stdbuf -oL avahi-browse -rp _bigmodel._tcp 2>/dev/null";
  FILE* p = popen(cmd.c_str(), "r");
  if (!p) return false;

  bool ok = false;
  ServerEndpoint ep;
  char buf[4096];
  while (fgets(buf, sizeof buf, p) != nullptr) {
    std::string line(buf);
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();
    ServerEndpoint cand;
    if (parse_avahi_browse_line(line, &cand) && in_scope(cand, scope)) {
      ep = cand;
      ok = true;
      break;  // 取第一条范围内记录（多台手机并存时 v1 不挑，手动 IP 兜底）
    }
  }
  pclose(p);
  if (ok && out) *out = ep;
  return ok;
}
