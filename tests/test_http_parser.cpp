#include <txl/unit_test.h>
#include <txl/http.h>

TXL_UNIT_TEST(lexer)
{
    txl::lexer::tokenizer tok{"?a=1&b=fizz+buzz"};

    txl::http::query_param_vector vec{};
    txl::http::query_string_parser parser{};
    parser.parse(tok, vec);

    assert_equal(vec, txl::http::query_param_vector{txl::http::query_param{"a","1"}, txl::http::query_param{"b", "fizz buzz"}});
}

TXL_UNIT_TEST(lexer_2)
{
    txl::lexer::tokenizer tok{"?id=922&sql=%22SELECT%22&blabla"};

    txl::http::query_param_vector vec{};
    txl::http::query_string_parser parser{};
    parser.parse(tok, vec);

    assert_equal(vec, txl::http::query_param_vector{txl::http::query_param{"id","922"}, txl::http::query_param{"sql", "\"SELECT\""}, txl::http::query_param{"blabla", ""}});
}

TXL_RUN_TESTS()
