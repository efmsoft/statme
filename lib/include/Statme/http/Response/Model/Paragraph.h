#pragma once

#include <istream>
#include <string>
#include <string_view>

#include "Drawable.h"
#include <Statme/Macros.h>

namespace Model
{
  struct ParagraphSettings
  {
    bool Uniform{true};
    size_t LineLength{128};
  };

  class IParagraph : public IDrawable
  {
  public:
    virtual ~IParagraph() = default;
    virtual IParagraph& AppendText(const std::string& text) = 0;
    virtual IParagraph& AppendLine(std::istream& is) = 0;
    virtual IParagraph& AppendAll(std::istream& is) = 0;

    STATMELNK static std::shared_ptr<IParagraph> Create(const std::string& text = {}, const ParagraphSettings& settings = {});
  };
}
