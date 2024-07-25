#include <stdexcept>

#include "ScreenImpl.h"

using namespace Model;

Position ScreenImpl::GetCurPos()
{
  return CurPos;
}

void ScreenImpl::SetCurPos(Position p)
{
  CurPos = p;
}

void ScreenImpl::NextLine(size_t x)
{
  CurPos.X = x;
  ++CurPos.Y;
}

size_t ScreenImpl::GetHeight() const
{
  return Screen.size();
}

size_t ScreenImpl::GetMaxWidth() const
{
  size_t maxWidth{};
  for (const auto& line : Screen)
    maxWidth = std::max(maxWidth, line.size());
  return maxWidth;
};

const ScreenImpl::Line& ScreenImpl::GetLine(size_t y) const
{
  if (Screen.size() <= y)
    throw std::runtime_error("Out of range");

  return Screen[y];
}

void ScreenImpl::Write(std::string_view str)
{
  for (auto c : str)
  {
    if (c == '\n')
    {
      EnsureScreenHeight(CurPos.Y);
      ++CurPos.Y;
      CurPos.X = 0;
      continue;
    }

    auto& line = EnsureLineWidth(CurPos);
    line[CurPos.X++].C = c;
  }
}

void ScreenImpl::WriteScreen(const ScreenImpl& src, Position p)
{
  for (size_t y{}; y < src.GetHeight(); ++y)
  {
    const auto& srcLine = src.GetLine(y);
    auto& dstLine = EnsureLineWidth({p.X + srcLine.size(), p.Y + y});
    std::copy(srcLine.cbegin(), srcLine.cend(), dstLine.begin() + p.X);
  }
}

ScreenImpl& ScreenImpl::operator<<(std::string_view str)
{
  Write(str);
  return *this;
}

void ScreenImpl::Dump(std::ostream& os, Position leftTopPos, Position rightBottomPos)
{
  for (auto y = leftTopPos.Y; y < std::min(Screen.size(), rightBottomPos.Y); ++y)
  {
    const auto& line = Screen[y];
    for (auto x = leftTopPos.X; x < std::min(line.size(), rightBottomPos.X); ++x)
      os << line[x].C;
    os << "\n";
  }
}

void ScreenImpl::EnsureScreenHeight(size_t y)
{
  if (Screen.size() <= y)
    Screen.resize(y + 1);
}

ScreenImpl::Line& ScreenImpl::EnsureLineWidth(Position p)
{
  EnsureScreenHeight(p.Y);
  auto& line = Screen[p.Y];
  if (line.size() <= p.X)
    line.resize(p.X + 1);
  return line;
}

std::string Pad(std::string str, char padSymbol, size_t totalLength)
{
  auto result = str;
  auto oldLength{result.length()};
  if (oldLength >= totalLength)
    return result;

  result.resize(totalLength);
  std::fill(result.begin() + oldLength, result.end(), padSymbol);
  return result;
}

class TablePrinter
{
  ScreenImpl& Out;
  const std::vector<std::vector<ScreenImpl>>& Cells;
  size_t MaxColumns{};
  std::vector<size_t> RowHeights;
  std::vector<size_t> ColumnWidths;

public:
  TablePrinter(ScreenImpl& out, const std::vector<std::vector<ScreenImpl>>& cells)
    : Out{out}
    , Cells{cells}
  {
    for (const auto& row : Cells)
    {
      RowHeights.push_back(1);
      MaxColumns = std::max(MaxColumns, row.size());
      ColumnWidths.resize(MaxColumns);

      for (size_t colIndex{}; colIndex < row.size(); ++colIndex)
      {
        const auto& cell = row[colIndex];
        auto width = cell.GetMaxWidth();
        auto height = cell.GetHeight();
        RowHeights.back() = std::max(RowHeights.back(), height);
        ColumnWidths[colIndex] = std::max(ColumnWidths[colIndex], width);
      }
    }
  }

  void Print()
  {
    PrintBorderLine();
    for (size_t rowIndex{}; rowIndex < Cells.size(); ++rowIndex)
    {
      PrintRow(rowIndex);
      if (rowIndex == 0 || rowIndex == Cells.size() - 1)
        PrintBorderLine();
    }
  }

private:
  void PrintBorderLine()
  {
    auto pos = Out.GetCurPos();
    Out << "+";
    for (size_t i = 0; i < MaxColumns; ++i)
      Out << Pad("", '-', ColumnWidths[i] + 2) << "+";
    Out.NextLine(pos.X);
  }

  void PrintRow(size_t rowIndex)
  {
    auto startPos = Out.GetCurPos();
    for (size_t r{}; r < RowHeights[rowIndex]; ++r)
    {
      auto pos = Out.GetCurPos();
      Out << "|";
      for (size_t c{}; c < MaxColumns; ++c)
      {
        pos.X += ColumnWidths[c] + 3; // +1 due to outputting first '|'
        Out.SetCurPos(pos);
        Out << "|";
      }
      Out.NextLine(startPos.X);
    }

    auto pos = Position { startPos.X + 2, startPos.Y };
    const auto& row = Cells[rowIndex];
    for (size_t c{}; c < row.size(); ++c)
    {
      Out.WriteScreen(row[c], pos);
      pos.X += ColumnWidths[c] + 3;
    }
    Out.SetCurPos({startPos.X, startPos.Y + RowHeights[rowIndex]});
  }
};

void ScreenImpl::DrawTable(const std::vector<std::vector<ScreenImpl>>& cells)
{
  TablePrinter printer{*this, cells};
  printer.Print();
}

std::shared_ptr<IScreen> IScreen::Create()
{
  return std::make_shared<ScreenImpl>();
}
