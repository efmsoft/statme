#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <stdint.h>
#include <string>
#include <vector>

#include <json/json.h>

#include <Statme/Macros.h>

namespace Counters
{
  class Manager;
  typedef std::map<std::string, std::string> PropMap;
  typedef std::function<std::string(const std::string&)> TGetValue;
  typedef std::map<std::string, TGetValue> UpdaterMap;

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
    UpdaterMap Updater;
  
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

      STATMELNK void SetPropertyUpdater(const std::string& name, TGetValue u);
      STATMELNK void Update(const std::optional<std::list<std::string>>& props);

      Json::Value Get(const std::optional<std::list<std::string>>& props) const;
  };

  typedef std::shared_ptr<Counter> CounterPtr;
  typedef std::vector<CounterPtr> CounterArray;
}