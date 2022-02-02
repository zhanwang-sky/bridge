//
//  crypto.hpp
//  bridge
//
//  Created by 冀宸 on 2022/2/3.
//

#ifndef crypto_hpp
#define crypto_hpp

#include <cstddef> // std::size_t
#include <cstdint> // uintx_t

namespace bridge {

constexpr std::size_t crypto_header_len = 24;

class CryptoBase {
 public:
  explicit CryptoBase(uint32_t client_id, uint8_t* buf, std::size_t len);
  virtual ~CryptoBase();

 protected:
  uint32_t client_id_;
  uint8_t* buf_;
  std::size_t len_;
};

class Encryptor : public CryptoBase {
 public:
  explicit Encryptor(uint32_t client_id, uint8_t* buf, std::size_t len);
  virtual ~Encryptor();

  bool encrypt(uint64_t gen_id, uint64_t pkt_seq,
               std::size_t& data_offst, std::size_t& data_len);
};

class Decryptor : public CryptoBase {
 public:
  explicit Decryptor(uint32_t client_id, uint8_t* buf, std::size_t len);
  virtual ~Decryptor();

  bool decrypt(uint64_t& gen_id, uint64_t& pkt_seq,
               std::size_t& data_offst, std::size_t& data_len);
};

}

#endif /* crypto_hpp */
