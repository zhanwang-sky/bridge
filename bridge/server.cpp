//
//  server.cpp
//  bridge
//
//  Created by 冀宸 on 2022/2/3.
//

#include <chrono>
#include <functional>
#include <thread>
#include <glog/logging.h>
#include "crypto.hpp"
#include "server.hpp"

#if defined(__APPLE__)
extern int utun_open(std::string& name);
#define opentun(ifname) utun_open(ifname)
#elif defined(__linux__)
extern int tun_open(std::string& name);
#define opentun(ifname) tun_open(ifname)
#else
#error unknown platform
#endif

using namespace bridge;

Server::Server(boost::asio::io_context& io, const std::string& ip,
               const std::string& port, uint32_t client_id)
    : io_(io),
      ifname_(),
      fd_(io, opentun(ifname_)),
      socket_(io),
      timer_(io),
      client_id_(client_id) {
  fd_.non_blocking(true);
  boost::asio::ip::udp::resolver resolver(io_);
  auto ep = *resolver.resolve(ip.c_str(), port.c_str()).begin();
  socket_.open(ep.endpoint().protocol());
  socket_.bind(ep.endpoint());
  socket_.non_blocking(true);

  LOG(INFO) << ifname_ << " is opened, fd=" << fd_.native_handle();
#if defined(__APPLE__)
  LOG(INFO) << "hint:$ sudo ifconfig " << ifname_ << " inet 10.0.0.254/24 10.0.0.1 mtu 1448 up";
#elif defined(__linux__)
  LOG(INFO) << "hint:$ sudo ip a add dev " << ifname_ << " 10.0.0.254/24";
  LOG(INFO) << "hint:$ sudo ip l set dev " << ifname_ << " mtu 1448 up";
  LOG(INFO) << "hint:$ sudo iptables -t nat -A POSTROUTING -s 10.0.0.0/24 -o <NIC> -j MASQUERADE";
#endif
}

Server::~Server() {
  timer_.cancel();
  socket_.close();
  fd_.close();
}

void Server::start() {
  start_reading();
  start_receiving();
  start_timing();
}

void Server::start_reading() {
  buf_ptr pbuf = std::make_shared<buf_type>();
  fd_.async_read_some(boost::asio::buffer(pbuf->data() + crypto_header_len,
                                          pbuf->size() - crypto_header_len),
                      std::bind(&Server::read_handler, this, pbuf,
                                std::placeholders::_1, std::placeholders::_2));
}

void Server::start_receiving() {
  buf_ptr pbuf = std::make_shared<buf_type>();
  addr_ptr paddr = std::make_shared<addr_type>();
  socket_.async_receive_from(boost::asio::buffer(*pbuf), *paddr,
                             std::bind(&Server::receive_handler, this, pbuf, paddr,
                                       std::placeholders::_1, std::placeholders::_2));
}

void Server::start_timing() {
  timer_.expires_at(timer_.expiry() + std::chrono::seconds(60));
  timer_.async_wait(std::bind(&Server::timeout_handler, this,
                              std::placeholders::_1));
}

void Server::read_handler(buf_ptr pbuf, const boost::system::error_code& ec,
                          std::size_t nbytes) {
  if (ec) {
    if (ec == boost::system::errc::operation_canceled) {
      return;
    }
    LOG(WARNING) << "server read error: " << ec.message() << " (" << ec << ")";
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  start_reading();

  if (!ec && active_) {
    std::size_t data_offst = crypto_header_len;
    std::size_t data_len = nbytes;
#if defined(__APPLE__)
    if (pbuf->at(data_offst + 0) != 0 || pbuf->at(data_offst + 1) != 0
        || pbuf->at(data_offst + 2) != 0 || pbuf->at(data_offst + 3) != 2) {
      // Family != IP
      return;
    }
    data_offst += 4;
    data_len -= 4;
#endif

    Encryptor encryptor(client_id_, pbuf->data(), pbuf->size());
    if (!encryptor.encrypt(gen_id_, ++tx_cnt_, data_offst, data_len)) {
      --tx_cnt_;
      return;
    }

    ++timed_tx_cnt_;

    socket_.async_send_to(boost::asio::buffer(pbuf->data() + data_offst, data_len),
                          client_addr_,
                          [this, pbuf](const boost::system::error_code&,
                                       std::size_t){});
  }
}

void Server::receive_handler(buf_ptr pbuf, addr_ptr paddr,
                             const boost::system::error_code& ec,
                             std::size_t nbytes) {
  if (ec) {
    if (ec == boost::system::errc::operation_canceled) {
      return;
    }
    LOG(WARNING) << "server receive error: " << ec.message() << " (" << ec << ")";
  }

  start_receiving();

  if (!ec) {
    std::size_t data_offst = 0;
    std::size_t data_len = nbytes;
    uint64_t gen_id = 0;
    uint64_t pkt_seq = 0;

    Decryptor decryptor(client_id_, pbuf->data() + data_offst, data_len);
    if (!decryptor.decrypt(gen_id, pkt_seq, data_offst, data_len)) {
      return;
    }

    if (gen_id < gen_id_) {
      return;
    } else if (gen_id == gen_id_) {
      if (pkt_seq <= rx_seq_) {
        if (*paddr != client_addr_) {
          return;
        }
      } else {
        client_addr_ = *paddr;
        rx_seq_ = pkt_seq;
      }
    } else {
      client_addr_ = *paddr;
      gen_id_ = gen_id;
      rx_seq_ = pkt_seq;
    }

    ++rx_cnt_;
    ++timed_rx_cnt_;
    zero_rx_times_ = 0;
    active_ = true;

#if defined(__APPLE__)
    data_offst -= 4;
    data_len += 4;
    pbuf->at(data_offst + 0) = 0;
    pbuf->at(data_offst + 1) = 0;
    pbuf->at(data_offst + 2) = 0;
    pbuf->at(data_offst + 3) = 2;
#endif

    boost::asio::async_write(fd_,
                             boost::asio::buffer(pbuf->data() + data_offst, data_len),
                             [this, pbuf](const boost::system::error_code&,
                                          std::size_t){});
  }
}

void Server::timeout_handler(const boost::system::error_code& ec) {
  if (ec) {
    if (ec == boost::system::errc::operation_canceled) {
      return;
    }
    LOG(WARNING) << "server timer error: " << ec.message() << " (" << ec << ")";
  }

  start_timing();

  if (!ec) {
    if (!timed_rx_cnt_) {
      if (++zero_rx_times_ >= 5) {
        gen_id_ = 0;
        rx_seq_ = 0;
        active_ = false;
      }
    }
    timed_rx_cnt_ = 0;
    timed_tx_cnt_ = 0;
  }
}
