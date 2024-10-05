#include <cassert>
#include <string.h>

#include <Statme/http/StreamData.h>

static const size_t BLOCK_SIZE = 64UL * 1024;

static size_t AlignSize(size_t n)
{
  size_t blocks = ((n + BLOCK_SIZE - 1) / BLOCK_SIZE);
  return blocks * BLOCK_SIZE;
}

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
    
    if (size > capacity())
    {
      size_t ncb = AlignSize(size);
      assert(ncb >= size);

      reserve(ncb);
    }

    resize(size);
    memcpy(&(*this)[cb], data, n);
  }
}
