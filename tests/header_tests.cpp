#include <gtest/gtest.h>
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

TEST(header_tests, mix_separators_crlf_status_line_does_not_leak_cr)
{
  // Bare LFs outnumber CRLFs, so the parser splits on LF. The CR of the one
  // CRLF (the status line's) belongs to the separator, not to ReqRes.
  std::string data = "HTTP/1.1 200 OK\r\nContent-Type: text/html\nConnection: close\n\n";

  ResHeaders h;
  ASSERT_EQ(h.Parse(data.c_str(), data.length(), Verification::NotStrict), HEADER_ERROR::NONE);

  EXPECT_TRUE(h.MixedLineEndings);
  EXPECT_EQ(h.LineEnding, '\n');
  EXPECT_EQ(h.ReqRes, "HTTP/1.1 200 OK");
  EXPECT_EQ(h.ToString(), "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n");
}

TEST(header_tests, mix_separators_crlf_header_line_does_not_leak_cr)
{
  // Same, for a CRLF in the middle of an LF-dominated buffer, and for a request.
  std::string data = "HTTP/1.1 200 OK\nA: 1\r\nB: 2\nC: 3\n\n";

  ResHeaders res;
  ASSERT_EQ(res.Parse(data.c_str(), data.length(), Verification::NotStrict), HEADER_ERROR::NONE);

  EXPECT_EQ(res.ReqRes, "HTTP/1.1 200 OK");
  ASSERT_EQ(res.Header.size(), 3u);
  EXPECT_EQ(res.GetHeader("A")->front(), "1");
  EXPECT_EQ(res.GetHeader("B")->front(), "2");
  EXPECT_EQ(res.GetHeader("C")->front(), "3");

  std::string req = "GET /x HTTP/1.1\r\nHost: example.com\nAccept: */*\n\n";

  ReqHeaders r;
  ASSERT_EQ(r.Parse(req.c_str(), req.length(), Verification::NotStrict), HEADER_ERROR::NONE);

  EXPECT_EQ(r.ReqRes, "GET /x HTTP/1.1");
  EXPECT_EQ(r.GetHeader("Host")->front(), "example.com");
  EXPECT_EQ(r.GetHeader("Accept")->front(), "*/*");
}

TEST(header_tests, parse_result_does_not_depend_on_the_separator_style)
{
  const char* crlf = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n";
  const char* lf = "HTTP/1.1 200 OK\nContent-Type: text/html\nConnection: close\n\n";
  const char* mixed = "HTTP/1.1 200 OK\r\nContent-Type: text/html\nConnection: close\n\n";

  std::string expected;

  for (const char* data : { crlf, lf, mixed })
  {
    ResHeaders h;
    ASSERT_EQ(h.Parse(data, strlen(data), Verification::NotStrict), HEADER_ERROR::NONE) << data;

    if (expected.empty())
      expected = h.ToString();

    EXPECT_EQ(h.ToString(), expected) << data;
  }
}

TEST(header_tests, add_header_new_name_keeps_wire_case_unless_lowercase_is_requested)
{
  Headers wire(false);
  wire.AddHeader("Content-Type", "text/html");

  ASSERT_EQ(wire.Header.size(), 1u);
  EXPECT_EQ(wire.Header.front().Key, "Content-Type");
  EXPECT_EQ(wire.Header.front().NormalizedKey, "content-type");
  ASSERT_EQ(wire.Header.front().Values.size(), 1u);
  EXPECT_EQ(wire.Header.front().Values.front(), "text/html");

  Headers lower(true);
  lower.AddHeader("Content-Type", "text/html");

  ASSERT_EQ(lower.Header.size(), 1u);
  EXPECT_EQ(lower.Header.front().Key, "content-type");
  EXPECT_EQ(lower.Header.front().NormalizedKey, "content-type");
}

TEST(header_tests, add_header_repeated_name_appends_a_value_to_the_first_field)
{
  for (bool lowerCase : { false, true })
  {
    Headers h(lowerCase);
    h.AddHeader("Set-Cookie", "a=1");
    h.AddHeader("X-Other", "o");
    h.AddHeader("set-cookie", "b=2");
    h.AddHeader("SET-COOKIE", "c=3");

    ASSERT_EQ(h.Header.size(), 2u) << "lowerCase=" << lowerCase;

    auto it = h.Header.begin();
    EXPECT_EQ(it->Key, lowerCase ? "set-cookie" : "Set-Cookie") << "the first spelling is the one kept";
    EXPECT_EQ(it->Values, (StringArray{ "a=1", "b=2", "c=3" }));

    ++it;
    EXPECT_EQ(it->Key, lowerCase ? "x-other" : "X-Other");
    EXPECT_EQ(it->Values, (StringArray{ "o" }));

    for (const char* spelling : { "Set-Cookie", "set-cookie", "SET-COOKIE" })
    {
      auto found = h.FindHeader(spelling);
      ASSERT_TRUE(found != h.Header.end()) << spelling;
      EXPECT_EQ(found->Values.size(), 3u) << spelling;
    }
  }
}

TEST(header_tests, add_header_keeps_the_index_in_step_with_the_list)
{
  Headers h(false);

  for (int round = 0; round < 3; ++round)
  {
    for (int i = 0; i < 100; ++i)
      h.AddHeader("X-Header-" + std::to_string(i), "v" + std::to_string(round));
  }

  ASSERT_EQ(h.Header.size(), 100u);

  for (int i = 0; i < 100; ++i)
  {
    std::string name = "X-Header-" + std::to_string(i);
    auto found = h.FindHeader(name);
    ASSERT_TRUE(found != h.Header.end()) << name;
    EXPECT_EQ(found->Key, name);
    EXPECT_EQ(found->Values, (StringArray{ "v0", "v1", "v2" })) << name;
  }

  // Fields stay in first-insertion order.
  int expected = 0;
  for (const auto& field : h.Header)
    EXPECT_EQ(field.Key, "X-Header-" + std::to_string(expected++));

  // A copy rebuilds its own index from the fields' stored keys.
  Headers copy(false);
  h.CopyTo(copy);
  ASSERT_EQ(copy.Header.size(), 100u);
  auto inCopy = copy.FindHeader("x-header-42");
  ASSERT_TRUE(inCopy != copy.Header.end());
  EXPECT_EQ(inCopy->Values.size(), 3u);
}
