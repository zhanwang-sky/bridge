//
//  server.hpp
//  bridge
//
//  Created by 冀宸 on 2022/2/3.
//

#ifndef server_hpp
#define server_hpp

#include <array>
#include <memory>
#include <string>
#include <boost/asio.hpp>

namespace bridge {

class Server {
 public:
  explicit Server(boost::asio::io_context& io, const std::string& ip,
                  const std::string& port, uint32_t client_id);
  virtual ~Server();

  void start();

 private:
  using buf_type = std::array<uint8_t, 4096>;
  using buf_ptr = std::shared_ptr<buf_type>;
  using addr_type = boost::asio::ip::udp::endpoint;
  using addr_ptr = std::shared_ptr<addr_type>;

  void start_reading();
  void start_receiving();
  void start_timing();
  void read_handler(buf_ptr pbuf, const boost::system::error_code& ec,
                    std::size_t nbytes);
  void receive_handler(buf_ptr pbuf, addr_ptr paddr,
                       const boost::system::error_code& ec, std::size_t nbytes);
  void timeout_handler(const boost::system::error_code& ec);

  boost::asio::io_context& io_;
  std::string ifname_;
  boost::asio::posix::stream_descriptor fd_;
  boost::asio::ip::udp::socket socket_;
  boost::asio::steady_timer timer_;
  uint32_t client_id_;

  addr_type client_addr_;
  uint64_t gen_id_ = 0;
  uint64_t rx_seq_ = 0;

  uint64_t rx_cnt_ = 0;
  uint64_t tx_cnt_ = 0;
  uint64_t timed_rx_cnt_ = 0;
  uint64_t timed_tx_cnt_ = 0;
  uint64_t zero_rx_times_ = 0;
  bool active_ = false;

  Server(const Server&) = delete;
  Server& operator=(const Server&) = delete;
};

}

#endif /* server_hpp */
