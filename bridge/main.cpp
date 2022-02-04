//
//  main.cpp
//  bridge
//
//  Created by 冀宸 on 2022/2/1.
//

#include <cstdlib>
#include <exception>
#include <string>
#include <boost/asio.hpp>
#include <glog/logging.h>
#include "client.hpp"
#include "server.hpp"

void client_start(boost::asio::io_context& io, const std::string& ip,
                  const std::string& port, uint32_t client_id) {
  static bridge::Client c(io, ip, port, client_id);
  c.start();
}

void server_start(boost::asio::io_context& io, const std::string& ip,
                  const std::string& port, uint32_t client_id) {
  static bridge::Server s(io, ip, port, client_id);
  s.start();
}

int main(int argc, char* argv[]) {
  if ((argc != 4 && argc != 5) || (argc == 5 && strcmp(argv[1], "-s"))) {
    LOG(ERROR) << "Usage: ./bridge [-s] ip port client_id";
    exit(EXIT_FAILURE);
  }

  bool server = (argc == 5) ? true : false;
  const char *ip = argv[argc - 3];
  const char *port = argv[argc - 2];
  uint32_t client_id = (uint32_t) atol(argv[argc - 1]);
  if (client_id == 0 || client_id == UINT32_MAX) {
    LOG(ERROR) << "invalid client_id";
    exit(EXIT_FAILURE);
  }

  try {
    boost::asio::io_context io;
    if (server) {
      server_start(io, ip, port, client_id);
    } else {
      client_start(io, ip, port, client_id);
    }
    io.run();
  } catch (std::exception& e) {
    LOG(ERROR) << e.what();
    exit(EXIT_FAILURE);
  }

  return 0;
}
