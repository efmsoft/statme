#pragma once

#include <memory>

#include "Drawable.h"
#include <Statme/Macros.h>

namespace Model
{
  class IScriptedButton : public IDrawable
  {
  public:
    virtual ~IScriptedButton() = default;

    STATMELNK static std::shared_ptr<IScriptedButton> Create(const std::string& text, const std::string& script);
  };
}
