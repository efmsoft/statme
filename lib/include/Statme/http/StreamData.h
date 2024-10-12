#pragma once

#include <memory>
#include <string>
#include <vector>

#include <Statme/Macros.h>

struct StreamData : public std::vector<char>
{
  STATMELNK StreamData(size_t prealloc);

  STATMELNK size_t Append(const void* data, size_t size);
  STATMELNK size_t Append(const std::vector<char>& stream);
  STATMELNK size_t Append(const std::string& str);

  STATMELNK operator const char* () const;
};

typedef std::shared_ptr<StreamData> StreamDataPtr;