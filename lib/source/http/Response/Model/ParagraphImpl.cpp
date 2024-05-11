#include "ParagraphImpl.h"

using namespace Model;

ParagraphImpl::ParagraphImpl(const ParagraphSettings& settings)
  : Settings{settings}
{
}

void ParagraphImpl::AppendLineImpl(std::string line)
{
  Lines.push_back(std::move(line));
}

ParagraphImpl& ParagraphImpl::AppendText(const std::string& text)
{
  auto trimAndAppend = [&](size_t start, size_t end) {
    while (start < end && text[start] == '\r')
      ++start;
    while (start < end && text[end - 1] == '\r')
      --end;
    AppendLineImpl(std::string(text.begin() + start, text.begin() + end));
  };

  size_t start{};
  while (start < text.length())
  {
    auto end = text.find_first_of('\n', start);
    if (end == -1)
      end = text.length();
    trimAndAppend(start, end);
    start = ++end;
  }

  return *this;
}

ParagraphImpl& ParagraphImpl::AppendLine(std::istream& is)
{
  std::string line;
  if (std::getline(is, line))
    AppendLineImpl(std::move(line));
  return *this;
}

ParagraphImpl& ParagraphImpl::AppendAll(std::istream& is)
{
  std::string line;
  while (std::getline(is, line))
    AppendLineImpl(std::move(line));
  return *this;
}

void ParagraphImpl::DrawAsText(IScreen& screen)
{
  auto& screenImpl = dynamic_cast<ScreenImpl&>(screen);
  auto curX = screenImpl.GetCurPos().X;
  for (const auto& l : Lines)
  {
    screenImpl.Write(l);
    screenImpl.NextLine(curX);
  }
}

void ParagraphImpl::DrawAsHtml(std::ostream& o)
{
  if (Settings.Uniform)
  {
    o << "<pre>";
    for (const auto& l : Lines)
      o << l << "\n";
    o << "</pre>\n";
  }
  else
  {
    if (!Lines.empty())
    {
      o << Lines.front();
      for (size_t i{1}; i < Lines.size(); ++i)
        o << "<br>" << Lines[i];
    }
  }
}

std::shared_ptr<IParagraph> IParagraph::Create(const std::string& text, const ParagraphSettings& settings)
{
  auto paragraph = std::make_shared<ParagraphImpl>(settings);
  paragraph->AppendText(text);
  return paragraph;
}
