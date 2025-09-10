#pragma once

#include <list>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <Statme/http/Find2CRLF.h>
#include <Statme/http/StreamData.h>
#include <Statme/http/Version.h>
#include <Statme/Macros.h>

namespace HTTP
{
  namespace Header
  {
    typedef std::vector<std::string> StringArray;
    typedef std::shared_ptr<StringArray> StringArrayPtr;

    struct Field
    {
      std::string Key;
      StringArray Values;
    };

    enum class Verification
    {
      NotStrict,
      Strict
    };

    typedef std::shared_ptr<Field> FieldPtr;
    typedef std::list<FieldPtr> FieldList;

    struct Headers
    {
      size_t Size;
      std::string ReqRes;
      FieldList Header;
      std::string Body;

      bool LowerCase;
      bool MixedLineEndings;
      int LineEnding; // \n or \r\n

    public:
      STATMELNK Headers(bool lowerCase);
      STATMELNK virtual ~Headers();

      STATMELNK virtual HEADER_ERROR Parse(const StreamData& data, Verification type);
      STATMELNK virtual HEADER_ERROR Parse(const char* data, size_t length, Verification type);
      STATMELNK std::string Reparse();

      STATMELNK bool Empty() const;
      STATMELNK void Clear();

      STATMELNK std::string ToString(
        const char* indent = ""
        , bool dropTermination = false
        , bool appendBody = true
      ) const;

      STATMELNK std::string BodyToString(
        const char* indent = ""
        , bool InitialLF = false
        , size_t limit = 512
        , size_t split = 120
      ) const;

      STATMELNK StringArrayPtr GetHeader(
        const std::string& field
        , bool lowercase = false
        , const char* splitter = ","
      ) const;

      STATMELNK void DeleteHeader(const std::string& field);
      STATMELNK void AddHeader(const std::string& field, const std::string& value);
      STATMELNK void SetHeader(const std::string& field, const std::string& value);
      STATMELNK FieldList::const_iterator FindHeader(const std::string& field) const;
      STATMELNK bool HasHeader(const std::string& field);

      STATMELNK std::string GetFirstValue(
        const std::string& field
        , bool lowercase = true
        , const char* splitter = nullptr
        , const std::string& def = std::string()
      ) const;

      STATMELNK void CopyTo(Headers& to) const;

      STATMELNK static bool Complete(const std::vector<char>& data);
      STATMELNK static bool Complete(const char* data, size_t length);
      STATMELNK static size_t SizeOfHeader(const char* data, size_t length);

      STATMELNK static const char* sstrtok(
        char* str,
        const char* delimiters,
        char** context
      );

      STATMELNK static bool Printable(const std::string& str);
      STATMELNK static size_t CalcPrintable(const std::string& str);

      STATMELNK static bool ValidKey(const std::string& key);
      STATMELNK static bool ValidValue(const std::string& val);
      STATMELNK static bool ValidHeaderBuf(const std::string& buf);

    private:
      static void PushValue(StringArrayPtr arr, const std::string& value, bool lowrcase);
      int SenseType(char* buffer);
      char* ExtractHeaderLine(
        char* buffer
        , char*& context
        , int& type
      );
    };

    struct ReqHeaders : public Headers 
    {
      std::string Method;
      std::string Uri;
      Version Protocol;

    public:
      STATMELNK ReqHeaders(bool lowerCase = false);

      STATMELNK HEADER_ERROR Parse(const StreamData& data, Verification type) override;
      STATMELNK HEADER_ERROR Parse(const char* data, size_t length, Verification type) override;

      STATMELNK bool IsHeadRequest() const;
      STATMELNK void CopyTo(ReqHeaders& to) const;

      STATMELNK static HEADER_ERROR TryParse(std::string_view data);

    private:
      HEADER_ERROR ParseReqLine(Verification type);
    };

    struct ResHeaders : public Headers
    {
      Version Protocol;
      int Status;
      std::string Reason;

    public:
      STATMELNK ResHeaders(bool lowerCase = false);

      STATMELNK HEADER_ERROR Parse(const StreamData& data, Verification type) override;
      STATMELNK HEADER_ERROR Parse(const char* data, size_t length, Verification type) override;
      STATMELNK void CopyTo(ResHeaders& to) const;

    private:
      HEADER_ERROR ParseResLine(Verification type);
    };
  }
}

#define HEADERS_STR(h) h.ToString("  ", true, false).c_str()
#define BODY_STR(h) h.BodyToString("  ", true).c_str()