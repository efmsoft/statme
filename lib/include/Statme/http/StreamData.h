#pragma once

#include <memory>
#include <string>
#include <vector>

#include <Statme/Macros.h>

struct StreamData : public std::vector<char>
{
  STATMELNK StreamData(size_t prealloc);

  STATMELNK void Append(const void* data, size_t size);
  STATMELNK void Append(const std::vector<char>& stream);
  STATMELNK void Append(const std::string& str);

  STATMELNK operator const char* () const;
};

typedef std::shared_ptr<StreamData> StreamDataPtr;