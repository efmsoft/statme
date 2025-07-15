#pragma once

#include <memory>

#include "Drawable.h"
#include <Statme/Macros.h>

namespace Model
{
  class IDropdown : public IDrawable
  {
  public:
    virtual ~IDropdown() = default;
    virtual void AddItem(const std::string& name, const std::string& url) = 0;

    STATMELNK static std::shared_ptr<IDropdown> Create();
  };
}
