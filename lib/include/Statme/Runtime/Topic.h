#pragma once

#include <functional>
#include <list>
#include <memory>
#include <string>
#include <string_view>

#include <Statme/http/Headers.h>
#include <Statme/http/Response/Formatter.h>
#include <Statme/http/Response/Generator.h>

namespace Runtime
{
  typedef std::list<std::string> StringList;
  typedef std::function<bool(HTTP::Response::Formatter&, const std::string&, const std::string&)> TPrint;
  typedef std::function<bool(
    const HTTP::Header::ReqHeaders& req
    , std::string_view reqBody
    , std::string& res
    )> TUnprocessedPrint;

  struct Topic
  {
    std::string Name;
    StringList Subtopics;
    TPrint Print;

  public:
    Topic(const char* name, TPrint print, const StringList& subtopics = StringList());
    ~Topic();
  };

  typedef std::shared_ptr<Topic> TopicPtr;
  typedef std::list<TopicPtr> TopicList;
}