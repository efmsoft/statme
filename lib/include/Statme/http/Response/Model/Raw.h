#pragma once

#include <memory>

#include "Drawable.h"
#include <Statme/Macros.h>

namespace Model
{
  class IRaw : public IDrawable
  {
  public:
    virtual ~IRaw() = default;

    STATMELNK static std::shared_ptr<IRaw> Create(const std::string& data);
  };
}
