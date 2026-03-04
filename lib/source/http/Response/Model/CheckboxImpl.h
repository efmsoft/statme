#pragma once

#include <Statme/http/Response/Model/Checkbox.h>

namespace Model
{
  class CheckboxImpl : public ICheckbox
  {
    std::string Label;
    bool Checked;
    std::string OnChangeScript;

  public:
    CheckboxImpl(const std::string& label, bool checked, const std::string& onChangeScript);

    std::string Id() const override;
    void DrawAsText(IScreen& screen) override;
    void DrawAsHtml(std::ostream& o) override;
  };
}
