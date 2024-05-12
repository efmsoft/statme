#pragma once

#include <memory>
#include <variant>
#include <vector>

#include <Statme/http/Response/Model/Tree.h>

namespace Model
{
  class TreeImpl : public ITree
  {
    struct TreeItem
    {
      std::string Text;
      std::shared_ptr<IDrawable> Content;
      bool Collapsable;
    };

    std::vector<TreeItem> Items;

  public:
    TreeImpl& Add(const std::string& text, const std::shared_ptr<IDrawable>& content, bool collapsable) override;

    void DrawAsText(IScreen& screen) override;
    void DrawAsHtml(std::ostream& o) override;
  };
}
