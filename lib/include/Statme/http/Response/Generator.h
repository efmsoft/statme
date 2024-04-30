#pragma once

#include <Statme/http/Headers.h>
#include <Statme/http/Response/Formatter.h>

namespace HTTP
{
  namespace Response
  {
    struct Generator
    {
      HTTP::Header::ResHeaders Headers;
      FormatterPtr Formatter;

    public:
      Generator(int code, const std::string& phrase, const Version& ver = Version());

      void SetFormatter(FormatterPtr formatter);
      void SetContentType(const std::string& mime);

      std::string Data() const;
    };

    struct OK : public Generator
    {
      OK() 
        : Generator(200, "OK", Version())
      {
      }
    };
 
    struct NOT_FOUND : public Generator
    {
      NOT_FOUND()
        : Generator(404, "Not Found", Version())
      {
      }
    };

    struct REDIRECT : public Generator
    {
      REDIRECT(const std::string& location)
        : Generator(301, "Moved Permanently", Version())
      {
        Headers.AddHeader("Location", location);
      }
    };

    struct INTERNAL_SERVER_ERROR : public Generator
    {
      INTERNAL_SERVER_ERROR() 
        : Generator(500, "Internal Server Error", Version())
      {
      }
    };

    extern const NOT_FOUND NotFound;
    extern const INTERNAL_SERVER_ERROR InternalServerError;
  }
}