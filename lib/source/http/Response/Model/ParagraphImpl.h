#pragma once

#include <vector>

#include <Statme/http/Response/Model/Paragraph.h>

#include "ScreenImpl.h"

namespace Model
{
  class ParagraphImpl : public IParagraph
  {
    ParagraphSettings Settings;
    std::vector<std::string> Lines;

  public:
    explicit ParagraphImpl(const ParagraphSettings& settings);
    ParagraphImpl& AppendText(const std::string& text) override;
    ParagraphImpl& AppendLine(std::istream& is) override;
    ParagraphImpl& AppendAll(std::istream& is) override;

    void DrawAsText(IScreen& screen) override;
    void DrawAsHtml(std::ostream& o) override;

  private:
    void AppendLineImpl(std::string line);
  };
}
