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
  ss << "<html><head><link rel=\"stylesheet\" href=\"/statme.css\"></head><body>";

  if (!TOC.empty())
  {
    ss << "<div class=\"toc\">";

    for (auto& t : TOC)
      ss << "&nbsp;<a href=\"" << t.Link << "\">" << t.Title << "</a>";

    ss << "</div>\n";
  }

  ss << "<div class=\"content\">";

  for (auto& table : Tables)
  {
    ss << "<table>\n";
    for (auto& r : table->Rows)
    {
      ss << "<tr>\n";
      for (auto& c : r->Columns)
      {
        if (r->Header)
          ss << "<th>" << c << "</th>";
        else
          ss << "<td>" << c << "</td>";
      }
      ss << "</tr>\n";
    }
    ss << "</table>\n";
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

  ss << "</div></body></html>";
  return ss.str();
}
