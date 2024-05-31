#pragma once

#include <list>

#include <Statme/http/Response/Model/Button.h>

namespace Model
{
  class ButtonImpl : public IButton
  {
    std::string Text;
    std::string Url;

  public:
    ButtonImpl(const std::string& text, const std::string& url);

    void DrawAsText(IScreen& screen) override;
    void DrawAsHtml(std::ostream& o) override;

  private:
    std::string Id() const;
  };
}
