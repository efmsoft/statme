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

  STATMELNK size_t Replace(const std::string& str);

  STATMELNK operator const char* () const;
};

// unique_ptr, not shared_ptr: the sole owner (Connection::Request::ReqStream/
// ResStream, and DataStream which inherits them) never shares this pointer --
// every other consumer only ever dereferences it -- so the refcount block was
// pure overhead on every request/response object (VTune, 2026-09).
typedef std::unique_ptr<StreamData> StreamDataPtr;