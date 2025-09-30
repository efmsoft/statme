#include "PanelImpl.h"
#include "ScreenImpl.h"

using namespace Model;

PanelImpl::PanelImpl(const std::string& title)
  : Title{title}
{
}

PanelImpl& PanelImpl::AddContent(const std::shared_ptr<IDrawable>& content)
{
  AllContent.push_back(content);
  return *this;
}

void PanelImpl::DrawAsText(IScreen& screen)
{
  for (auto& content : AllContent)
  {
    content->DrawAsText(screen);
    auto& screenImpl = dynamic_cast<ScreenImpl&>(screen);
    auto curPos = screenImpl.GetCurPos();
    screenImpl.NextLine(curPos.X);
  }
}

void PanelImpl::DrawAsHtml(std::ostream& o)
{
  o << "<fieldset class=\"section\">\n";
  o << "<legend>" << Title << "</legend>\n";
  for (auto& content : AllContent)
    content->DrawAsHtml(o);
  o << "</fieldset>\n";
}

std::shared_ptr<IPanel> IPanel::Create(const std::string& title)
{
  return std::make_shared<PanelImpl>(title);
}
