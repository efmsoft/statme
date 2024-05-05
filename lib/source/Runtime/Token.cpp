#include <algorithm>

#include <openssl/rand.h>

#include <Statme/http/Base64.hpp>
#include <Statme/Runtime/AES.h>
#include <Statme/Runtime/Broker.h>

#if defined(_WIN32) || defined(_WIN64)
/* We are on Windows */
# define strtok_r strtok_s
#else
#include <string.h>
#endif 

using namespace Runtime;
using namespace Syncme;

void Broker::GenerateKey()
{
  RAND_bytes(&Key[0], (int)Key.size());
}

bool Broker::VerifyToken(const std::string& cookie, const std::string& peerIP) const
{
  auto token = base64::from_base64(cookie);
  if (token.empty())
    return false;

  std::vector<uint8_t> tdata;
  std::copy(token.begin(), token.end(), std::back_inserter(tdata));

  std::vector<uint8_t> data;
  if (!AESDecrypt(Key, tdata, data))
    return false;

  std::string v;
  std::copy(data.begin(), data.end(), std::back_inserter(v));

  size_t pos = v.find(',');
  if (pos == std::string::npos)
    return false;

  std::string ip = v.substr(0, pos);
  std::string t = v.substr(pos + 1);
  if (ip != peerIP)
    return false;

  time_t now = time(nullptr);
  time_t expires = std::strtoll(t.c_str(), nullptr, 10);
  return now < expires;
}

std::string Broker::CreateToken(const std::string& peerIP) const
{
  // ip:expires_time_t
  time_t expires = time(nullptr) + 24LL * 60 * 60;
  std::string value = peerIP + "," + std::to_string(expires);

  std::vector<uint8_t> token;
  std::vector<uint8_t> message;
  std::copy(value.begin(), value.end(), std::back_inserter(message));
  AESEncrypt(Key, message, token);

  std::string ret;
  std::copy(token.begin(), token.end(), std::back_inserter(ret));
  return base64::to_base64(ret);
}

bool Broker::VerifyAuthorization(const std::string& auth) const
{
  std::string a(auth);

  char* ctx = nullptr;
  char* p0 = strtok_r(&a[0], " ", &ctx);
  char* p1 = strtok_r(nullptr, " ", &ctx);
  if (p0 == nullptr || p1 == nullptr)
    return false;

  std::string k(p0);
  std::transform(k.begin(), k.end(), k.begin(), ::tolower);
  if (k != "basic")
    return false;

  auto av = base64::from_base64(p1);

  char* login = strtok_r(&av[0], ":", &ctx);
  char* pass = strtok_r(nullptr, ":", &ctx);
  if (login == nullptr || login != Login)
    return false;

  std::string pwd(pass ? pass : "");
  if (pwd != Pass)
    return false;

  return true;
}
