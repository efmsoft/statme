#include <Statme/http/Response/Formatter.h>

using namespace HTTP::Response;

StaticFormatter::StaticFormatter(const char* mime, const unsigned char* data, size_t size)
  : Mime(mime)
  , Data(data)
  , Size(size)
{
}

std::string StaticFormatter::GetMimeType()
{
  return Mime;
}

std::string HtmlFormatter::Run()
{
  return std::string((const char*)Data, Size);
}
