#pragma once

#include <format>

#include <Statme/Counters/Holder.h>
#include <Statme/Counters/Manager.h>
#include <Syncme/ProcessThreadId.h>
#include <Syncme/SetThreadName.h>

#define CNTR_PROP_TYPE "type"
#define CNTR_PROP_URL "url"
#define CNTR_PROP_HOST "host"
#define CNTR_PROP_LINK "link"
#define CNTR_PROP_LOGFILE "logfile"
#define CNTR_PROP_LOGCH "logch"
#define CNTR_PROP_THREADID "thread"
#define CNTR_PROP_UUID "request-id"                 // Unique identifier of request
#define CNTR_PROP_INDEX "index"             // Index of request within keep-alive http/https connection
#define CNTR_PROP_CLIENT "client"
#define CNTR_PROP_PARENT "parent-id"

#define CNTR_CATEGORY_PROTOCOL "protocol"
#define CNTR_CATEGORY_SCHEME "scheme"
#define CNTR_CATEGORY_THREAD "thread"
#define CNTR_CATEGORY_REQUEST "request"

#define _CNTRM_INST_ Counters::Manager::GetInstance()

#define INIT_CNTR_HOLDER(holder, self, category) \
  holder(_CNTRM_INST_ ? _CNTRM_INST_->AddCounter(std::format("{}", (const void*)self), category) : Counters::CounterPtr())

#define THREAD_COUNTER(name) \
  Counters::Holder holder(_CNTRM_INST_ ? _CNTRM_INST_->AddCounter(name, CNTR_CATEGORY_THREAD) : Counters::CounterPtr()); \
  if (holder.Ptr) holder.Ptr->SetProperty(CNTR_PROP_THREADID, std::to_string(Syncme::GetCurrentThreadId()))

#define THREAD_COUNTER2(counters, name) \
  Counters::Holder holder(counters->AddCounter(name, CNTR_CATEGORY_THREAD)); \
  holder.Ptr->SetProperty(CNTR_PROP_THREADID, std::to_string(Syncme::GetCurrentThreadId()))

#define SET_CUR_THREAD_NAME_EX(name) \
  SET_CUR_THREAD_NAME(name); \
  THREAD_COUNTER(name)

#define SET_CUR_THREAD_NAME_EX2(counters, name) \
  SET_CUR_THREAD_NAME(name); \
  THREAD_COUNTER2(counters, name)
