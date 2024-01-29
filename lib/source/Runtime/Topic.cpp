#include <Statme/Runtime/Topic.h>

using namespace Runtime;

Topic::Topic(const char* name, TPrint print, const StringList& subtopics)
  : Name(name)
  , Subtopics(subtopics)
  , Print(print)
{
}

Topic::~Topic()
{
}
