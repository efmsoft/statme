// Parse() must be cheap: one block, no per-header allocations, nothing copied
// per header, and the lookups/copies around it must not allocate either. These
// tests parse real and representative header blocks (header_samples.h) and
// control the number of heap allocations, the bytes requested, the layout of
// the parsed text and the allocations of everything that is done with the
// result afterwards.
//
// The allocations are counted by replacing the global operator new/delete of
// this test binary. That sees the library when it is linked into the binary
// (static library, or a shared library on Linux, where the executable's
// operator new takes over). A Windows DLL has its own operator new, which is
// not seen; the fixture detects that and skips instead of passing vacuously.

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <Statme/http/Headers.h>

#include "header_samples.h"

using namespace HTTP::Header;
using HeaderSamples::Kind;
using HeaderSamples::Sample;

namespace
{
  std::atomic<bool> CountingEnabled{false};
  std::atomic<size_t> AllocationCount{0};
  std::atomic<size_t> DeallocationCount{0};
  std::atomic<size_t> AllocatedBytes{0};
}

void* operator new(size_t size)
{
  if (CountingEnabled.load(std::memory_order_relaxed))
  {
    AllocationCount.fetch_add(1, std::memory_order_relaxed);
    AllocatedBytes.fetch_add(size, std::memory_order_relaxed);
  }

  if (void* p = std::malloc(size ? size : 1))
  {
    return p;
  }

  throw std::bad_alloc();
}

void* operator new[](size_t size)
{
  return operator new(size);
}

void operator delete(void* p) noexcept
{
  if (p && CountingEnabled.load(std::memory_order_relaxed))
  {
    DeallocationCount.fetch_add(1, std::memory_order_relaxed);
  }

  std::free(p);
}

void operator delete[](void* p) noexcept
{
  operator delete(p);
}

void operator delete(void* p, size_t) noexcept
{
  operator delete(p);
}

void operator delete[](void* p, size_t) noexcept
{
  operator delete(p);
}

namespace
{
  // What the global counters saw between a scope's construction and Stop().
  struct Measured
  {
    size_t Allocations;
    size_t Deallocations;
    size_t Bytes;

    Measured() : Allocations(0), Deallocations(0), Bytes(0) {}
  };

  class AllocScope
  {
  public:
    AllocScope()
    {
      AllocationCount = 0;
      DeallocationCount = 0;
      AllocatedBytes = 0;
      CountingEnabled = true;
    }

    ~AllocScope()
    {
      CountingEnabled = false;
    }

    Measured Stop()
    {
      CountingEnabled = false;

      Measured m;
      m.Allocations = AllocationCount;
      m.Deallocations = DeallocationCount;
      m.Bytes = AllocatedBytes;
      return m;
    }
  };

  // ---- the budgets -------------------------------------------------------

  // Distinct names above which an index in front of the list is allowed (one
  // more allocation). Below it a scan of the list is as fast as a hash probe.
  constexpr size_t IndexAfterNames = 64;

  // Bytes that may be spent per header line beyond the text itself: two
  // string_views, the links and a hash. This is the whole per-header budget.
  constexpr size_t BytesPerLine = 64;

  // Alignment of the block and the like.
  constexpr size_t BlockSlack = 64;

  // What the index may take per name, if there is one.
  constexpr size_t IndexBytesPerName = 64;

  // Allocator granularity of a std::string's buffer.
  constexpr size_t StringSlack = 16;

  size_t SmallStringCapacity()
  {
    return std::string().capacity();
  }

  struct Facts
  {
    size_t Lines; // header lines, without the first line
    size_t Names; // distinct names

    Facts() : Lines(0), Names(0) {}
  };

  std::string LowerCopy(std::string s)
  {
    for (auto& c : s)
    {
      c = char(::tolower((unsigned char)c));
    }

    return s;
  }

  Facts Describe(const std::string& text)
  {
    Facts facts;
    std::set<std::string> names;

    size_t pos = text.find("\r\n");
    while (pos != std::string::npos)
    {
      pos += 2;

      const size_t end = text.find("\r\n", pos);
      if (end == std::string::npos || end == pos)
      {
        break;
      }

      ++facts.Lines;
      names.insert(LowerCopy(text.substr(pos, text.find(':', pos) - pos)));
      pos = end;
    }

    facts.Names = names.size();
    return facts;
  }

  // Everything one parse may take: its block, the std::string members that
  // do not fit the small-string buffer, an index for very many names.
  struct Budget
  {
    size_t Allocations;
    size_t Bytes;

    Budget() : Allocations(0), Bytes(0) {}
  };

  Budget ParseBudget(const Sample& sample, const Facts& facts, size_t headerSize, size_t heapStringBytes, size_t heapStrings)
  {
    (void)sample;

    Budget budget;
    budget.Allocations = 1 + heapStrings + (facts.Names > IndexAfterNames ? 1 : 0);
    budget.Bytes = headerSize + 1 + facts.Lines * BytesPerLine + BlockSlack
      + heapStringBytes + heapStrings * StringSlack
      + (facts.Names > IndexAfterNames ? facts.Names * IndexBytesPerName : 0);

    return budget;
  }

  // The heap-allocated std::string members of a parsed object.
  void HeapStrings(const Headers& h, size_t& count, size_t& bytes)
  {
    count = 0;
    bytes = 0;

    if (h.ReqRes.size() > SmallStringCapacity())
    {
      ++count;
      bytes += h.ReqRes.size() + 1;
    }

    if (h.Body.size() > SmallStringCapacity())
    {
      ++count;
      bytes += h.Body.size() + 1;
    }
  }

  bool Parses(Headers& h, const Sample& sample)
  {
    return h.Parse(sample.Text.data(), sample.Text.size(), Verification::NotStrict) == HEADER_ERROR::NONE;
  }

  class HeaderAllocations : public ::testing::Test
  {
  protected:
    void SetUp() override
    {
#if defined(_MSC_VER) && defined(_ITERATOR_DEBUG_LEVEL) && _ITERATOR_DEBUG_LEVEL != 0
      // A debug STL allocates a proxy object per container: every number below would be wrong.
      GTEST_SKIP() << "allocation numbers are only meaningful with _ITERATOR_DEBUG_LEVEL=0 (release)";
#endif

      // The probe allocates for sure (an arena); if nothing is counted, the
      // library has its own operator new and none of the numbers below mean
      // anything.
      AllocScope scope;
      {
        Headers h(false);
        h.AddHeader("X-Calibration", std::string(64, 'v'));
      }

      if (scope.Stop().Allocations == 0)
      {
        GTEST_SKIP() << "allocations of the library are not visible to this binary";
      }
    }
  };

  struct ParseRun
  {
    Measured M;
    size_t HeaderSize;
    size_t HeapStringCount;
    size_t HeapStringBytes;
    size_t Fields;
    bool Ok;

    ParseRun() : HeaderSize(0), HeapStringCount(0), HeapStringBytes(0), Fields(0), Ok(false) {}
  };

  // Creates, parses and destroys an object of the sample's own type inside
  // one scope: a leak or a late free shows as unequal counters.
  ParseRun RunParse(const Sample& sample)
  {
    ParseRun run;
    AllocScope scope;

    switch (sample.Type)
    {
    case Kind::Response:
    {
      ResHeaders h(sample.LowerCase);
      run.Ok = Parses(h, sample);
      run.HeaderSize = h.Size;
      run.Fields = h.Header.size();
      HeapStrings(h, run.HeapStringCount, run.HeapStringBytes);
      if (h.Reason.size() > SmallStringCapacity())
      {
        ++run.HeapStringCount;
        run.HeapStringBytes += h.Reason.size() + 1;
      }
      break;
    }

    case Kind::Request:
    {
      ReqHeaders h(sample.LowerCase);
      run.Ok = Parses(h, sample);
      run.HeaderSize = h.Size;
      run.Fields = h.Header.size();
      HeapStrings(h, run.HeapStringCount, run.HeapStringBytes);
      if (h.Uri.size() > SmallStringCapacity())
      {
        ++run.HeapStringCount;
        run.HeapStringBytes += h.Uri.size() + 1;
      }
      break;
    }

    default:
    {
      Headers h(sample.LowerCase);
      run.Ok = Parses(h, sample);
      run.HeaderSize = h.Size;
      run.Fields = h.Header.size();
      HeapStrings(h, run.HeapStringCount, run.HeapStringBytes);
      break;
    }
    }

    run.M = scope.Stop();
    return run;
  }
}

// One block per parse: the copy of the received text with the nodes in front
// of it. Nothing per header, and an index only for very many names.
TEST_F(HeaderAllocations, ParseTakesOneBlock)
{
  for (const auto& sample : HeaderSamples::All())
  {
    SCOPED_TRACE(sample.Name);

    const Facts facts = Describe(sample.Text);
    const ParseRun run = RunParse(sample);
    ASSERT_TRUE(run.Ok);
    ASSERT_GT(run.Fields, 0u);

    const Budget budget = ParseBudget(sample, facts, run.HeaderSize, run.HeapStringBytes, run.HeapStringCount);

    EXPECT_LE(run.M.Allocations, budget.Allocations)
      << "lines=" << facts.Lines << " names=" << facts.Names
      << " heap strings=" << run.HeapStringCount;

    // Everything that was allocated was freed, inside the scope.
    EXPECT_EQ(run.M.Allocations, run.M.Deallocations);
  }
}

// The block holds the text once and a small fixed amount per header; no
// other storage grows with the number of headers.
TEST_F(HeaderAllocations, ParseFootprintIsTheTextPlusAFixedAmountPerHeader)
{
  for (const auto& sample : HeaderSamples::All())
  {
    SCOPED_TRACE(sample.Name);

    const Facts facts = Describe(sample.Text);
    const ParseRun run = RunParse(sample);
    ASSERT_TRUE(run.Ok);

    const Budget budget = ParseBudget(sample, facts, run.HeaderSize, run.HeapStringBytes, run.HeapStringCount);

    EXPECT_LE(run.M.Bytes, budget.Bytes)
      << "header text=" << run.HeaderSize << " lines=" << facts.Lines
      << " requested=" << run.M.Bytes
      << " overhead per line=" << (run.M.Bytes > run.HeaderSize ? (run.M.Bytes - run.HeaderSize) / std::max<size_t>(facts.Lines, 1) : 0)
      << " (budget " << BytesPerLine << ")";
  }
}

// Keys and values are views into one copy of the received block, which is
// not the caller's buffer, and the caller's buffer is left as it was.
TEST_F(HeaderAllocations, ParsedTextIsOneCopyOfTheBlockAndTheSourceIsUntouched)
{
  for (const auto& sample : HeaderSamples::All())
  {
    SCOPED_TRACE(sample.Name);

    const std::string source = sample.Text;
    std::string buffer = sample.Text;

    Headers h(sample.LowerCase);
    ASSERT_EQ(h.Parse(buffer.data(), buffer.size(), Verification::NotStrict), HEADER_ERROR::NONE);

    EXPECT_EQ(buffer, source) << "Parse wrote into the caller's buffer";

    const char* low = nullptr;
    const char* high = nullptr;
    auto note = [&low, &high, &buffer](std::string_view s)
    {
      EXPECT_EQ(s.data()[s.size()], '\0') << s;

      if (!low || s.data() < low)
      {
        low = s.data();
      }

      if (!high || s.data() + s.size() > high)
      {
        high = s.data() + s.size();
      }

      EXPECT_FALSE(s.data() >= buffer.data() && s.data() < buffer.data() + buffer.size())
        << "a view points into the caller's buffer: " << s;
    };

    for (const auto& field : h.Header)
    {
      note(field.Key);

      for (std::string_view value : field.Values)
      {
        note(value);
      }
    }

    ASSERT_NE(low, nullptr);

    // All the text lies in a window the size of the header block: nothing was
    // copied anywhere else.
    EXPECT_LE(size_t(high - low), h.Size + 1);
  }
}

// A copy is one block as well.
TEST_F(HeaderAllocations, CopyTakesOneBlock)
{
  for (const auto& sample : HeaderSamples::All())
  {
    SCOPED_TRACE(sample.Name);

    const Facts facts = Describe(sample.Text);

    Headers source(sample.LowerCase);
    ASSERT_TRUE(Parses(source, sample));

    size_t heapStrings = 0;
    size_t heapStringBytes = 0;
    HeapStrings(source, heapStrings, heapStringBytes);

    Measured m;
    {
      AllocScope scope;
      {
        Headers copy(source);
        EXPECT_EQ(copy.Header.size(), source.Header.size());
      }
      m = scope.Stop();
    }

    const size_t allowed = 1 + heapStrings + (facts.Names > IndexAfterNames ? 1 : 0);
    EXPECT_LE(m.Allocations, allowed);
    EXPECT_EQ(m.Allocations, m.Deallocations);
    EXPECT_LE(m.Bytes, source.Size + 1 + facts.Lines * BytesPerLine + BlockSlack + heapStringBytes + heapStrings * StringSlack
      + (facts.Names > IndexAfterNames ? facts.Names * IndexBytesPerName : 0));
  }
}

// Looking things up and walking the result costs no allocation at all.
TEST_F(HeaderAllocations, LookupsAndIterationDoNotAllocate)
{
  for (const auto& sample : HeaderSamples::All())
  {
    SCOPED_TRACE(sample.Name);

    Headers h(sample.LowerCase);
    ASSERT_TRUE(Parses(h, sample));

    // Spelled the way a rule would: as stored, lowercase, uppercase.
    std::vector<std::string> names;
    for (const auto& field : h.Header)
    {
      const std::string key(field.Key);
      names.push_back(key);
      names.push_back(LowerCopy(key));

      std::string upper = key;
      for (auto& c : upper)
      {
        c = char(::toupper((unsigned char)c));
      }
      names.push_back(upper);
    }
    names.push_back("x-no-such-header-at-all");

    size_t found = 0;
    size_t valueBytes = 0;

    Measured m;
    {
      AllocScope scope;

      for (const auto& name : names)
      {
        if (h.HasHeader(name))
        {
          ++found;
        }

        auto it = h.FindHeader(name);
        if (it != h.Header.end())
        {
          for (std::string_view value : it->Values)
          {
            valueBytes += value.size();
          }
        }
      }

      for (const auto& field : h.Header)
      {
        valueBytes += field.Key.size();
      }

      m = scope.Stop();
    }

    EXPECT_EQ(m.Allocations, 0u);
    EXPECT_GE(found, h.Header.size() * 3);
    EXPECT_GT(valueBytes, 0u);
  }
}

// Headers added after the parse go into what is left of the block or one
// arena, not one allocation each.
TEST_F(HeaderAllocations, HeadersAddedAfterTheParseDoNotAllocateOneByOne)
{
  for (const auto& sample : HeaderSamples::All())
  {
    SCOPED_TRACE(sample.Name);

    Headers h(sample.LowerCase);
    ASSERT_TRUE(Parses(h, sample));

    std::vector<std::string> names;
    std::vector<std::string> values;
    for (int i = 0; i < 16; ++i)
    {
      names.push_back("X-Added-By-Policy-" + std::to_string(i));
      values.push_back("added-value-" + std::to_string(i) + "-with-some-more-text");
    }

    Measured m;
    {
      AllocScope scope;

      for (size_t i = 0; i < names.size(); ++i)
      {
        h.AddHeader(names[i], values[i]);
      }

      h.SetHeader(names[0], "replaced");
      h.AddHeader("Set-Cookie", "one=1");
      h.AddHeader("Set-Cookie", "two=2");
      h.DeleteHeader(names[1]);

      m = scope.Stop();
    }

    // At most one arena for all of it (plus the index, if the parse made one
    // and the count grew past the threshold with these additions).
    EXPECT_LE(m.Allocations, 2u);
    EXPECT_EQ(h.FindHeader(names[0])->Values.front(), "replaced");
  }
}

// Not asserted: what the numbers are, for the record.
TEST_F(HeaderAllocations, ReportParseMetrics)
{
  std::printf("\n  %-30s %7s %6s %6s | %6s %8s %9s | %9s %9s\n"
    , "sample", "text B", "lines", "names", "allocs", "bytes", "B/line+", "parse ns", "copy ns");

  for (const auto& sample : HeaderSamples::All())
  {
    const Facts facts = Describe(sample.Text);
    const ParseRun run = RunParse(sample);

    Headers h(sample.LowerCase);
    Parses(h, sample);

    const int iterations = 2000;
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i)
    {
      Headers p(sample.LowerCase);
      Parses(p, sample);
    }
    auto parsed = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i)
    {
      Headers c(h);
    }
    auto copied = std::chrono::steady_clock::now();

    auto ns = [iterations](std::chrono::steady_clock::time_point a, std::chrono::steady_clock::time_point b)
    {
      return double(std::chrono::duration_cast<std::chrono::nanoseconds>(b - a).count()) / iterations;
    };

    std::printf("  %-30s %7zu %6zu %6zu | %6zu %8zu %9.1f | %9.0f %9.0f\n"
      , sample.Name.c_str()
      , run.HeaderSize
      , facts.Lines
      , facts.Names
      , run.M.Allocations
      , run.M.Bytes
      , run.M.Bytes > run.HeaderSize ? double(run.M.Bytes - run.HeaderSize) / double(std::max<size_t>(facts.Lines, 1)) : 0.0
      , ns(start, parsed)
      , ns(parsed, copied));
  }

  std::printf("\n");
}
