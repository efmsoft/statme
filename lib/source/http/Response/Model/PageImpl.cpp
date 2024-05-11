#include "PageImpl.h"
#include "ScreenImpl.h"

using namespace Model;

PageImpl& PageImpl::AddContent(const std::shared_ptr<IDrawable>& content)
{
  AllContent.push_back(content);
  return *this;
}

void PageImpl::DrawAsText(IScreen& screen)
{
  for (auto& content : AllContent)
  {
    content->DrawAsText(screen);
    auto& screenImpl = dynamic_cast<ScreenImpl&>(screen);
    auto curPos = screenImpl.GetCurPos();
    screenImpl.NextLine(curPos.X);
  }
}

void PageImpl::DrawAsHtml(std::ostream& o)
{
  for (auto& content : AllContent)
    content->DrawAsHtml(o);
}

std::shared_ptr<IPage> IPage::Create()
{
  return std::make_shared<PageImpl>();
}
