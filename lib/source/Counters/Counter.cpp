#include <Syncme/TickCount.h>

#include <Statme/Counters/Counter.h>
#include <Statme/Counters/Manager.h>

using namespace Syncme;
using namespace Counters;

uint64_t Counters::Counter::IDGenerator = 0;

Counter::Counter(
  Manager* owner
  , const char* pointer
  , const char* category
)
  : Created(0)
  , Updated(0)
  , Deleted(0)
  , Pointer(pointer)
  , Category(category)
  , Owner(owner)
  , ID(0)
{
  ID = ++IDGenerator;
  Created = Updated = GetTimeInMillisec();
}

void Counter::SetProperty(
  const char* name
  , const std::string& value
)
{
  SetProperty(name, value.c_str());
}

void Counter::SetProperty(
  const char* name
  , const char* value
)
{
  std::lock_guard guard(Owner->GetLock());

  Properties[name] = value;
  Updated = GetTimeInMillisec();
  Owner->SetDirty();
}

void Counter::SetPropertyUpdater(const std::string& name, TGetValue u)
{
  std::lock_guard guard(Owner->GetLock());

  if (u == nullptr)
  {
    auto it = Updater.find(name);
    if (it != Updater.end())
      Updater.erase(it);
  }
  else
    Updater[name] = u;
}

void Counter::Update()
{
  std::lock_guard guard(Owner->GetLock());

  if (Updater.empty())
    return;

  for (auto& p : Properties)
  {
    auto it = Updater.find(p.first);
    if (it != Updater.end() && it->second)
    {
      std::string v = it->second(it->first);
      if (v != p.second)
      {
        p.second = v;
        Owner->SetDirty();
      }
    }
  }
}

Json::Value Counter::Get() const
{
  Json::Value value;

  value["id"] = ID;
  value["ptr"] = Pointer;
  value["category"] = Category;
  value["created"] = Created;
  value["updated"] = Updated;
  value["deleted"] = Deleted;

  Json::Value props(Json::arrayValue);
  for (auto& p : Properties)
  {
    Json::Value v;
    v[p.first] = p.second;

    props.append(v);
  }

  value["properties"] = props;
  return value;
}
