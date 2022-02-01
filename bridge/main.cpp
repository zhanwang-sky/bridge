//
//  main.cpp
//  bridge
//
//  Created by 冀宸 on 2022/2/1.
//

#include <exception>
#include <iomanip>
#include <iostream>
#include <string>
#include "common/scoped_fd.hpp"

#if defined(__APPLE__)
extern int utun_open(std::string& name);
#define opentun(ifname) utun_open(ifname)
#else
#error unknown platform
#endif

using std::cout;
using std::cerr;
using std::endl;

uint8_t buf[4096];

int main(int argc, char* argv[]) {
  try {
    std::string ifname;
    bridge::ScopedFD fd(opentun(ifname));

    cout << ifname << " is opened, fd=" << fd() << endl;
#if defined(__APPLE__)
    cout << "hint:$ sudo ifconfig " << ifname << " inet 10.0.0.1/24 10.0.0.254 up\n";
#endif

    std::size_t sz;
    cout << std::hex;
    while ((sz = read(fd(), buf, sizeof(buf))) > 0) {
      for (std::size_t i = 0; i < sz; ++i) {
        if (i && (i % 16 == 0)) {
          cout << endl;
        }
        cout << std::setw(2) << std::setfill('0') << (unsigned) buf[i] << ' ';
      }
      cout << endl;
    }
  } catch (std::exception& e) {
    cerr << e.what() << endl;
  }

  return 0;
}
