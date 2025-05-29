#pragma once

#include <stdint.h>
#include <vector>

#include <Statme/Macros.h>

namespace Runtime
{
  STATMELNK bool AESEncrypt(
    const std::vector<uint8_t>& key
    , const std::vector<uint8_t>& message
    , std::vector<uint8_t>& output
  );

  STATMELNK bool AESDecrypt(
    const std::vector<uint8_t>& key
    , const std::vector<uint8_t>& message
    , std::vector<uint8_t>& output
  );
}
