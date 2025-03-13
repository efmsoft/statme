#pragma once

#include <vector>

#include <Statme/http/Response/Model/Table.h>

namespace Model
{
  class RowImpl : public IRow
  {
    std::vector<std::shared_ptr<IDrawable>> Cells;
    std::vector<std::string> Styles;

  public:
    const std::vector<std::shared_ptr<IDrawable>>& GetCells() const;
    const std::vector<std::string>& GetStyles() const;
    RowImpl& AddCell(const std::shared_ptr<IDrawable>& cell, const std::string& style) override;
  };

  class TableImpl : public ITable
  {
    std::vector<std::shared_ptr<RowImpl>> Rows;
    bool HasHeader{};

  public:
    TableImpl();
    RowImpl& AppendHeaderRow() override;
    RowImpl& AppendRow() override;

    void DrawAsText(IScreen& screen) override;
    void DrawAsHtml(std::ostream& o) override;
  };
}
