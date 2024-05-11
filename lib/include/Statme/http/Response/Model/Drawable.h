#pragma once

#include <memory>
#include <ostream>

#include "Screen.h"

namespace Model
{
  class IDrawable : public std::enable_shared_from_this<IDrawable>
  {
  public:
    virtual ~IDrawable() = default;
    virtual void DrawAsText(IScreen& screen) = 0;
    virtual void DrawAsHtml(std::ostream& o) = 0;
  };
}
