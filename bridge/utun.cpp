//
//  utun.cpp
//  bridge
//
//  Created by 冀宸 on 2022/2/1.
//

#include <stdexcept>
#include <string>
#include <net/if_utun.h>
#include <sys/ioctl.h>
#include <sys/kern_control.h>
#include <sys/socket.h>
#include <sys/sys_domain.h>
#include "scoped_fd.hpp"

// Open specific utun device unit and return fd.
// If the unit number is already in use, return -1.
// Throw exceptions for all other errors.
// Return the iface name in name.
static int utun_open(std::string& name, int unit) {
  struct sockaddr_ctl sc;
  struct ctl_info ctlInfo;

  memset(&ctlInfo, 0, sizeof(ctlInfo));
  if (strlcpy(ctlInfo.ctl_name, UTUN_CONTROL_NAME, sizeof(ctlInfo.ctl_name))
      >= sizeof(ctlInfo.ctl_name)) {
    throw std::runtime_error("UTUN_CONTROL_NAME too long");
  }

  bridge::ScopedFD fd(socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL));
  if (!fd.defined()) {
    throw std::runtime_error("socket(SYSPROTO_CONTROL) error");
  }

  if (ioctl(fd(), CTLIOCGINFO, &ctlInfo) == -1) {
    throw std::runtime_error("ioctl(CTLIOCGINFO) error");
  }

  sc.sc_id = ctlInfo.ctl_id;
  sc.sc_len = sizeof(sc);
  sc.sc_family = AF_SYSTEM;
  sc.ss_sysaddr = AF_SYS_CONTROL;
  sc.sc_unit = unit + 1;
  std::memset(sc.sc_reserved, 0, sizeof(sc.sc_reserved));

  // If the connect is successful, a utunX device will be created, where X
  // is our unit number - 1.
  if (connect(fd(), (struct sockaddr*) &sc, sizeof(sc)) == -1) {
    return -1;
  }

  // Get iface name of newly created utun dev.
  char utunname[20];
  socklen_t utunname_len = sizeof(utunname);
  if (getsockopt(fd(), SYSPROTO_CONTROL, UTUN_OPT_IFNAME,
                 utunname, &utunname_len)) {
    throw std::runtime_error("getsockopt(UTUN_OPT_IFNAME) error");
  }

  name = utunname;

  return fd.release();
}

// Try to open an available utun device unit.
// Return the iface name in name.
int utun_open(std::string& name) {
  for (int unit = 0; unit < 256; ++unit) {
    int fd = utun_open(name, unit);
    if (fd >= 0) {
      return fd;
    }
  }
  throw std::runtime_error("cannot open available utun device");
}
