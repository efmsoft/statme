#pragma once

#include <list>

#include <Statme/http/Response/Model/Panel.h>

namespace Model
{
  class PanelImpl : public IPanel
  {
    std::string Title;
    std::list<std::shared_ptr<IDrawable>> AllContent;

  public:
    explicit PanelImpl(const std::string& title);
    PanelImpl& AddContent(const std::shared_ptr<IDrawable>& content) override;

    void DrawAsText(IScreen& screen) override;
    void DrawAsHtml(std::ostream& o) override;
  };
}
