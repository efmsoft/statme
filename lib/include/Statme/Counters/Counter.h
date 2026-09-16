#pragma once

#include <functional>
#include <list>
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
  struct Counter;

  typedef std::shared_ptr<Counter> CounterPtr;
  typedef std::list<CounterPtr> CounterArray;

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

    // Position within Manager::Counters, set by Manager::AddCounter right
    // after insertion. Lets Manager::DeleteCounter erase in O(1) instead of
    // scanning every live counter -- this runs on every connection/request
    // teardown (Counters::Holder::~Holder), so with thousands of concurrent
    // counters the scan dominated CPU time under real load.
    CounterArray::iterator SelfIt;
    bool Registered;

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
}