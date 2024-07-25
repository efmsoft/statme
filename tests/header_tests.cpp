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
