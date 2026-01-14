#include "MasterDetailImpl.h"

using namespace Model;

void MasterDetailImpl::AddNavItem(const std::string& label, const std::string& url, bool active)
{
  NavItems.push_back({label, url, active});
}

void MasterDetailImpl::SetDetail(const std::shared_ptr<IDrawable>& content)
{
  Detail = content;
}

void MasterDetailImpl::DrawAsText(IScreen& screen)
{
  if (Detail)
    Detail->DrawAsText(screen);
}

void MasterDetailImpl::DrawAsHtml(std::ostream& o)
{
  o << R"(<style>
.master-detail { display: flex; gap: 20px; }
.master { min-width: 200px; border-right: 1px solid #ccc; padding-right: 20px; }
.master ul { list-style: none; padding: 0; margin: 0; }
.master li a { display: block; padding: 8px 12px; text-decoration: none; color: #333; }
.master li a:hover { background: #f0f0f0; }
.master li.active a { background: #e0e0e0; font-weight: bold; }
.detail { flex: 1; overflow: auto; }
</style>
)";

  o << R"(<div class="master-detail">)" << "\n";
  o << R"(<nav class="master">)" << "\n";
  o << R"(<ul>)" << "\n";
  for (const auto& item : NavItems)
  {
    o << "<li";
    if (item.Active)
      o << R"( class="active")";
    o << R"(><a href=")" << item.Url << R"(">)" << item.Label << R"(</a></li>)" << "\n";
  }
  o << R"(</ul>)" << "\n";
  o << R"(</nav>)" << "\n";

  o << R"(<main class="detail">)" << "\n";
  if (Detail)
    Detail->DrawAsHtml(o);
  o << R"(</main>)" << "\n";
  o << R"(</div>)" << "\n";
}

std::shared_ptr<IMasterDetail> IMasterDetail::Create()
{
  return std::make_shared<MasterDetailImpl>();
}
