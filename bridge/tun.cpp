//
//  utun.cpp
//  bridge
//
//  Created by 冀宸 on 2022/2/4.
//

#if defined(__linux__)

#include <cstring>
#include <stdexcept>
#include <string>
#include <fcntl.h>
#include <linux/if_tun.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include "scoped_fd.hpp"

static void tun_open(const std::string& name, struct ifreq& ifr, int fd) {
  if (!name.empty()) {
    for (int unit = 0; unit < 256; ++unit) {
      std::string ifname = name;
      if (unit) {
        ifname += std::to_string(unit);
      }
      if (ifname.length() >= IFNAMSIZ) {
        throw std::runtime_error("ifname too long");
      }
      strcpy(ifr.ifr_name, ifname.c_str());
      if (ioctl(fd, TUNSETIFF, (void*) &ifr) == 0) {
        return;
      }
      throw std::runtime_error("cannot open available tun device");
    }
  } else {
    if (ioctl(fd, TUNSETIFF, (void*) &ifr) < 0) {
      throw std::runtime_error("cannot open available tun device");
    }
  }
}

int tun_open(std::string& name) {
  static const char node[] = "/dev/net/tun";

  bridge::ScopedFD fd(open(node, O_RDWR));
  if (!fd.defined()) {
    throw std::runtime_error("open tun dev error");
  }

  struct ifreq ifr;
  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_ONE_QUEUE;
  ifr.ifr_flags |= IFF_NO_PI;
  ifr.ifr_flags |= IFF_TUN;
  tun_open(name, ifr, fd());

  name = ifr.ifr_name;

  return fd.release();
}

#endif /* defined(__linux__) */
