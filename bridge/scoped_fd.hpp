//
//  scoped_fd.hpp
//  bridge
//
//  Created by 冀宸 on 2022/2/1.
//

#ifndef scoped_fd_hpp
#define scoped_fd_hpp

#include <errno.h>
#include <unistd.h>

namespace bridge {

class ScopedFD {
 public:
  ScopedFD() : fd_(-1) { }

  explicit ScopedFD(int fd) : fd_(fd) { }

  ScopedFD(ScopedFD&& other) noexcept {
    fd_ = other.fd_;
    other.fd_ = -1;
  }

  ScopedFD& operator=(ScopedFD&& other) {
    close();
    fd_ = other.fd_;
    other.fd_ = -1;
    return *this;
  }

  virtual ~ScopedFD() { close(); }

  virtual void post_close(int/*eno*/) { }

  bool defined() const { return fd_ >= 0; }

  int operator()() const { return fd_; }

  int release() {
    int ret = fd_;
    fd_ = -1;
    return ret;
  }

  int close() {
    int eno = 0;
    if (defined()) {
      if (::close(fd_) == -1) {
        eno = errno;
      }
      post_close(eno);
      fd_ = -1;
    }
    return eno;
  }

  void reset(int fd) {
    close();
    fd_ = fd;
  }

 private:
  int fd_;

  ScopedFD(const ScopedFD&) = delete;
  ScopedFD& operator=(const ScopedFD&) = delete;
};

}

#endif /* scoped_fd_hpp */
