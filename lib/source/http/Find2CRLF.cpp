#include <stdint.h>

#include <Statme/http/Find2CRLF.h>

#pragma warning(disable : 6276)

#define CR_LF_CR_LF *(uint32_t*)"\r\n\r\n"
#define LF_LF *(uint16_t*)"\n\n"

size_t Find2CRLF(const std::string& str, int limit)
{
  return Find2CRLF(str.c_str(), str.size(), limit);
}

size_t Find2CRLF(const std::vector<char>& arr, int limit)
{
  return Find2CRLF(&arr[0], arr.size(), limit);
}

size_t Find2CRLF(const char* p, size_t cb, int limit)
{
  if (limit <= 0)
    limit = MAX_HEADER_SIZE;
  else
    limit += 1024;

  uint32_t crlfcrlf = CR_LF_CR_LF;
  uint16_t lflf = LF_LF;

  for (size_t pos = 0; cb > 0; p++, pos++, cb--)
  {
    if (*p == '\0')
      return (size_t)HEADER_ERROR::ZERO_CHAR;

    if (pos > limit)
      return (size_t)HEADER_ERROR::TOO_LARGE;

    if (cb >= sizeof(uint32_t))
    {
      uint32_t b = *(uint32_t*)p;
      if (b == crlfcrlf)
        return pos + sizeof(uint32_t);
    }

    if (cb >= sizeof(uint16_t))
    {
      uint16_t b = *(uint16_t*)p;
      if (b == lflf)
        return pos + sizeof(uint16_t);
    }
  }
  return (size_t)HEADER_ERROR::NOT_COMPLETED;
}

