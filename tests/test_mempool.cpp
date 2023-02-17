#include <txl/mempool.h>
#include <txl/unit_test.h>
#include <iostream>
#include <cassert>

void test_mempool()
{
    txl::memory_pool mp(4, 256);
    auto h1 = mp.rent(32);
    auto h2 = mp.rent(32);
    auto h3 = mp.rent(32);
    auto h4 = mp.rent(32);
    auto h5 = mp.rent(32);
    assert(!h1.empty());
    assert(!h2.empty());
    assert(!h3.empty());
    assert(!h4.empty());
    assert(h5.empty());
    mp.release(h1);
    h5 = mp.rent(32);
    assert(!h5.empty());
}

int main()
{
    test_mempool();
}
