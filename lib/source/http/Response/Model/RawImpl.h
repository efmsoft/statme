#pragma once

#include <list>

#include <Statme/http/Response/Model/Raw.h>

namespace Model
{
  class RawImpl : public IRaw
  {
    std::string Data;

  public:
    RawImpl(const std::string& data);

    void DrawAsText(IScreen& screen) override;
    void DrawAsHtml(std::ostream& o) override;
  };
}
