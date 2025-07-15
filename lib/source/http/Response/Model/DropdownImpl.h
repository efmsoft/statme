#pragma once

#include <list>
#include <string>

#include <Statme/http/Response/Model/Dropdown.h>

namespace Model
{
  class DropdownImpl : public IDropdown
  {
    struct ItemInfo
    {
      std::string Name;
      std::string Url;
    };

    std::list<ItemInfo> Items;

  public:
    void AddItem(const std::string& name, const std::string& url) override;

    void DrawAsText(IScreen& screen) override;
    void DrawAsHtml(std::ostream& o) override;

  private:
    std::string Id() const;
  };
}
