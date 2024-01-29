#pragma once

#include <functional>
#include <list>
#include <memory>
#include <string>

namespace Runtime
{
  typedef std::list<std::string> StringList;
  typedef std::function<std::string(const std::string&, const StringList&)> TPrint;

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