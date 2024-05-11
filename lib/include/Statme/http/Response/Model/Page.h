#pragma once

#include <memory>

#include "Drawable.h"
#include <Statme/Macros.h>

namespace Model
{
  class IPage : public IDrawable
  {
  public:
    virtual ~IPage() = default;
    virtual IPage& AddContent(const std::shared_ptr<IDrawable>& content) = 0;

    IPage& AddContent(IDrawable& content)
    {
      return AddContent(content.shared_from_this());
    }

    STATMELNK static std::shared_ptr<IPage> Create();
  };
}
