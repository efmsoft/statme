#include <string>

#include "RawImpl.h"
#include "ScreenImpl.h"

using namespace Model;

RawImpl::RawImpl(const std::string& data)
  : Data{data}
{
}

void RawImpl::DrawAsText(IScreen& screen)
{
}

void RawImpl::DrawAsHtml(std::ostream& o)
{
  o << Data;
}

std::shared_ptr<IRaw> IRaw::Create(const std::string& data)
{
  return std::make_shared<RawImpl>(data);
}
