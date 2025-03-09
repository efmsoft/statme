#pragma once

#include <memory>
#include <variant>
#include <vector>

#include <Statme/http/Response/Model/Tab.h>

namespace Model
{
  class TabManagerImpl : public ITabManager
  {
    struct TabManagerItem
    {
      std::string Title;
      std::shared_ptr<IDrawable> Content;
    };

    std::vector<TabManagerItem> Items;

  public:
    TabManagerImpl& AddTab(const std::string& title, const std::shared_ptr<IDrawable>& content) override;

    void DrawAsText(IScreen& screen) override;
    void DrawAsHtml(std::ostream& o) override;
  };
}
