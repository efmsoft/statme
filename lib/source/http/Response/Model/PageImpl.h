#pragma once

#include <list>

#include <Statme/http/Response/Model/Page.h>

namespace Model
{
  class PageImpl : public IPage
  {
    std::list<std::shared_ptr<IDrawable>> AllContent;

  public:
    PageImpl& AddContent(const std::shared_ptr<IDrawable>& content) override;

    void DrawAsText(IScreen& screen) override;
    void DrawAsHtml(std::ostream& o) override;
  };
}
