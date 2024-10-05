#include <string.h>

#include <Statme/http/StreamData.h>

StreamData::StreamData(size_t prealloc)
  : std::vector<char>()
{
  reserve(prealloc);
}

StreamData::operator const char* () const
{
  return &(*this)[0];
}

void StreamData::Append(const std::vector<char>& stream)
{
  Append(&stream[0], stream.size());
}

void StreamData::Append(const std::string& str)
{
  Append(str.c_str(), str.size());
}

void StreamData::Append(const void* data, size_t n)
{
  if (n)
  {
    size_t cb = size();
    size_t size = cb + n;
    
    resize(size);
    memcpy(&(*this)[cb], data, n);
  }
}
