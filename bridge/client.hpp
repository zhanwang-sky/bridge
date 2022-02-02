//
//  client.hpp
//  bridge
//
//  Created by 冀宸 on 2022/2/1.
//

#ifndef client_hpp
#define client_hpp

#include <array>
#include <memory>
#include <string>
#include <boost/asio.hpp>

namespace bridge {

class Client {
 public:
  explicit Client(boost::asio::io_context& io, const std::string& ip,
                  const std::string& port, uint32_t client_id);
  virtual ~Client();

  void start();

 private:
  using buf_type = std::array<uint8_t, 4096>;
  using buf_ptr = std::shared_ptr<buf_type>;

  void start_reading();
  void start_receiving();
  void read_handler(buf_ptr pbuf, const boost::system::error_code& ec,
                    std::size_t nbytes);
  void receive_handler(buf_ptr pbuf, const boost::system::error_code& ec,
                       std::size_t nbytes);

  boost::asio::io_context& io_;
  std::string ifname_;
  boost::asio::posix::stream_descriptor fd_;
  boost::asio::ip::udp::socket socket_;
  uint32_t client_id_;
  uint64_t gen_id_;
  uint64_t tx_cnt_ = 0;
  uint64_t rx_cnt_ = 0;

  Client(const Client&) = delete;
  Client& operator=(const Client&) = delete;
};

}

#endif /* client_hpp */
