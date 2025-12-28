#include <txl/unit_test.h>
#include <txl/lexer.h>

TXL_UNIT_TEST(lexer)
{
    txl::lexer::tokenizer tok{"?a=1&b=fizz+buzz"};

    txl::query_param_vector vec{};
    txl::query_string_parser parser{vec};
    parser.parse(tok);

    assert_equal(vec, txl::query_param_vector{txl::query_param{"a","1"}, txl::query_param{"b", "fizz buzz"}});
}

TXL_UNIT_TEST(lexer_2)
{
    txl::lexer::tokenizer tok{"?id=922&sql=%22SELECT%22&blabla"};

    txl::query_param_vector vec{};
    txl::query_string_parser parser{vec};
    parser.parse(tok);

    assert_equal(vec, txl::query_param_vector{txl::query_param{"id","922"}, txl::query_param{"sql", "\"SELECT\""}, txl::query_param{"blabla", ""}});
}

TXL_RUN_TESTS()
