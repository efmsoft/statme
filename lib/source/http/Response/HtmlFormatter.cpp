#include <sstream>

#include <Statme/http/Response/Formatter.h>

using namespace HTTP::Response;

std::string HtmlFormatter::GetMimeType()
{
  return "text/html";
}

std::string HtmlFormatter::Run()
{
  if (Rel.empty())
    Rel = "./";

  std::stringstream ss;
  ss << "<html><head><link rel=\"stylesheet\" href=\"" << Rel << "statme.css\">";
  ss << "<link rel=\"icon\" type=\"image/x-icon\" href=\"" << Rel << "favicon.ico\">";
  ss << "</head><body>";

  if (!TOC.empty())
  {
    ss << "<div class=\"toc\">";

    for (auto& t : TOC)
    {
      if (t.Active)
        ss << "&nbsp;<span>" << t.Title << "</span>";
      else
        ss << "&nbsp;<a href=\"" << t.Link << "\">" << t.Title << "</a>";
    } 
    ss << "</div>\n";
  }

  ss << "<div class=\"content\">";
  MainPage->DrawAsHtml(ss);
  ss << "</div></body></html>";

  return ss.str();
}
