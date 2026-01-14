#pragma once

#include <vector>

#include <Statme/http/Response/Model/MasterDetail.h>

namespace Model
{
  struct NavItem
  {
    std::string Label;
    std::string Url;
    bool Active;
  };

  class MasterDetailImpl : public IMasterDetail
  {
    std::vector<NavItem> NavItems;
    std::shared_ptr<IDrawable> Detail;

  public:
    void AddNavItem(const std::string& label, const std::string& url, bool active) override;
    void SetDetail(const std::shared_ptr<IDrawable>& content) override;

    void DrawAsText(IScreen& screen) override;
    void DrawAsHtml(std::ostream& o) override;
  };
}
