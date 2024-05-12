#pragma once

#include <ostream>
#include <string_view>
#include <vector>

#include <Statme/http/Response/Model/Screen.h>

namespace Model
{
  struct Char
  {
    char C{};
  };

  class ScreenImpl : public IScreen
  {
    Position CurPos;
    using Line = std::vector<Char>;
    std::vector<Line> Screen;

  public:
    Position GetCurPos();
    void SetCurPos(Position p);
    void NextLine(size_t x);

    size_t GetHeight() const;
    size_t GetMaxWidth() const;
    const Line& GetLine(size_t y) const;

    void Write(std::string_view str);
    void WriteScreen(const ScreenImpl& src, Position p);
    ScreenImpl& operator<<(std::string_view str);
    void DrawTable(const std::vector<std::vector<ScreenImpl>>& cells);

    void Dump(std::ostream& os, Position leftTopPos = {0, 0}, Position rightBottomPos = {(size_t) -1, (size_t)-1}) override;

  private:
    void EnsureScreenHeight(size_t y);
    Line& EnsureLineWidth(Position p);
  };
}
