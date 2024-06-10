#pragma once

#include <string>
#include <vector>

#include <Statme/Macros.h>

// Apache - 8K
// Nginx - 4K - 8K
// IIS - 8K - 16K
// Tomcat - 8K – 48K
// Node(< 13) - 8K; (> 13) - 16K
#define MAX_HEADER_SIZE (48 * 1024)

enum class HEADER_ERROR
{
  NONE = 0,

  NOT_COMPLETED = -1,
  ZERO_CHAR = -2,
  TOO_LARGE = -3,
  INVALID = -4,
  INVALID_CHAR = -5,
  EMPTY_KEY = -6,
};

// Returns offset of next character after 
// "\r\n\r\n" or "\n\n" chracters sequesnce
// NOT_COMPLETED if sequence is not found
// ZERO_CHAR if '\0' character found
// TOO_LARGE if the header size exceeds the limit
STATMELNK size_t Find2CRLF(const char* p, size_t cb, int limit = 0);
STATMELNK size_t Find2CRLF(const std::string& str, int limit = 0);
STATMELNK size_t Find2CRLF(const std::vector<char>& arr, int limit = 0);
