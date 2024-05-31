#pragma once

#include <memory>

#include "Drawable.h"
#include <Statme/Macros.h>

namespace Model
{
  class IButton : public IDrawable
  {
  public:
    virtual ~IButton() = default;

    STATMELNK static std::shared_ptr<IButton> Create(const std::string& text, const std::string& url);
  };
}
