#pragma once

#include <memory>
#include <variant>
#include <vector>

#include <Statme/http/Response/Model/TextInput.h>

namespace Model
{
  class TextInputImpl : public ITextInput
  {
    std::string Text;
    TextInputSettings Settings;

  public:
    explicit TextInputImpl(const std::string& text, const TextInputSettings& settings);

    std::string Id() const override;

    void DrawAsText(IScreen& screen) override;
    void DrawAsHtml(std::ostream& o) override;
  };
}
