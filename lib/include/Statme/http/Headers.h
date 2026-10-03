#pragma once

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <list>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <Statme/http/Find2CRLF.h>
#include <Statme/http/StreamData.h>
#include <Statme/http/Version.h>
#include <Statme/Macros.h>

namespace HTTP
{
  namespace Header
  {
    typedef std::vector<std::string> StringArray;
    typedef std::shared_ptr<StringArray> StringArrayPtr;

    struct Headers;

    // One value of a header field. Text points into a memory block owned by
    // the Headers object the value belongs to and is always NUL-terminated
    // (Text.data()[Text.size()] == 0), so it can be shown by a debugger and
    // passed to C APIs as is. Nothing here is ever allocated per value: the
    // node itself and its text are carved out of the same blocks.
    struct FieldValue
    {
      std::string_view Text;
      FieldValue* Next;

      FieldValue() : Next(nullptr) {}
    };

    // The values of one field, in the order they were added. Looks like the
    // std::vector<std::string> it replaces (size/empty/[]/front/range-for),
    // but yields std::string_view.
    class ValueList
    {
    public:
      class const_iterator
      {
      public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::string_view;
        using difference_type = std::ptrdiff_t;
        using pointer = const std::string_view*;
        using reference = const std::string_view&;

        const_iterator() : Node(nullptr) {}
        explicit const_iterator(const FieldValue* node) : Node(node) {}

        reference operator*() const { return Node->Text; }
        pointer operator->() const { return &Node->Text; }

        const_iterator& operator++() { Node = Node->Next; return *this; }
        const_iterator operator++(int) { const_iterator t(*this); Node = Node->Next; return t; }

        bool operator==(const const_iterator& other) const { return Node == other.Node; }
        bool operator!=(const const_iterator& other) const { return Node != other.Node; }

      private:
        const FieldValue* Node;
      };

      typedef const_iterator iterator;

      ValueList() : First(nullptr), Last(nullptr), Count(0) {}

      bool empty() const { return First == nullptr; }
      size_t size() const { return Count; }

      const std::string_view& front() const { return First->Text; }
      const std::string_view& back() const { return Last->Text; }

      // O(n) -- a field rarely has more than one value.
      const std::string_view& operator[](size_t index) const
      {
        const FieldValue* node = First;
        while (index--)
        {
          node = node->Next;
        }

        return node->Text;
      }

      const_iterator begin() const { return const_iterator(First); }
      const_iterator end() const { return const_iterator(nullptr); }

    private:
      friend struct Headers;

      FieldValue* First;
      FieldValue* Last;
      size_t Count;

      void Append(FieldValue* node)
      {
        if (Last)
        {
          Last->Next = node;
        }
        else
        {
          First = node;
        }

        Last = node;
        ++Count;
      }
    };

    class FieldList;

    // One header field (all the lines of the message that carry the same
    // name). Key is the name as stored: lowercased if the owning Headers was
    // created with lowerCase, wire case otherwise. NUL-terminated like the
    // values. A Field lives exactly as long as its Headers object and never
    // moves, so pointers/iterators to it stay valid across AddHeader/
    // SetHeader of other fields (DeleteHeader of this very field ends it).
    struct Field
    {
      std::string_view Key;
      ValueList Values;

      Field() : Next(nullptr), Prev(nullptr), Hash(0), Capacity(0) {}

    private:
      friend struct Headers;
      friend class FieldList;

      Field* Next;
      Field* Prev;
      uint32_t Hash; // case-insensitive hash of Key

      // Room (without the NUL) in the buffer holding the text of the first
      // value. Text.size() is only how much of it is in use right now; keeping
      // the room lets SetHeader reuse the buffer after the value was shrunk.
      uint32_t Capacity;
    };

    enum class Verification
    {
      NotStrict,
      Strict
    };

    // The fields of a Headers object in the order of their first appearance.
    // Read-only from outside: the fields are created by Headers itself.
    class FieldList
    {
    public:
      class const_iterator
      {
      public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = Field;
        using difference_type = std::ptrdiff_t;
        using pointer = const Field*;
        using reference = const Field&;

        const_iterator() : Node(nullptr) {}
        explicit const_iterator(const Field* field) : Node(field) {}

        reference operator*() const { return *Node; }
        pointer operator->() const { return Node; }

        const_iterator& operator++() { Node = NextOf(Node); return *this; }
        const_iterator operator++(int) { const_iterator t(*this); Node = NextOf(Node); return t; }

        bool operator==(const const_iterator& other) const { return Node == other.Node; }
        bool operator!=(const const_iterator& other) const { return Node != other.Node; }

      private:
        const Field* Node;
      };

      typedef const_iterator iterator;

      FieldList() : First(nullptr), Last(nullptr), Count(0) {}

      // The fields live in blocks owned by their Headers object; a list
      // cannot be duplicated on its own, copy the Headers instead.
      FieldList(const FieldList&) = delete;
      FieldList& operator=(const FieldList&) = delete;

      bool empty() const { return First == nullptr; }
      size_t size() const { return Count; }

      const Field& front() const { return *First; }
      const Field& back() const { return *Last; }

      const_iterator begin() const { return const_iterator(First); }
      const_iterator end() const { return const_iterator(nullptr); }

    private:
      friend struct Headers;

      Field* First;
      Field* Last;
      size_t Count;

      static const Field* NextOf(const Field* field) { return field->Next; }
    };

    // Header fields are not allocated one by one. Parse() copies the received
    // block once, puts a NUL after every key and every value inside that copy
    // and builds the Field/FieldValue nodes in the same allocation, so a
    // parsed message costs one allocation however many headers it has.
    // Whatever is added afterwards (AddHeader/SetHeader) goes into 4K arenas
    // allocated on demand. Blocks never move and are only released by
    // Clear()/Parse()/destructor, which is why Key/Text views, Field
    // pointers and FieldList iterators stay valid until then. A view of a
    // value is invalidated by SetHeader/DeleteHeader of its own field.
    struct Headers
    {
      size_t Size;
      std::string ReqRes;
      FieldList Header;
      std::string Body;

      bool LowerCase;
      bool MixedLineEndings;
      int LineEnding; // \n or \r\n

    public:
      STATMELNK Headers(bool lowerCase);

      // A copy owns its blocks: the text is duplicated into a single new
      // block, nothing is shared with the source.
      STATMELNK Headers(const Headers& other);
      STATMELNK Headers& operator=(const Headers& other);

      STATMELNK virtual ~Headers();

      STATMELNK virtual HEADER_ERROR Parse(const StreamData& data, Verification type);
      STATMELNK virtual HEADER_ERROR Parse(const char* data, size_t length, Verification type);
      STATMELNK std::string Reparse();

      STATMELNK bool Empty() const;
      STATMELNK void Clear();

      STATMELNK std::string ToString(
        const char* indent = ""
        , bool dropTermination = false
        , bool appendBody = true
      ) const;

      STATMELNK std::string BodyToString(
        const char* indent = ""
        , bool InitialLF = false
        , size_t limit = 512
        , size_t split = 120
      ) const;

      STATMELNK StringArrayPtr GetHeader(
        std::string_view field
        , bool lowercase = false
        , const char* splitter = ","
      ) const;

      STATMELNK void DeleteHeader(std::string_view field);
      STATMELNK void AddHeader(std::string_view field, std::string_view value);
      STATMELNK void SetHeader(std::string_view field, std::string_view value);
      STATMELNK FieldList::const_iterator FindHeader(std::string_view field) const;
      STATMELNK bool HasHeader(std::string_view field) const;

      STATMELNK std::string GetFirstValue(
        std::string_view field
        , bool lowercase = true
        , const char* splitter = nullptr
        , const std::string& def = std::string()
      ) const;

      STATMELNK void CopyTo(Headers& to) const;

      STATMELNK static bool Complete(const std::vector<char>& data);
      STATMELNK static bool Complete(const char* data, size_t length);
      STATMELNK static size_t SizeOfHeader(const char* data, size_t length);

      STATMELNK static const char* sstrtok(
        char* str,
        const char* delimiters,
        char** context
      );

      STATMELNK static bool Printable(std::string_view str);
      STATMELNK static size_t CalcPrintable(std::string_view str);

      STATMELNK static bool ValidKey(std::string_view key);
      STATMELNK static bool ValidValue(std::string_view val);
      STATMELNK static bool ValidHeaderBuf(std::string_view buf);

    private:
      static void PushValue(StringArrayPtr arr, const std::string& value, bool lowrcase);
      int SenseType(char* buffer);
      char* ExtractHeaderLine(
        char* buffer
        , char*& context
        , int& type
      );

      struct Block;

      // More fields than this get a hash table in front of the list; up to
      // this many a scan of the list is faster than hashing.
      static constexpr size_t IndexThreshold = 16;

      // Size of the arena allocated when AddHeader/SetHeader run out of room.
      static constexpr size_t ArenaSize = 4096;

      // Slack added to every Reserve() for the alignment of the two nodes
      // that can be carved after unaligned text.
      static constexpr size_t NodePadding = 16;

      Block* Blocks; // newest first, all released by ClearFields()

      // Free tail of the newest block that new nodes and text are carved
      // from. Only this object ever writes there: a copy gets blocks of its
      // own, so there is nothing to synchronize.
      char* Cur;
      char* End;

      // Open-addressing hash of Header, case-insensitive on the key. Empty
      // while Header is small; when it is empty FindHeader scans the list,
      // so it can be dropped at any time without breaking lookups.
      std::vector<Field*> Table;

      // Slots of Table that hold a deleted field (see DeleteHeader). They
      // stay occupied for probing until the table is rebuilt.
      size_t Tombstones;

      void ClearFields();
      void AssignFields(const Headers& other);

      char* NewBlock(size_t bytes);
      char* NewLayout(size_t nodeBytes, size_t textBytes);
      void Reserve(size_t bytes);
      void* TakeNode(size_t size);
      std::string_view StoreText(std::string_view text, bool lower);

      Field* Find(std::string_view key, uint32_t hash) const;
      Field* NewField(std::string_view storedKey, uint32_t hash);
      void AppendValue(Field* field, std::string_view storedValue);
      void LinkField(Field* field);
      void UnlinkField(Field* field);
      void AddNew(std::string_view field, uint32_t hash, std::string_view value);
      void AddStored(std::string_view storedKey, std::string_view storedValue);
      void IndexField(Field* field);
      void BuildTable();
    };

    struct ReqHeaders : public Headers
    {
      std::string Method;
      std::string Uri;
      Version Protocol;

    public:
      STATMELNK ReqHeaders(bool lowerCase = false);

      STATMELNK HEADER_ERROR Parse(const StreamData& data, Verification type) override;
      STATMELNK HEADER_ERROR Parse(const char* data, size_t length, Verification type) override;

      STATMELNK bool IsHeadRequest() const;
      STATMELNK void CopyTo(ReqHeaders& to) const;

      STATMELNK static HEADER_ERROR TryParse(std::string_view data);

    private:
      HEADER_ERROR ParseReqLine(Verification type);
    };

    struct ResHeaders : public Headers
    {
      Version Protocol;
      int Status;
      std::string Reason;

    public:
      STATMELNK ResHeaders(bool lowerCase = false);

      STATMELNK HEADER_ERROR Parse(const StreamData& data, Verification type) override;
      STATMELNK HEADER_ERROR Parse(const char* data, size_t length, Verification type) override;
      STATMELNK void CopyTo(ResHeaders& to) const;

    private:
      HEADER_ERROR ParseResLine(Verification type);
    };
  }
}

#define HEADERS_STR(h) h.ToString("  ", true, false).c_str()
#define BODY_STR(h) h.BodyToString("  ", true).c_str()
