#include <sstream>

#include <Statme/http/Response/Formatter.h>
#include "Model/ScreenImpl.h"

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

  auto screen = Model::IScreen::Create();
  MainPage->DrawAsText(*screen);
  screen->Dump(ss);

  return ss.str();
}
