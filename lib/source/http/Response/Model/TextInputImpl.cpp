#include "TextInputImpl.h"
#include "ScreenImpl.h"

using namespace Model;

TextInputImpl::TextInputImpl(const std::string& text, const TextInputSettings& settings)
  : Text{text}
  , Settings{settings}
{
}

std::string TextInputImpl::Id() const
{
  return std::to_string(reinterpret_cast<uint64_t>(this));;
}

void TextInputImpl::DrawAsText(IScreen& screen)
{
}

void TextInputImpl::DrawAsHtml(std::ostream& o)
{
  o << "<textarea id=\"" << Id() << "\" rows=\"" << Settings.Rows << "\" cols = \"" << Settings.Cols << "\">" << Text << "</textarea>";
}

std::shared_ptr<ITextInput> Model::ITextInput::Create(const std::string& text, const TextInputSettings& settings)
{
  return std::make_shared<TextInputImpl>(text, settings);
}
