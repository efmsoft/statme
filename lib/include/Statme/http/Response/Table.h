#pragma once

#include <memory>
#include <string>
#include <vector>

namespace HTTP
{
  namespace Response
  {
    typedef std::vector<std::string> StringArray;

    struct Row
    {
      bool Header;
      StringArray Columns;

      Row() : Header(false)
      {
      }
    };

    typedef std::shared_ptr<Row> RowPtr;
    typedef std::vector<RowPtr> RowArray;

    struct Table
    {
      RowArray Rows;

    public:
      Table()
      {
      }

      RowPtr AddRow(bool header = false)
      {
        RowPtr row = std::make_shared<Row>();
        row->Header = header;
        Rows.push_back(row);
        return row;
      }
    };

    typedef std::shared_ptr<Table> TablePtr;
    typedef std::vector<TablePtr> TableArray;
  }
}