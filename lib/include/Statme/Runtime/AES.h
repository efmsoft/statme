#pragma once

#include <stdint.h>
#include <vector>

bool AESEncrypt(
  const std::vector<uint8_t>& key
  , const std::vector<uint8_t>& message
  , std::vector<uint8_t>& output
);

bool AESDecrypt(
  const std::vector<uint8_t>& key
  , const std::vector<uint8_t>& message
  , std::vector<uint8_t>& output
);
