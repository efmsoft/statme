#include <sstream>

#include <Statme/http/Response/Generator.h>

using namespace HTTP::Response;

const NOT_FOUND HTTP::Response::NotFound;
const INTERNAL_SERVER_ERROR HTTP::Response::InternalServerError;

Generator::Generator(int code, const std::string& phrase, const Version& ver)
{
  Headers.Status = code;
  Headers.Reason = phrase;
  Headers.Protocol = ver;

  Headers.AddHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  Headers.AddHeader("Pragma", "no-cache");
  Headers.AddHeader("Expires", "0");
}

void Generator::SetFormatter(FormatterPtr formatter)
{
  SetContentType(formatter->GetMimeType());
  Formatter = formatter;
}

void Generator::SetContentType(const std::string& mime)
{
  Headers.DeleteHeader("Content-Type");

  if (!mime.empty())
    Headers.AddHeader("Content-Type", mime);
}

std::string Generator::Data() const
{
  std::string content;
  
  if (Formatter)
    content = Formatter->Run();

  std::stringstream ss;

  ss << "HTTP/"
    << Headers.Protocol.Major
    << "."
    << Headers.Protocol.Minor
    << " "
    << std::to_string(Headers.Status)
    << " "
    << Headers.Reason
    << "\r\n";

  for (auto& h : Headers.Header)
    for (auto& v : h->Values)
      ss << h->Key << ": " << v << "\r\n";

  if (!content.empty())
    ss << "Content-Length: " << std::to_string(content.size()) << "\r\n";

  ss << "\r\n";
  ss << content;

  return ss.str();
}
