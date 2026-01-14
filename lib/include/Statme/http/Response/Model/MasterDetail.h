#pragma once

#include <memory>

#include "Drawable.h"
#include <Statme/Macros.h>

namespace Model
{
  class IMasterDetail : public IDrawable
  {
  public:
    virtual ~IMasterDetail() = default;
    virtual void AddNavItem(const std::string& label, const std::string& url, bool active = false) = 0;
    virtual void SetDetail(const std::shared_ptr<IDrawable>& content) = 0;

    STATMELNK static std::shared_ptr<IMasterDetail> Create();
  };
}
