#pragma once

#include <memory>

#include "Drawable.h"
#include "Paragraph.h"
#include <Statme/Macros.h>

namespace Model
{
  class IRow
  {
  public:
    virtual ~IRow() = default;
    virtual IRow& AddCell(const std::shared_ptr<IDrawable>& cell) = 0;
    IRow& AddCell(const std::string& text)
    {
      return AddCell(IParagraph::Create(text, {.Uniform = false}));
    }
  };

  class ITable : public IDrawable
  {
  public:
    virtual ~ITable() = default;
    virtual IRow& AppendHeaderRow() = 0;
    virtual IRow& AppendRow() = 0;

    STATMELNK static std::shared_ptr<ITable> Create();
  };
}
