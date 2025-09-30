#pragma once

#include <memory>

#include "Drawable.h"
#include <Statme/Macros.h>

namespace Model
{
  class IPanel : public IDrawable
  {
  public:
    virtual ~IPanel() = default;
    virtual IPanel& AddContent(const std::shared_ptr<IDrawable>& content) = 0;

    IPanel& AddContent(IDrawable& content)
    {
      return AddContent(content.shared_from_this());
    }

    STATMELNK static std::shared_ptr<IPanel> Create(const std::string& title);
  };
}
