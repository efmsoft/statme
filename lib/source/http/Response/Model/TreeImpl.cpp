#include "ScreenImpl.h"
#include "TreeImpl.h"

using namespace Model;

TreeImpl& TreeImpl::Add(const std::string& text, const std::shared_ptr<IDrawable>& content, bool collapsable)
{
  TreeItem item {
    .Text = text
    , .Content = content
    , .Collapsable = collapsable
  };

  Items.push_back(std::move(item));
  return *this;
}

void TreeImpl::DrawAsText(IScreen& screen)
{
  auto& screenImpl = dynamic_cast<ScreenImpl&>(screen);

  auto pos = screenImpl.GetCurPos();
  for (size_t i = 0; i < Items.size(); ++i)
  {
    const auto& item = Items[i];
    screenImpl.Write("|");
    screenImpl.NextLine(pos.X);
    screenImpl.Write(i < Items.size() - 1 ? "|-- " : "'-- ");
    auto itemPos = screenImpl.GetCurPos();
    screenImpl.Write(item.Text);
    screenImpl.NextLine(itemPos.X);

    ScreenImpl itemScreen;
    if (item.Content)
      item.Content->DrawAsText(itemScreen);

    screenImpl.WriteScreen(itemScreen, screenImpl.GetCurPos());
    pos.Y += 1;
    screenImpl.SetCurPos(pos);

    if (i < Items.size() - 1)
    {
      for (size_t y{}; y < itemScreen.GetHeight() + 1; ++y)
      {
        screenImpl.Write("|");
        pos.Y += 1;
        screenImpl.SetCurPos(pos);
      }
    }
  }
}

void TreeImpl::DrawAsHtml(std::ostream& o)
{
  o << "<ul class=\"tree\">\n";
  for (const auto& item : Items)
  {
    o << "<li>\n";
    if (item.Collapsable)
      o << "<details open>\n<summary>\n";

    o << "" << item.Text;

    if (item.Collapsable)
      o << "</summary>\n";
    
    if (item.Content)
      item.Content->DrawAsHtml(o);

    if (item.Collapsable)
      o << "</details>\n";
    o << "</li>\n";
  }
  o << "</ul>\n";
}

std::shared_ptr<ITree> ITree::Create()
{
  return std::make_shared<TreeImpl>();
}
