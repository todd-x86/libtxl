#include <txl/unit_test.h>
#include <txl/work_list.h>

#include <set>

TXL_UNIT_TEST(basic)
{
    txl::work_list work_list{}, chain{};
    std::set<std::string> results{};
    
    chain.push_back(txl::work_units::lambda([&]() { results.emplace("STEP 1 COLLECT UNDERPANTS"); }, 0));
    chain.push_back(txl::work_units::lambda([&]() { results.emplace("STEP 2 ?"); }, 0));
    chain.push_back(txl::work_units::lambda([&]() { results.emplace("STEP 3 PROFIT"); }, 0));

    work_list.push_back(std::make_unique<txl::work_chain>( std::move(chain) ));
    work_list.push_back(txl::work_units::lambda([&]() { results.emplace("WORKING 1"); }, 0));
    work_list.push_back(txl::work_units::lambda([&]() { results.emplace("PARTIAL WORK"); throw std::runtime_error{"OK"}; results.emplace("WORKING 123"); }, 0));
    work_list.push_back(txl::work_units::lambda([&]() { results.emplace("WORKING 465"); }, 0));
    
    txl::work_distributor dist{};
    dist.create_worker<txl::inline_worker>();
    dist.create_worker<txl::inline_worker>();
    dist.create_worker<txl::inline_worker>();
    dist.run(work_list);

    std::set<std::string> expected{"STEP 1 COLLECT UNDERPANTS", "STEP 2 ?", "STEP 3 PROFIT", "WORKING 1", "PARTIAL WORK", "WORKING 465"};

    assert_equal(expected, results);
}

TXL_RUN_TESTS()
