//
//  crypto.cpp
//  bridge
//
//  Created by 冀宸 on 2022/2/3.
//

#include <random>
#include "crypto.hpp"

using namespace bridge;

static void pack32(uint8_t* buf, uint32_t val) {
  buf[0] = (uint8_t) ((val >> 24) & 0xff);
  buf[1] = (uint8_t) ((val >> 16) & 0xff);
  buf[2] = (uint8_t) ((val >> 8) & 0xff);
  buf[3] = (uint8_t) (val & 0xff);
}

static void pack64(uint8_t* buf, uint64_t val) {
  buf[0] = (uint8_t) ((val >> 56) & 0xff);
  buf[1] = (uint8_t) ((val >> 48) & 0xff);
  buf[2] = (uint8_t) ((val >> 40) & 0xff);
  buf[3] = (uint8_t) ((val >> 32) & 0xff);
  buf[4] = (uint8_t) ((val >> 24) & 0xff);
  buf[5] = (uint8_t) ((val >> 16) & 0xff);
  buf[6] = (uint8_t) ((val >> 8) & 0xff);
  buf[7] = (uint8_t) (val & 0xff);
}

static uint32_t unpack32(const uint8_t* buf) {
  uint32_t val = 0;

  val = (uint32_t) buf[0] << 24;
  val |= (uint32_t) buf[1] << 16;
  val |= (uint32_t) buf[2] << 8;
  val |= (uint32_t) buf[3];

  return val;
}

static uint64_t unpack64(const uint8_t* buf) {
  uint64_t val = 0;

  val = (uint64_t) buf[0] << 56;
  val |= (uint64_t) buf[1] << 48;
  val |= (uint64_t) buf[2] << 40;
  val |= (uint64_t) buf[3] << 32;
  val |= (uint64_t) buf[4] << 24;
  val |= (uint64_t) buf[5] << 16;
  val |= (uint64_t) buf[6] << 8;
  val |= (uint64_t) buf[7];

  return val;
}

CryptoBase::CryptoBase(uint32_t client_id, uint8_t* buf, std::size_t len)
    : client_id_(client_id), buf_(buf), len_(len) { }

CryptoBase::~CryptoBase() { }

Encryptor::Encryptor(uint32_t client_id, uint8_t* buf, std::size_t len)
    : CryptoBase(client_id, buf, len) { }

Encryptor::~Encryptor() { }

bool Encryptor::encrypt(uint64_t gen_id, uint64_t pkt_seq,
                        std::size_t& data_offst, std::size_t& data_len) {
  if (!buf_ || len_ < (crypto_header_len + data_len)) {
    return false;
  }

  if (data_offst < crypto_header_len) {
    return false;
  }

  uint8_t* p = buf_ + data_offst - crypto_header_len;

  std::random_device rd;
  std::minstd_rand0 generator(rd());
  std::uniform_int_distribution<uint32_t> distribution;
  uint32_t rand = distribution(generator);
  pack32(p, rand);
  p += 4;

  uint32_t client_id = client_id_ ^ rand;
  pack32(p, client_id);
  p += 4;

  uint64_t x1 = (uint64_t) rand << 32;
  x1 |= (uint64_t) client_id_;
  x1 ^= 0x73614bcf0a49671d;
  gen_id ^= x1;
  pack64(p, gen_id);
  p += 8;

  uint64_t x2 = (uint64_t) client_id_ << 32;
  x2 |= (uint64_t) rand;
  x2 ^= 0x96834d5e32017c65;
  pkt_seq ^= x2;
  pack64(p, pkt_seq);
  p += 8;

  uint8_t x = (uint8_t) ((rand >> 9) ^ 0x13);
  for (std::size_t i = 0; i < data_len; ++i) {
    p[i] ^= x;
    if ((i % 4) == 0) {
      x += 19;
    }
  }

  data_offst -= crypto_header_len;
  data_len += crypto_header_len;

  return true;
}

Decryptor::Decryptor(uint32_t client_id, uint8_t* buf, std::size_t len)
    : CryptoBase(client_id, buf, len) { }

Decryptor::~Decryptor() { }

bool Decryptor::decrypt(uint64_t& gen_id, uint64_t& pkt_seq,
                        std::size_t& data_offst, std::size_t& data_len) {
  if (!buf_ || len_ < crypto_header_len) {
    return false;
  }

  uint8_t* p = buf_;

  uint32_t rand = unpack32(p);
  p += 4;

  uint32_t client_id = unpack32(p);
  p += 4;

  if ((client_id ^ rand) != client_id_) {
    return false;
  }

  uint64_t x1 = (uint64_t) rand << 32;
  x1 |= (uint64_t) client_id_;
  x1 ^= 0x73614bcf0a49671d;
  gen_id = unpack64(p) ^ x1;
  p += 8;

  uint64_t x2 = (uint64_t) client_id_ << 32;
  x2 |= (uint64_t) rand;
  x2 ^= 0x96834d5e32017c65;
  pkt_seq = unpack64(p) ^ x2;
  p += 8;

  data_offst = crypto_header_len;
  data_len = len_ - crypto_header_len;

  uint8_t x = (uint8_t) ((rand >> 9) ^ 0x13);
  for (std::size_t i = 0; i < data_len; ++i) {
    p[i] ^= x;
    if ((i % 4) == 0) {
      x += 19;
    }
  }

  return true;
}
