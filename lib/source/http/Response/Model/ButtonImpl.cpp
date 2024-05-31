#include <string>

#include "ButtonImpl.h"
#include "ScreenImpl.h"

using namespace Model;

ButtonImpl::ButtonImpl(const std::string& text, const std::string& url)
  : Text{text}
  , Url{url}
{
}

std::string ButtonImpl::Id() const
{
  return std::to_string(reinterpret_cast<uint64_t>(this));
}

void ButtonImpl::DrawAsText(IScreen& screen)
{
  // Text formatter is non-interactive, no need to draw buttons
}

void ButtonImpl::DrawAsHtml(std::ostream& o)
{
  o << R"(
<form action=")" + Url + R"(">
    <input type="submit" value=")" + Text + R"(" />
</form>
)";
}

std::shared_ptr<IButton> IButton::Create(const std::string& text, const std::string& url)
{
  return std::make_shared<ButtonImpl>(text, url);
}
