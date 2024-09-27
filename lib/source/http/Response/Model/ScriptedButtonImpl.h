#pragma once

#include <list>

#include <Statme/http/Response/Model/ScriptedButton.h>

namespace Model
{
  class ScriptedButtonImpl : public IScriptedButton
  {
    std::string Text;
    std::string Script;

  public:
    ScriptedButtonImpl(const std::string& text, const std::string& script);

    void DrawAsText(IScreen& screen) override;
    void DrawAsHtml(std::ostream& o) override;

  private:
    std::string Id() const;
  };
}
