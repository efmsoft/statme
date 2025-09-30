#include <string>

#include "ScriptedButtonImpl.h"
#include "ScreenImpl.h"

using namespace Model;

ScriptedButtonImpl::ScriptedButtonImpl(const std::string& text, const std::string& script)
  : Text{text}
  , Script{script}
{
}

std::string ScriptedButtonImpl::Id() const
{
  return std::to_string(reinterpret_cast<uint64_t>(this));
}

void ScriptedButtonImpl::DrawAsText(IScreen& screen)
{
  // Text formatter is non-interactive, no need to draw buttons
}

void ScriptedButtonImpl::DrawAsHtml(std::ostream& o)
{
  o << "<div class=\"scriptedbutton\"><button type=\"button\" onclick=\"on_button_" + Id() + "()\">" + Text + "</button></div>\n";
  o << "<script>\nfunction on_button_" + Id() + "() {\n";
  o << Script;
  o << "}\n</script>";
}

std::shared_ptr<IScriptedButton> IScriptedButton::Create(const std::string& text, const std::string& script)
{
  return std::make_shared<ScriptedButtonImpl>(text, script);
}
