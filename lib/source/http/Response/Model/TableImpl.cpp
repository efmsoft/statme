#include "ScreenImpl.h"
#include "TableImpl.h"

using namespace Model;

const std::vector<std::shared_ptr<IDrawable>>& RowImpl::GetCells() const
{
  return Cells;
}

RowImpl& RowImpl::AddCell(const std::shared_ptr<IDrawable>& cell)
{
  Cells.push_back(cell);
  return *this;
}

TableImpl::TableImpl()
  : Rows(1)
{
}

RowImpl& TableImpl::AppendHeaderRow()
{
  auto row = std::make_shared<RowImpl>();
  Rows[0] = row;
  HasHeader = true;
  return *row;
}

RowImpl& TableImpl::AppendRow()
{
  auto row = std::make_shared<RowImpl>();
  Rows.push_back(row);
  return *row;
}

void TableImpl::DrawAsText(IScreen& screen)
{
  auto& screenImpl = dynamic_cast<ScreenImpl&>(screen);

  std::vector<std::vector<ScreenImpl>> cellScreens;
  for (size_t i{}; i < Rows.size(); ++i)
  {
    if (i == 0 && !HasHeader)
      continue;

    const auto& row = Rows[i];
    std::vector<ScreenImpl> screenRow;
    for (const auto& cell: row->GetCells())
    {
      ScreenImpl cellScreen;
      cell->DrawAsText(cellScreen);
      screenRow.push_back(std::move(cellScreen));
    }
    cellScreens.push_back(std::move(screenRow));
  }

  screenImpl.DrawTable(cellScreens);
}

void TableImpl::DrawAsHtml(std::ostream& o)
{
  o << "<table>\n";
  for (size_t i{}; i < Rows.size(); ++i)
  {
    if (i == 0 && !HasHeader)
      continue;

    const auto& row = Rows[i];
    auto cells = row->GetCells();
    size_t cellsCount = cells.size();
    if (HasHeader)
      cellsCount = std::max(cellsCount, Rows.front()->GetCells().size());

    o << "<tr>\n";
    for (size_t j{}; j < cellsCount; ++j)
    {
      const auto& cell = j < cells.size() ? cells[j] : IParagraph::Create();
      o << (i == 0 ? "<th>" : "<td>");
      cell->DrawAsHtml(o);
      o << (i == 0 ? "</th>" : "</td>");
    }
    o << "</tr>\n";
  }
  o << "</table>\n";
}

std::shared_ptr<ITable> ITable::Create()
{
  return std::make_shared<TableImpl>();
}
