#pragma once

#include <memory>
#include <string>

#include "Drawable.h"
#include <Statme/Macros.h>

namespace Model
{
  class ICheckbox : public IDrawable
  {
  public:
    virtual ~ICheckbox() = default;
    virtual std::string Id() const = 0;

    STATMELNK static std::shared_ptr<ICheckbox> Create(
      const std::string& label
      , bool checked
      , const std::string& onChangeScript = {}
    );
  };
}
