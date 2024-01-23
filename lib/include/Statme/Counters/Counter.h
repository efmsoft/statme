#pragma once

#include <map>
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

#include <json/json.h>

#include <Statme/Macros.h>

namespace Counters
{
  class Manager;
  typedef std::map<std::string, std::string> PropMap;

  constexpr static uint64_t DELETE_AFTER = 5000;

  struct Counter
  {
    uint64_t Created;
    uint64_t Updated;
    uint64_t Deleted;
    std::string Pointer;
    std::string Category;
    PropMap Properties;

    Manager* Owner;
  
    static uint64_t IDGenerator;
    uint64_t ID;

    public:
      STATMELNK Counter(
        Manager* owner
        , const char* pointer
        , const char* category
      );

      STATMELNK void SetProperty(
        const char* name
        , const std::string& value
      );

      STATMELNK void SetProperty(
        const char* name
        , const char* value
      );

      Json::Value Get() const;
  };

  typedef std::shared_ptr<Counter> CounterPtr;
  typedef std::vector<CounterPtr> CounterArray;
}