#include <format>

#include "DropdownImpl.h"
#include "ScreenImpl.h"

using namespace Model;

std::string DropdownImpl::Id() const
{
  return std::to_string(reinterpret_cast<uint64_t>(this));
}

void DropdownImpl::AddItem(const std::string& name, const std::string& url)
{
  Items.emplace_back(ItemInfo { .Name = name, .Url = url });
}

void DropdownImpl::DrawAsText(IScreen& screen)
{
  // Text formatter is non-interactive, no need to draw buttons
}

void DropdownImpl::DrawAsHtml(std::ostream& o)
{
  o << std::format(R"a(<select id="dropdown_{0}" onchange="fetchContent_{0}()">)a", Id()) << std::endl;
  for (const auto& i : Items)
    
    o << std::format(R"(<option value="{}">{}</option>)", i.Url, i.Name) << std::endl;
  o << R"(</select>)" << std::endl;

  o << std::format(R"(<div class="dropdownresult" id="result_{}">Select an option to see content here...</div>)", Id()) << std::endl;

  o << std::format(R"(
    <script>
        async function fetchContent_{0}() {{
            const dropdown = document.getElementById('dropdown_{0}');
            const result = document.getElementById('result_{0}');
            
            if (!dropdown.value) {{
                result.innerHTML = 'Select an option to see content here...';
                return;
            }}

            try {{
                result.innerHTML = 'Loading...';
                const response = await fetch(dropdown.value);
                const content = await response.text();
                result.innerHTML = content;
            }} catch (error) {{
                result.innerHTML = `Error: ${{error.message}}`;
            }}
        }}
    </script>)"
    , Id()
  );
}

std::shared_ptr<IDropdown> IDropdown::Create()
{
  return std::make_shared<DropdownImpl>();
}
