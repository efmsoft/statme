#include "CheckboxImpl.h"
#include "ScreenImpl.h"

using namespace Model;

CheckboxImpl::CheckboxImpl(const std::string& label, bool checked, const std::string& onChangeScript)
  : Label{label}
  , Checked{checked}
  , OnChangeScript{onChangeScript}
{
}

std::string CheckboxImpl::Id() const
{
  return "cb_" + std::to_string(reinterpret_cast<uint64_t>(this));
}

void CheckboxImpl::DrawAsText(IScreen& screen)
{
  auto& s = static_cast<ScreenImpl&>(screen);
  std::string text = std::string("[") + (Checked ? "x" : " ") + "] " + Label;
  s.Write(text);
}

void CheckboxImpl::DrawAsHtml(std::ostream& o)
{
  auto id = Id();
  o << "<label><input type=\"checkbox\" id=\"" << id << "\"";
  if (Checked)
    o << " checked";

  if (!OnChangeScript.empty())
  {
    o << " onchange=\"on_cb_" << id << "(this.checked)\"";
  }

  o << " /> " << Label << "</label>\n";

  if (!OnChangeScript.empty())
  {
    o << "<script>\nfunction on_cb_" << id << "(checked) {\n";
    o << OnChangeScript;
    o << "}\n</script>\n";
  }
}

std::shared_ptr<ICheckbox> ICheckbox::Create(
  const std::string& label
  , bool checked
  , const std::string& onChangeScript
)
{
  return std::make_shared<CheckboxImpl>(label, checked, onChangeScript);
}
