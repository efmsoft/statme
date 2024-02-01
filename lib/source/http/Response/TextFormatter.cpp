#include <sstream>

#include <Statme/http/Response/Formatter.h>

using namespace HTTP::Response;

std::string TextFormatter::GetMimeType()
{
  return "text/plain";
}

std::string TextFormatter::Run()
{
  std::stringstream ss;

  if (!TOC.empty())
  {
    ss << "Items:";
    for (auto& t : TOC)
    {
      ss << " " << t.Title;
    }
    ss << "\n";
  }

  if (!PRE.empty())
  {
    for (auto& t : PRE)
    {
      ss << t << "\n";
    }
  }

  return ss.str();
}
