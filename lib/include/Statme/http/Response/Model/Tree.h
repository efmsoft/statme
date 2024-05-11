#pragma once

#include <memory>
#include <string>

#include "Drawable.h"
#include <Statme/Macros.h>


namespace Model
{
  class ITree : public IDrawable
  {
  public:
    virtual ~ITree() = default;
    virtual ITree& Add(const std::string& text, const std::shared_ptr<IDrawable>& content = {}, bool collapsable = false) = 0;

    ITree& Add(const std::string& text, IDrawable& content, bool collapsable)
    {
      return Add(text, content.shared_from_this(), collapsable);
    }

    STATMELNK static std::shared_ptr<ITree> Create();
  };
}
