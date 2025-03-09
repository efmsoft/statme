#pragma once

#include <memory>
#include <string>

#include "Drawable.h"
#include <Statme/Macros.h>


namespace Model
{
  class ITabManager : public IDrawable
  {
  public:
    virtual ~ITabManager() = default;
    virtual ITabManager& AddTab(const std::string& title, const std::shared_ptr<IDrawable>& content) = 0;

    STATMELNK static std::shared_ptr<ITabManager> Create();
  };
}
