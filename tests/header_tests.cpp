#include <gtest/gtest.h>

#include <algorithm>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include <Statme/http/Headers.h>

using namespace HTTP::Header;

TEST(header_tests, partial_first_line_success)
{
  EXPECT_EQ(ReqHeaders::TryParse("G"), HEADER_ERROR::NOT_COMPLETED);
  EXPECT_EQ(ReqHeaders::TryParse("GE"), HEADER_ERROR::NOT_COMPLETED);
  EXPECT_EQ(ReqHeaders::TryParse("GET"), HEADER_ERROR::NOT_COMPLETED);
  EXPECT_EQ(ReqHeaders::TryParse("GET "), HEADER_ERROR::NOT_COMPLETED);
  EXPECT_EQ(ReqHeaders::TryParse("GET /bla"), HEADER_ERROR::NOT_COMPLETED);
  EXPECT_EQ(ReqHeaders::TryParse("GET /bla ht"), HEADER_ERROR::NOT_COMPLETED);
  EXPECT_EQ(ReqHeaders::TryParse("GET /bla http/1.1"), HEADER_ERROR::NOT_COMPLETED);
  EXPECT_EQ(ReqHeaders::TryParse("P"), HEADER_ERROR::NOT_COMPLETED);
  EXPECT_EQ(ReqHeaders::TryParse("PO"), HEADER_ERROR::NOT_COMPLETED);
  EXPECT_EQ(ReqHeaders::TryParse("POS"), HEADER_ERROR::NOT_COMPLETED);
  EXPECT_EQ(ReqHeaders::TryParse("POST "), HEADER_ERROR::NOT_COMPLETED);
  EXPECT_EQ(ReqHeaders::TryParse("POST /bla"), HEADER_ERROR::NOT_COMPLETED);
  EXPECT_EQ(ReqHeaders::TryParse("POST /bla ht"), HEADER_ERROR::NOT_COMPLETED);
  EXPECT_EQ(ReqHeaders::TryParse("POST /bla http/1.1"), HEADER_ERROR::NOT_COMPLETED);
}

TEST(header_tests, partial_first_line_failure)
{
  EXPECT_NE(ReqHeaders::TryParse("BLA"), HEADER_ERROR::NOT_COMPLETED);
  EXPECT_NE(ReqHeaders::TryParse("PO /bla"), HEADER_ERROR::NOT_COMPLETED);
}

TEST(header_tests, partial_header_success)
{
  EXPECT_EQ(ReqHeaders::TryParse("POST /bla http/1.1\r\nContent-Type: application/json"), HEADER_ERROR::NOT_COMPLETED);
  EXPECT_EQ(ReqHeaders::TryParse("GET /bla http/3.0\r\nContent-Type: application/json"), HEADER_ERROR::NOT_COMPLETED);
}

TEST(header_tests, partial_header_failure)
{
  EXPECT_NE(ReqHeaders::TryParse("PO /bla http/1.1\r\nContent-Type: application/json"), HEADER_ERROR::NOT_COMPLETED);
  EXPECT_NE(ReqHeaders::TryParse("GET /bla http/1\r\nContent-Type: application/json"), HEADER_ERROR::NOT_COMPLETED);
  EXPECT_EQ(ReqHeaders::TryParse( "GET /bla http/3.0\r\nContent-Type: application/json"), HEADER_ERROR::NOT_COMPLETED);
}

TEST(header_tests, full_header_success)
{
  EXPECT_EQ(ReqHeaders::TryParse("POST /bla http/1.1\r\nContent-Type: application/json\r\n\r\n"), HEADER_ERROR::NONE);
  EXPECT_EQ(ReqHeaders::TryParse("POST /bla http/1.1\r\nContent-Type: application/json\n\n"), HEADER_ERROR::NONE);
  EXPECT_EQ(ReqHeaders::TryParse("GET /bla http/3.0\r\nContent-Type: application/json\r\n\r\n"), HEADER_ERROR::NONE);
}

TEST(header_tests, full_header_failure)
{
  EXPECT_NE(ReqHeaders::TryParse("POST /bla http/1.1\r\nContent-Type: application/json\t\t"), HEADER_ERROR::NONE);
  EXPECT_NE(ReqHeaders::TryParse("GET /bla unknown\r\nContent-Type: application/json\r\n\r\n"), HEADER_ERROR::NONE);
}

TEST(header_tests, mix_sepatators)
{
  std::string data = "HTTP/1.1 200 OK\r\n  Content-Type: text/html;CHARset=utf-8\nConnection:close  \nX-Frame-Options: SAMEORIGIN  \nX-Content-Type-Options: nosniff  \nX-XSS-Protection: 1; mode=block\n\n\n<!DOCTYPE html>";

  ResHeaders h;
  auto e = h.Parse(data.c_str(), data.length(), Verification::NotStrict);
  EXPECT_EQ(e, HEADER_ERROR::NONE);

  h.AddHeader("Content-Length", "1234");
  auto d2 = h.ToString();

  std::string data2 = "HTTP/1.1 200 OK\r\nContent-Type: text/html;CHARset=utf-8\r\nConnection: close\r\nX-Frame-Options: SAMEORIGIN\r\nX-Content-Type-Options: nosniff\r\nX-XSS-Protection: 1; mode=block\r\nContent-Length: 1234\r\n\r\n\n<!DOCTYPE html>";
  EXPECT_EQ(d2, data2);
}

namespace
{
  HEADER_ERROR ParseText(Headers& h, const std::string& text, Verification type = Verification::NotStrict)
  {
    return h.Parse(text.c_str(), text.size(), type);
  }

  std::string Value(const Headers& h, const std::string& name, size_t index = 0)
  {
    auto it = h.FindHeader(name);
    if (it == h.Header.end() || index >= it->Values.size())
    {
      return "<missing>";
    }

    return std::string(it->Values[index]);
  }

  // Everything a Field/FieldValue points at is NUL-terminated.
  void ExpectTerminated(const Headers& h)
  {
    for (const auto& field : h.Header)
    {
      EXPECT_EQ(field.Key.data()[field.Key.size()], '\0') << field.Key;
      for (const auto& value : field.Values)
      {
        EXPECT_EQ(value.data()[value.size()], '\0') << field.Key << ": " << value;
      }
    }
  }
}

TEST(header_tests, parse_trims_blanks_and_terminates_text)
{
  Headers h(false);
  ASSERT_EQ(ParseText(h, "HTTP/1.1 200 OK\r\nContent-Type:   text/html  \r\nX-Empty:\r\nX-Blank:    \r\n  Spaced  :v\r\n\r\n"), HEADER_ERROR::NONE);

  EXPECT_EQ(h.ReqRes, "HTTP/1.1 200 OK");
  ASSERT_EQ(h.Header.size(), 4u);

  auto it = h.FindHeader("content-TYPE");
  ASSERT_NE(it, h.Header.end());
  EXPECT_EQ(it->Key, "Content-Type");
  ASSERT_EQ(it->Values.size(), 1u);
  EXPECT_EQ(it->Values[0], "text/html");

  EXPECT_EQ(Value(h, "x-empty"), "");
  EXPECT_EQ(Value(h, "x-blank"), "");
  EXPECT_EQ(Value(h, "spaced"), "v");
  EXPECT_EQ(h.FindHeader("Spaced")->Key, "Spaced");

  ExpectTerminated(h);
}

TEST(header_tests, parse_lowercase_keys_keeps_value_case)
{
  Headers h(true);
  ASSERT_EQ(ParseText(h, "GET / HTTP/1.1\r\nContent-TYPE: Text/HTML\r\nX-Mixed-Case: AbC\r\n\r\n"), HEADER_ERROR::NONE);

  ASSERT_EQ(h.Header.size(), 2u);
  EXPECT_EQ(h.Header.front().Key, "content-type");
  EXPECT_EQ(Value(h, "Content-Type"), "Text/HTML");
  EXPECT_EQ(Value(h, "X-MIXED-CASE"), "AbC");
  EXPECT_EQ(h.ToString(), "GET / HTTP/1.1\r\ncontent-type: Text/HTML\r\nx-mixed-case: AbC\r\n\r\n");
}

TEST(header_tests, repeated_names_are_grouped_by_first_appearance)
{
  Headers h(false);
  ASSERT_EQ(ParseText(h, "HTTP/1.1 200 OK\r\nSet-Cookie: a=1\r\nContent-Length: 5\r\nset-cookie: b=2\r\nSET-COOKIE: c=3\r\n\r\n"), HEADER_ERROR::NONE);

  ASSERT_EQ(h.Header.size(), 2u);
  auto it = h.FindHeader("set-cookie");
  ASSERT_NE(it, h.Header.end());
  EXPECT_EQ(it->Key, "Set-Cookie");
  ASSERT_EQ(it->Values.size(), 3u);
  EXPECT_EQ(it->Values[0], "a=1");
  EXPECT_EQ(it->Values[1], "b=2");
  EXPECT_EQ(it->Values[2], "c=3");
  EXPECT_EQ(it->Values.front(), "a=1");
  EXPECT_EQ(it->Values.back(), "c=3");

  EXPECT_EQ(h.ToString(), "HTTP/1.1 200 OK\r\nSet-Cookie: a=1\r\nSet-Cookie: b=2\r\nSet-Cookie: c=3\r\nContent-Length: 5\r\n\r\n");
}

TEST(header_tests, parse_errors_and_skipped_lines)
{
  Headers h(false);
  EXPECT_EQ(ParseText(h, "GET / HTTP/1.1\r\nNoColonHere\r\n\r\n"), HEADER_ERROR::INVALID);
  EXPECT_EQ(ParseText(h, "GET / HTTP/1.1\r\nBad Key: v\r\n\r\n", Verification::Strict), HEADER_ERROR::INVALID_CHAR);
  EXPECT_EQ(ParseText(h, "GET / HTTP/1.1\r\nGood: bad\x01value\r\n\r\n", Verification::Strict), HEADER_ERROR::INVALID_CHAR);
  EXPECT_EQ(ParseText(h, "GET / HTTP/1.1\r\n: v\r\n\r\n", Verification::Strict), HEADER_ERROR::EMPTY_KEY);

  // Not strict: a nameless line is skipped, the rest is kept.
  ASSERT_EQ(ParseText(h, "GET / HTTP/1.1\r\n: v\r\nA: 1\r\n\r\n"), HEADER_ERROR::NONE);
  EXPECT_EQ(h.Header.size(), 1u);
  EXPECT_EQ(Value(h, "a"), "1");
}

TEST(header_tests, parse_again_replaces_previous_content)
{
  Headers h(false);
  ASSERT_EQ(ParseText(h, "GET / HTTP/1.1\r\nA: 1\r\nB: 2\r\n\r\n"), HEADER_ERROR::NONE);
  h.AddHeader("C", "3");

  ASSERT_EQ(ParseText(h, "GET /x HTTP/1.1\r\nD: 4\r\n\r\n"), HEADER_ERROR::NONE);
  EXPECT_EQ(h.Header.size(), 1u);
  EXPECT_FALSE(h.HasHeader("a"));
  EXPECT_FALSE(h.HasHeader("c"));
  EXPECT_EQ(Value(h, "d"), "4");

  h.Clear();
  EXPECT_TRUE(h.Header.empty());
  EXPECT_TRUE(h.Empty());
  EXPECT_FALSE(h.HasHeader("d"));
}

TEST(header_tests, find_has_get_first_value)
{
  Headers h(false);
  ASSERT_EQ(ParseText(h, "HTTP/1.1 200 OK\r\nContent-Type: Text/HTML; Charset=UTF-8\r\nAccept: a, B ,c\r\n\r\n"), HEADER_ERROR::NONE);

  EXPECT_TRUE(h.HasHeader("CONTENT-type"));
  EXPECT_FALSE(h.HasHeader("content-typ"));
  EXPECT_FALSE(h.HasHeader("content-type2"));
  EXPECT_FALSE(h.HasHeader(""));
  EXPECT_EQ(h.FindHeader("missing"), h.Header.end());

  EXPECT_EQ(h.GetFirstValue("content-type"), "text/html; charset=utf-8");
  EXPECT_EQ(h.GetFirstValue("content-type", false), "Text/HTML; Charset=UTF-8");
  EXPECT_EQ(h.GetFirstValue("missing", true, nullptr, "dflt"), "dflt");

  auto split = h.GetHeader("accept", true);
  ASSERT_NE(split, nullptr);
  ASSERT_EQ(split->size(), 3u);
  EXPECT_EQ((*split)[0], "a");
  EXPECT_EQ((*split)[1], "b");
  EXPECT_EQ((*split)[2], "c");

  auto whole = h.GetHeader("accept", false, nullptr);
  ASSERT_NE(whole, nullptr);
  ASSERT_EQ(whole->size(), 1u);
  EXPECT_EQ((*whole)[0], "a, B ,c");

  EXPECT_EQ(h.GetHeader("missing"), nullptr);
}

TEST(header_tests, add_set_delete_after_parse)
{
  Headers h(false);
  ASSERT_EQ(ParseText(h, "HTTP/1.1 200 OK\r\nA: 1\r\nB: 2\r\n\r\n"), HEADER_ERROR::NONE);

  h.AddHeader("a", "1b");            // existing field, second value
  h.AddHeader("New-One", "n");       // new field after the parsed ones
  h.SetHeader("B", "two-and-longer-than-the-original");
  h.SetHeader("c", "3");
  EXPECT_EQ(h.ToString(), "HTTP/1.1 200 OK\r\nA: 1\r\nA: 1b\r\nB: two-and-longer-than-the-original\r\nNew-One: n\r\nc: 3\r\n\r\n");

  // SetHeader replaces every value, and keeps the key as first stored.
  h.SetHeader("A", "x");
  EXPECT_EQ(h.FindHeader("a")->Values.size(), 1u);
  EXPECT_EQ(h.FindHeader("a")->Key, "A");
  EXPECT_EQ(Value(h, "a"), "x");

  h.DeleteHeader("NEW-one");
  EXPECT_FALSE(h.HasHeader("new-one"));
  h.DeleteHeader("does-not-exist");
  EXPECT_EQ(h.Header.size(), 3u);

  // Deleted names can come back, at the end.
  h.AddHeader("New-One", "again");
  EXPECT_EQ(h.Header.back().Key, "New-One");
  EXPECT_EQ(Value(h, "new-one"), "again");

  // Delete first / last / middle keeps the chain intact.
  h.DeleteHeader("a");
  EXPECT_EQ(h.Header.front().Key, "B");
  h.DeleteHeader("new-one");
  EXPECT_EQ(h.Header.back().Key, "c");
  h.DeleteHeader("b");
  h.DeleteHeader("c");
  EXPECT_TRUE(h.Header.empty());
  EXPECT_EQ(h.Header.begin(), h.Header.end());

  ExpectTerminated(h);
}

TEST(header_tests, set_header_repeatedly_with_changing_length)
{
  Headers h(false);
  ASSERT_EQ(ParseText(h, "GET / HTTP/1.1\r\nX: 0123456789\r\n\r\n"), HEADER_ERROR::NONE);

  for (int i = 0; i < 2000; ++i)
  {
    std::string v(size_t(i % 40), char('a' + i % 26));
    h.SetHeader("x", v);
    ASSERT_EQ(Value(h, "X"), v) << i;
    ASSERT_EQ(h.FindHeader("x")->Values.size(), 1u);
  }

  ExpectTerminated(h);
}

TEST(header_tests, arguments_may_point_into_the_headers_themselves)
{
  Headers h(false);
  ASSERT_EQ(ParseText(h, "GET / HTTP/1.1\r\nA: first\r\nB: second\r\n\r\n"), HEADER_ERROR::NONE);

  const std::string_view a = h.FindHeader("a")->Values.front();
  h.AddHeader("a", a);
  h.SetHeader("c", a);
  h.SetHeader("b", a);
  h.SetHeader("b", h.FindHeader("b")->Values.front());
  h.AddHeader(h.Header.front().Key, "third");

  EXPECT_EQ(Value(h, "a", 0), "first");
  EXPECT_EQ(Value(h, "a", 1), "first");
  EXPECT_EQ(Value(h, "a", 2), "third");
  EXPECT_EQ(Value(h, "b"), "first");
  EXPECT_EQ(Value(h, "c"), "first");
}

TEST(header_tests, lowercase_mode_lowers_added_keys_only)
{
  Headers h(true);
  h.AddHeader("Content-Type", "Text/Plain");
  h.SetHeader("X-Custom", "MiXed");
  h.SetHeader("CONTENT-TYPE", "Other");

  ASSERT_EQ(h.Header.size(), 2u);
  EXPECT_EQ(h.Header.front().Key, "content-type");
  EXPECT_EQ(h.Header.back().Key, "x-custom");
  EXPECT_EQ(Value(h, "Content-Type"), "Other");
  EXPECT_EQ(Value(h, "x-custom"), "MiXed");
}

TEST(header_tests, existing_pointers_survive_further_adds)
{
  Headers h(false);
  ASSERT_EQ(ParseText(h, "GET / HTTP/1.1\r\nFirst: one\r\n\r\n"), HEADER_ERROR::NONE);

  const Field* first = &h.Header.front();
  const char* key = first->Key.data();
  const char* value = first->Values.front().data();

  h.AddHeader("Added", "a");
  const Field* added = h.FindHeader("added").operator->();
  const char* addedValue = added->Values.front().data();

  // Enough to go through many arenas, plus a value larger than one.
  for (int i = 0; i < 3000; ++i)
  {
    h.AddHeader("Filler-" + std::to_string(i), std::string(70, 'f'));
  }
  h.AddHeader("Big", std::string(20000, 'b'));

  EXPECT_EQ(&h.Header.front(), first);
  EXPECT_EQ(first->Key.data(), key);
  EXPECT_EQ(first->Values.front().data(), value);
  EXPECT_EQ(first->Key, "First");
  EXPECT_EQ(first->Values.front(), "one");
  EXPECT_EQ(h.FindHeader("added").operator->(), added);
  EXPECT_EQ(added->Values.front().data(), addedValue);
  EXPECT_EQ(added->Values.front(), "a");
  EXPECT_EQ(Value(h, "big").size(), 20000u);
  EXPECT_EQ(h.Header.size(), 3003u);

  ExpectTerminated(h);
}

TEST(header_tests, many_fields_use_the_hash_table_consistently)
{
  std::string text = "HTTP/1.1 200 OK\r\n";
  for (int i = 0; i < 1500; ++i)
  {
    text += "X-Header-" + std::to_string(i) + ": v" + std::to_string(i) + "\r\n";
  }
  // Repeats, in a different case, interleaved far from the first appearance.
  for (int i = 0; i < 1500; i += 7)
  {
    text += "x-HEADER-" + std::to_string(i) + ": again" + std::to_string(i) + "\r\n";
  }
  text += "\r\n";
  ASSERT_LT(text.size(), size_t(MAX_HEADER_SIZE));

  Headers h(false);
  ASSERT_EQ(ParseText(h, text), HEADER_ERROR::NONE);
  ASSERT_EQ(h.Header.size(), 1500u);

  auto check = [&h](int i, bool present)
  {
    const std::string name = "X-HEADER-" + std::to_string(i);
    EXPECT_EQ(h.HasHeader(name), present) << name;
    if (!present)
    {
      return;
    }

    auto it = h.FindHeader(name);
    ASSERT_NE(it, h.Header.end()) << name;
    EXPECT_EQ(it->Values[0], "v" + std::to_string(i));
    ASSERT_EQ(it->Values.size(), i % 7 == 0 ? 2u : 1u) << name;
    if (i % 7 == 0)
    {
      EXPECT_EQ(it->Values[1], "again" + std::to_string(i));
    }
  };

  for (int i = 0; i < 1500; ++i)
  {
    check(i, true);
  }

  EXPECT_FALSE(h.HasHeader("X-Header-1500"));
  EXPECT_FALSE(h.HasHeader("X-Header-"));

  // Deleting while the table exists, then lookups, adds and re-adds.
  for (int i = 0; i < 1500; i += 3)
  {
    h.DeleteHeader("x-header-" + std::to_string(i));
  }
  for (int i = 0; i < 1500; ++i)
  {
    check(i, i % 3 != 0);
  }

  for (int i = 0; i < 1500; i += 3)
  {
    h.AddHeader("X-Header-" + std::to_string(i), "back" + std::to_string(i));
  }
  for (int i = 0; i < 1500; i += 3)
  {
    EXPECT_EQ(Value(h, "x-header-" + std::to_string(i)), "back" + std::to_string(i));
  }

  EXPECT_EQ(h.Header.size(), 1500u);

  // The list and the table agree on what is there.
  size_t walked = 0;
  for (const auto& field : h.Header)
  {
    ++walked;
    EXPECT_NE(h.FindHeader(field.Key), h.Header.end());
  }
  EXPECT_EQ(walked, h.Header.size());

  // Dropping below the threshold goes back to scanning the list.
  while (h.Header.size() > 5)
  {
    h.DeleteHeader(h.Header.front().Key);
  }
  EXPECT_EQ(h.Header.size(), 5u);
  for (const auto& field : h.Header)
  {
    EXPECT_NE(h.FindHeader(field.Key), h.Header.end());
  }
}

TEST(header_tests, copy_owns_its_text)
{
  auto source = std::make_unique<Headers>(false);
  ASSERT_EQ(ParseText(*source, "HTTP/1.1 200 OK\r\nA: 1\r\nB: 2\r\nA: 3\r\n\r\nbody"), HEADER_ERROR::NONE);
  source->AddHeader("Added", "x");
  const std::string expected = source->ToString();

  Headers copy(*source);
  Headers assigned(true);
  assigned.AddHeader("Stale", "gone");
  assigned = *source;
  Headers viaCopyTo(false);
  source->CopyTo(viaCopyTo);

  // Mutating the source leaves the copies alone, and the other way round.
  source->SetHeader("A", "changed");
  source->DeleteHeader("B");
  source->AddHeader("More", "m");
  copy.AddHeader("OnlyInCopy", "c");

  EXPECT_EQ(assigned.ToString(), expected);
  EXPECT_EQ(viaCopyTo.ToString(), expected);
  EXPECT_FALSE(copy.HasHeader("more"));
  EXPECT_EQ(Value(copy, "a", 1), "3");
  EXPECT_EQ(Value(copy, "onlyincopy"), "c");
  EXPECT_FALSE(source->HasHeader("onlyincopy"));

  // Nothing of the copies lives in the source's memory.
  source.reset();
  EXPECT_EQ(assigned.ToString(), expected);
  EXPECT_EQ(viaCopyTo.ToString(), expected);
  EXPECT_EQ(Value(copy, "added"), "x");
  EXPECT_EQ(copy.Body, "body");
  EXPECT_EQ(assigned.LowerCase, false);
  ExpectTerminated(copy);
  ExpectTerminated(assigned);

  // Self assignment and an empty source.
  Headers& same = assigned;
  assigned = same;
  EXPECT_EQ(assigned.ToString(), expected);

  Headers empty(false);
  assigned = empty;
  EXPECT_TRUE(assigned.Header.empty());
  EXPECT_FALSE(assigned.HasHeader("a"));
}

TEST(header_tests, copied_big_header_set_keeps_lookups)
{
  Headers source(false);
  for (int i = 0; i < 400; ++i)
  {
    source.AddHeader("Key-" + std::to_string(i), "val-" + std::to_string(i));
  }

  Headers copy(source);
  ASSERT_EQ(copy.Header.size(), 400u);
  for (int i = 0; i < 400; ++i)
  {
    ASSERT_EQ(Value(copy, "KEY-" + std::to_string(i)), "val-" + std::to_string(i));
  }

  copy.DeleteHeader("key-5");
  EXPECT_TRUE(source.HasHeader("key-5"));
  EXPECT_FALSE(copy.HasHeader("key-5"));
}

TEST(header_tests, derived_copy_to_keeps_the_fields)
{
  ResHeaders res;
  std::string text = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nX-A: a\r\n\r\n";
  ASSERT_EQ(res.Parse(text.c_str(), text.size(), Verification::Strict), HEADER_ERROR::NONE);
  EXPECT_EQ(res.Status, 404);
  EXPECT_EQ(res.Reason, "Not Found");

  ResHeaders res2;
  res.CopyTo(res2);
  EXPECT_EQ(res2.Status, 404);
  EXPECT_EQ(res2.Reason, "Not Found");
  EXPECT_EQ(res2.ToString(), res.ToString());

  ReqHeaders req;
  std::string rtext = "POST /p HTTP/1.1\r\nHost: example.org\r\nX-B: b\r\n\r\n";
  ASSERT_EQ(req.Parse(rtext.c_str(), rtext.size(), Verification::Strict), HEADER_ERROR::NONE);

  ReqHeaders req2(req);
  ReqHeaders req3;
  req3 = req;
  EXPECT_EQ(req2.Method, "POST");
  EXPECT_EQ(req2.Uri, "/p");
  EXPECT_EQ(Value(req2, "host"), "example.org");
  EXPECT_EQ(Value(req3, "x-b"), "b");
}

TEST(header_tests, empty_and_null_views_are_accepted)
{
  Headers h(false);
  h.AddHeader("X", "abc");

  // A default-constructed string_view has a null data(); nothing may be
  // copied from it.
  h.SetHeader("X", std::string_view());
  EXPECT_EQ(Value(h, "x"), "");

  h.AddHeader("Y", std::string_view());
  h.SetHeader("W", std::string_view());
  h.AddHeader("X", std::string_view());

  EXPECT_EQ(Value(h, "y"), "");
  EXPECT_EQ(Value(h, "w"), "");
  EXPECT_EQ(h.FindHeader("x")->Values.size(), 2u);
  EXPECT_FALSE(h.HasHeader(std::string_view()));

  // Set replaces every value, however many there were.
  h.SetHeader("X", "back again");
  EXPECT_EQ(Value(h, "x"), "back again");
  EXPECT_EQ(h.FindHeader("x")->Values.size(), 1u);

  ExpectTerminated(h);
}

TEST(header_tests, set_header_reuses_the_room_of_a_shrunk_value)
{
  Headers h(false);
  h.AddHeader("X", "a");
  h.SetHeader("X", std::string(30000, 'b'));
  const char* room = h.FindHeader("x")->Values.front().data();

  // Long -> short -> long again must not take new memory every time.
  for (int i = 0; i < 1000; ++i)
  {
    h.SetHeader("X", "c");
    ASSERT_EQ(Value(h, "x"), "c");

    h.SetHeader("X", std::string(30000 - i % 7, 'd'));
    ASSERT_EQ(h.FindHeader("x")->Values.front().data(), room) << i;
  }

  EXPECT_EQ(Value(h, "x").size(), 30000u - 999 % 7);
  ExpectTerminated(h);
}

TEST(header_tests, parsed_value_room_is_reused_too)
{
  Headers h(false);
  ASSERT_EQ(ParseText(h, "GET / HTTP/1.1\r\nX: 0123456789abcdef\r\n\r\n"), HEADER_ERROR::NONE);
  const char* room = h.FindHeader("x")->Values.front().data();

  h.SetHeader("x", "short");
  h.SetHeader("x", "0123456789abcdef");
  EXPECT_EQ(h.FindHeader("x")->Values.front().data(), room);
  EXPECT_EQ(Value(h, "x"), "0123456789abcdef");

  // Longer than the room: moves, and the new room is kept from then on.
  h.SetHeader("x", "0123456789abcdef-and-more");
  const char* moved = h.FindHeader("x")->Values.front().data();
  EXPECT_NE(moved, room);
  h.SetHeader("x", "tiny");
  h.SetHeader("x", "0123456789abcdef-and-more");
  EXPECT_EQ(h.FindHeader("x")->Values.front().data(), moved);
}

TEST(header_tests, many_fields_with_several_values_parse_and_copy)
{
  // Every field has several values, so there are as many value nodes as
  // lines: the node area of a parsed or copied block has to hold all of them
  // whatever the size of a node is on the target.
  std::string text = "HTTP/1.1 200 OK\r\n";
  for (int round = 0; round < 4; ++round)
  {
    for (int i = 0; i < 40; ++i)
    {
      text += "Field-" + std::to_string(i) + ": v" + std::to_string(round) + "\r\n";
    }
  }
  text += "\r\n";

  Headers h(false);
  ASSERT_EQ(ParseText(h, text), HEADER_ERROR::NONE);
  ASSERT_EQ(h.Header.size(), 40u);
  for (const auto& field : h.Header)
  {
    ASSERT_EQ(field.Values.size(), 4u) << field.Key;
  }

  const std::string expected = h.ToString();

  Headers copy(h);
  EXPECT_EQ(copy.ToString(), expected);

  // Add into what is left of the blocks and copy again.
  for (int i = 0; i < 20; ++i)
  {
    copy.AddHeader("Field-" + std::to_string(i), "extra");
  }
  Headers copy2(copy);
  EXPECT_EQ(copy2.ToString(), copy.ToString());
  EXPECT_EQ(Value(copy2, "field-19", 4), "extra");

  ExpectTerminated(h);
  ExpectTerminated(copy);
  ExpectTerminated(copy2);
}

namespace
{
  std::string Lower(std::string s)
  {
    for (auto& c : s)
    {
      c = char(::tolower((unsigned char)c));
    }

    return s;
  }

  using Model = std::map<std::string, std::vector<std::string>>;

  void ExpectMatches(const Headers& h, const std::vector<std::string>& order, const Model& model)
  {
    ASSERT_EQ(h.Header.size(), order.size());

    size_t i = 0;
    for (const auto& field : h.Header)
    {
      ASSERT_LT(i, order.size());
      ASSERT_EQ(Lower(std::string(field.Key)), order[i]);

      const auto& expected = model.at(order[i]);
      ASSERT_EQ(field.Values.size(), expected.size()) << order[i];

      size_t v = 0;
      for (std::string_view value : field.Values)
      {
        ASSERT_EQ(std::string(value), expected[v]) << order[i];
        ++v;
      }

      ++i;
    }
  }
}

TEST(header_tests, random_operations_match_a_model)
{
  std::mt19937 rng(20261003);

  Headers h(false);
  Model model;
  std::vector<std::string> order;

  auto spelling = [&rng](int n)
  {
    std::string s = "X-Name-" + std::to_string(n);
    switch (rng() % 3)
    {
    case 0: return s;
    case 1: return Lower(s);
    default:
      for (auto& c : s)
      {
        c = char(::toupper((unsigned char)c));
      }
      return s;
    }
  };

  auto text = [&rng]()
  {
    size_t length = rng() % 70;
    if (rng() % 150 == 0)
    {
      length = 5000;
    }

    std::string s;
    for (size_t i = 0; i < length; ++i)
    {
      s += char('a' + rng() % 26);
    }

    return s;
  };

  for (int step = 0; step < 30000; ++step)
  {
    // The number of names alternates between a few (the table is released
    // and the list is scanned) and many (the table is built and worked on).
    const int names = (step / 2500) % 2 ? 12 : 90;
    const std::string name = spelling(int(rng() % names));
    const std::string key = Lower(name);
    const std::string value = text();

    switch (rng() % 8)
    {
    case 0:
    case 1:
    case 2:
      h.AddHeader(name, value);
      if (!model.count(key))
      {
        order.push_back(key);
      }
      model[key].push_back(value);
      break;

    case 3:
    case 4:
      h.SetHeader(name, value);
      if (!model.count(key))
      {
        order.push_back(key);
      }
      model[key] = {value};
      break;

    default:
      h.DeleteHeader(name);
      if (model.erase(key))
      {
        order.erase(std::find(order.begin(), order.end(), key));
      }
      break;
    }

    ASSERT_EQ(h.HasHeader(name), model.count(key) > 0) << step;

    const std::string probe = spelling(int(rng() % names));
    ASSERT_EQ(h.HasHeader(probe), model.count(Lower(probe)) > 0) << step;

    if (step % 250 == 0)
    {
      ASSERT_NO_FATAL_FAILURE(ExpectMatches(h, order, model));
    }
  }

  ASSERT_NO_FATAL_FAILURE(ExpectMatches(h, order, model));

  // And a copy of whatever state it ended in.
  Headers copy(h);
  ASSERT_NO_FATAL_FAILURE(ExpectMatches(copy, order, model));
  ExpectTerminated(h);
  ExpectTerminated(copy);
}

// The nodes of a parsed block are carved from an area sized from the number of
// line ends in it. Whatever the block looks like -- mixed line endings, empty
// names, lines without a colon, blanks -- Parse() must stay inside it: it
// reports an error or parses, it never runs out of room (which would throw).
TEST(header_tests, random_blocks_never_overflow_the_node_area)
{
  std::mt19937 rng(424242);
  const std::string alphabet = "aAbB:: \t\r\n\n\r-";

  for (int round = 0; round < 30000; ++round)
  {
    std::string text;
    const size_t length = rng() % 160;
    for (size_t i = 0; i < length; ++i)
    {
      text += alphabet[rng() % alphabet.size()];
    }

    switch (rng() % 3)
    {
    case 0:
      text += "\r\n\r\n";
      break;

    case 1:
      text += "\n\n";
      break;

    default:
      text += "\r\n\n";
      break;
    }

    Headers h(rng() % 2 == 0);
    HEADER_ERROR error = HEADER_ERROR::NONE;
    const Verification type = rng() % 2 ? Verification::Strict : Verification::NotStrict;

    ASSERT_NO_THROW(error = h.Parse(text.c_str(), text.size(), type)) << "round " << round;

    if (error == HEADER_ERROR::NONE)
    {
      Headers copy(h);
      ASSERT_EQ(copy.ToString(), h.ToString()) << "round " << round;
      ExpectTerminated(h);
    }
  }
}
