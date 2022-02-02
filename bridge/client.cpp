//
//  client.cpp
//  bridge
//
//  Created by 冀宸 on 2022/2/1.
//

#include <chrono>
#include <functional>
#include <thread>
#include <glog/logging.h>
#include "crypto.hpp"
#include "client.hpp"

#if defined(__APPLE__)
extern int utun_open(std::string& name);
#define opentun(ifname) utun_open(ifname)
#else
#error unknown platform
#endif

#define TIMESTAMP_US() \
std::chrono::duration_cast<std::chrono::microseconds> \
(std::chrono::high_resolution_clock::now().time_since_epoch()).count()

using namespace bridge;

Client::Client(boost::asio::io_context& io, const std::string& ip,
               const std::string& port, uint32_t client_id)
    : io_(io),
      ifname_(),
      fd_(io, opentun(ifname_)),
      socket_(io),
      client_id_(client_id),
      gen_id_(TIMESTAMP_US()) {
  fd_.non_blocking(true);
  boost::asio::ip::udp::resolver resolver(io_);
  auto ep = *resolver.resolve(ip.c_str(), port.c_str()).begin();
  socket_.connect(ep.endpoint());
  socket_.non_blocking(true);

  LOG(INFO) << ifname_ << " is opened, fd=" << fd_.native_handle();
#if defined(__APPLE__)
  LOG(INFO) << "hint:$ sudo ifconfig " << ifname_ << " inet 10.0.0.1/24 10.0.0.254 mtu 1448 up";
#endif
}

Client::~Client() {
  socket_.close();
  fd_.close();
}

void Client::start() {
  start_reading();
  start_receiving();
}

void Client::start_reading() {
  buf_ptr pbuf = std::make_shared<buf_type>();
  fd_.async_read_some(boost::asio::buffer(pbuf->data() + crypto_header_len,
                                          pbuf->size() - crypto_header_len),
                      std::bind(&Client::read_handler, this, pbuf,
                                std::placeholders::_1, std::placeholders::_2));
}

void Client::start_receiving() {
  buf_ptr pbuf = std::make_shared<buf_type>();
  socket_.async_receive(boost::asio::buffer(*pbuf),
                        std::bind(&Client::receive_handler, this, pbuf,
                                  std::placeholders::_1, std::placeholders::_2));
}

void Client::read_handler(buf_ptr pbuf, const boost::system::error_code& ec,
                          std::size_t nbytes) {
  if (ec) {
    if (ec == boost::system::errc::operation_canceled) {
      return;
    }
    LOG(ERROR) << "client read error: " << ec.message() << " (" << ec << ")";
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  start_reading();

  if (!ec) {
    std::size_t data_offst = crypto_header_len;
    std::size_t data_len = nbytes;
#if defined(__APPLE__)
    if (data_len < 24
        || pbuf->at(data_offst + 0) != 0 || pbuf->at(data_offst + 1) != 0
        || pbuf->at(data_offst + 2) != 0 || pbuf->at(data_offst + 3) != 2) {
      // Family != IP
      return;
    }
    data_offst += 4;
    data_len -= 4;
#else
    if (data_len < 20 || (pbuf->at(data_offst) >> 4) != 4) {
      // not an IPv4 packet
      return;
    }
#endif

    Encryptor encryptor(client_id_, pbuf->data(), pbuf->size());
    if (!encryptor.encrypt(gen_id_, ++tx_cnt_, data_offst, data_len)) {
      --tx_cnt_;
      return;
    }

    socket_.async_send(boost::asio::buffer(pbuf->data() + data_offst, data_len),
                       [this, pbuf](const boost::system::error_code&,
                                    std::size_t){});
  }
}

void Client::receive_handler(buf_ptr pbuf, const boost::system::error_code& ec,
                             std::size_t nbytes) {
  if (ec) {
    if (ec == boost::system::errc::operation_canceled) {
      return;
    }
    LOG(ERROR) << "client receive error: " << ec.message() << " (" << ec << ")";
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

    if (gen_id != gen_id_) {
      return;
    }

    ++rx_cnt_;

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
