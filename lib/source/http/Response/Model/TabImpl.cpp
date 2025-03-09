#include "ScreenImpl.h"
#include "TabImpl.h"

using namespace Model;

TabManagerImpl& TabManagerImpl::AddTab(const std::string& title, const std::shared_ptr<IDrawable>& content)
{
  TabManagerItem item {
    .Title = title
    , .Content = content
  };

  Items.push_back(std::move(item));
  return *this;
}

void TabManagerImpl::DrawAsText(IScreen& screen)
{
}

void TabManagerImpl::DrawAsHtml(std::ostream& o)
{
  // Generate unique identifier for this tab manager instance
  std::string managerId = std::to_string(reinterpret_cast<uintptr_t>(this));
  std::string functionName = "openTab_" + managerId;
  std::string tabClass = "tab_" + managerId;
  std::string tabContentClass = "tabcontent_" + managerId;
  std::string tabLinksClass = "tablinks_" + managerId;

  // Tab wrapper to ensure alignment
  o << "<div class=\"tab-wrapper\">\n";

  // Tab navigation buttons
  o << "<div class=\"tab " << tabClass << "\">\n";
  for (size_t i = 0; i < Items.size(); ++i)
  {
    const auto& item = Items[i];
    std::string tabId = "tab_" + managerId + "_" + std::to_string(reinterpret_cast<uintptr_t>(item.Content.get()));
    o << "  <button class=\"tablinks " << tabLinksClass << "\" onclick=\"" << functionName
      << "(event, '" << tabId << "')\">" << item.Title << "</button>\n";
  }
  o << "</div>\n\n";

  // Tab content sections
  for (size_t i = 0; i < Items.size(); ++i)
  {
    const auto& item = Items[i];
    std::string tabId = "tab_" + managerId + "_" + std::to_string(reinterpret_cast<uintptr_t>(item.Content.get()));
    o << "<div id=\"" << tabId << "\" class=\"tabcontent " << tabContentClass << "\">\n";
    if (item.Content)
    {
      item.Content->DrawAsHtml(o);
    }
    o << "</div>\n";
  }

  // Close tab wrapper
  o << "</div>\n";

  // JavaScript for tab functionality
  o << "<script>\n";
  o << "function " << functionName << "(evt, tabName) {\n";
  o << "  var i, tabcontent, tablinks;\n";
  o << "  tabcontent = document.getElementsByClassName(\"" << tabContentClass << "\");\n";
  o << "  for (i = 0; i < tabcontent.length; i++) {\n";
  o << "    tabcontent[i].style.display = \"none\";\n";
  o << "  }\n";
  o << "  tablinks = document.getElementsByClassName(\"" << tabLinksClass << "\");\n";
  o << "  for (i = 0; i < tablinks.length; i++) {\n";
  o << "    tablinks[i].className = tablinks[i].className.replace(\" active\", \"\");\n";
  o << "  }\n";
  o << "  document.getElementById(tabName).style.display = \"block\";\n";
  o << "  evt.currentTarget.className += \" active\";\n";
  o << "}\n\n";

  // Auto-select the first tab on page load
  o << "// Auto-select the first tab on page load\n";
  o << "document.addEventListener('DOMContentLoaded', function() {\n";
  o << "  // Get the first tab button for this specific tab manager\n";
  o << "  var firstTab = document.getElementsByClassName(\"" << tabLinksClass << "\")[0];\n";
  o << "  if (firstTab) {\n";
  o << "    firstTab.click();\n";
  o << "  }\n";
  o << "});\n";
  o << "</script>\n";
}

std::shared_ptr<ITabManager> ITabManager::Create()
{
  return std::make_shared<TabManagerImpl>();
}
