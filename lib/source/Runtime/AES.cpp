#include <memory>
#include <vector>
#include <stdint.h>

#include <openssl/aes.h>
#include <openssl/aes.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/evperr.h>

#define DECL_OPENSSL_PTR(tname, free_func) \
  struct openssl_##tname##_dtor {          \
    void operator()(tname* v) {            \
        free_func(v);                      \
    }                                      \
  };                                       \
  typedef std::unique_ptr<tname, openssl_##tname##_dtor> tname##_t


DECL_OPENSSL_PTR(EVP_CIPHER_CTX, ::EVP_CIPHER_CTX_free);

class AES256 
{
  std::vector<uint8_t> IV;

public:
  explicit AES256()
    : IV(16)
  {
    for (int i = 0; i < IV.size(); ++i)
      IV[i] = uint8_t(i * i);
  }

  bool Encrypt(
    const std::vector<uint8_t>& key
    , const std::vector<uint8_t>& message
    , std::vector<uint8_t>& output
  ) const 
  {
    output.resize(message.size() * AES_BLOCK_SIZE);

    int inlen = (int)message.size();
    int outlen = 0;
    size_t total_out = 0;

    EVP_CIPHER_CTX_t ctx(EVP_CIPHER_CTX_new());
    if (ERR_get_error() != 0)
      return false;

    const std::vector<uint8_t> enc_key = key;

    int res;
    res = EVP_EncryptInit(ctx.get(), EVP_aes_256_cbc(), enc_key.data(), IV.data());
    if (ERR_get_error() != 0)
      return false;

    res = EVP_EncryptUpdate(ctx.get(), output.data(), &outlen, message.data(), inlen);
    if (ERR_get_error() != 0)
      return false;

    total_out += outlen;
    res = EVP_EncryptFinal(ctx.get(), output.data() + total_out, &outlen);
    if (ERR_get_error() != 0)
      return false;

    total_out += outlen;
    output.resize(total_out);
    
    return true;
  }

  bool Decrypt(
    const std::vector<uint8_t>& key
    , const std::vector<uint8_t>& message
    , std::vector<uint8_t>& output
  ) const
  {
    output.resize(message.size() * 3);
  
    int outlen = 0;
    size_t total_out = 0;

    EVP_CIPHER_CTX_t ctx(EVP_CIPHER_CTX_new());
    if (ERR_get_error() != 0)
      return false;

    const std::vector<uint8_t> enc_key = key;
    std::vector<uint8_t> target_message;
    std::vector<uint8_t> iv;

    iv = IV;
    target_message = message;

    int inlen = (int)target_message.size();

    int res;
    res = EVP_DecryptInit(ctx.get(), EVP_aes_256_cbc(), enc_key.data(), iv.data());
    if (ERR_get_error() != 0)
      return false;

    res = EVP_DecryptUpdate(ctx.get(), output.data(), &outlen, target_message.data(), inlen);
    if (ERR_get_error() != 0)
      return false;

    total_out += outlen;
    res = EVP_DecryptFinal(ctx.get(), output.data() + outlen, &outlen);
    if (ERR_get_error() != 0)
      return false;

    total_out += outlen;
    output.resize(total_out);

    return true;
  }
};

bool AESEncrypt(
  const std::vector<uint8_t>& key
  , const std::vector<uint8_t>& message
  , std::vector<uint8_t>& output
)
{
  AES256 aes;
  return aes.Encrypt(key, message, output);
}

bool AESDecrypt(
  const std::vector<uint8_t>& key
  , const std::vector<uint8_t>& message
  , std::vector<uint8_t>& output
)
{
  AES256 aes;
  return aes.Decrypt(key, message, output);
}
