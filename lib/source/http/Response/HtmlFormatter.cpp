#include <sstream>

#include <Statme/http/Response/Formatter.h>

using namespace HTTP::Response;

std::string HtmlFormatter::GetMimeType()
{
  return "text/html";
}

std::string HtmlFormatter::Run()
{
  std::stringstream ss;

  if (!TOC.empty())
  {
    ss << "<p>";
    for (auto& t : TOC)
    {
      ss << "&nbsp;<a href=\"" << t.Link << "\">" << t.Title << "</a>";
    }
    ss << "</p>\n";
  }

  if (!PRE.empty())
  {
    ss << "<pre>";
    for (auto& t : PRE)
    {
      ss << t << "\n";
    }
    ss << "</pre>\n";
  }

  return ss.str();
}
