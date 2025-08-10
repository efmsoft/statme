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

size_t StreamData::Replace(const std::string& str)
{
  clear();
  return Append(str);
}

size_t StreamData::Append(const std::vector<char>& stream)
{
  return Append(&stream[0], stream.size());
}

size_t StreamData::Append(const std::string& str)
{
  return Append(str.c_str(), str.size());
}

size_t StreamData::Append(const void* data, size_t n)
{
  if (n)
  {
    size_t cb = size();
    size_t size = cb + n;
    
    resize(size);
    memcpy(&(*this)[cb], data, n);
  }

  return size();
}
