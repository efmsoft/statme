#include <sstream>

#include <Statme/http/Response/Formatter.h>

using namespace HTTP::Response;

Formatter::Formatter()
  : MainPage{Model::IPage::Create()}
{
}

Formatter::~Formatter()
{
}

void Formatter::AddTOCItem(
  bool active
  , const std::string& title
  , const std::string& link
  , const std::string& descr
)
{
  TOCItem item{};
  item.Active = active;
  item.Title = title;
  item.Link = link;
  item.Descr = descr;

  TOC.push_back(item);
}
