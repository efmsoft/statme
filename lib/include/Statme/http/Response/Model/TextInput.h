#pragma once

#include <memory>
#include <string>

#include "Drawable.h"
#include <Statme/Macros.h>

namespace Model
{
  struct TextInputSettings
  {
    size_t Rows = 30;
    size_t Cols = 100;
  };

  class ITextInput : public IDrawable
  {
  public:
    virtual ~ITextInput() = default;
    virtual std::string Id() const = 0;

    STATMELNK static std::shared_ptr<ITextInput> Create(const std::string& text, const TextInputSettings& settings = TextInputSettings{});
  };
}
