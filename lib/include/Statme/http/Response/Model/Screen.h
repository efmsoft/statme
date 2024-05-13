#pragma once

#include <memory>
#include <ostream>

#include <Statme/Macros.h>

namespace Model
{
  struct Position
  {
    size_t X{};
    size_t Y{};
  };

  class IScreen
  {
  public:
    virtual ~IScreen() = default;
    virtual void Dump(std::ostream& os, Position leftTopPos = {0, 0}, Position rightBottomPos = {(size_t) -1, (size_t)-1}) = 0;

    STATMELNK static std::shared_ptr<IScreen> Create();
  };
}
