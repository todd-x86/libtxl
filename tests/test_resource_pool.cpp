#include <txl/unit_test.h>
#include <txl/types.h>
#include <txl/resource_pool.h>

#include <cstring>

static int deleted = 0;

class message
{
private:
    txl::byte_vector data_;
public:
    message(size_t bytes)
        : data_(bytes)
    {
    };

    ~message() { ++deleted; }

    void * data() { return data_.data(); }
};

class message_factory
{
private:
    size_t size_;
public:
    static int created;
    message_factory(size_t size)
        : size_(size)
    {
    }

    std::unique_ptr<message> operator()() const
    {
        ++created;
        return std::make_unique<message>(size_);
    }
};

int message_factory::created = 0;

TXL_UNIT_TEST(resource_pool)
{
    txl::resource_pool<message, message_factory> rp(message_factory(1024), 3);
    assert(message_factory::created == 3);
    {
        rp.get();
        rp.get();
        rp.get();
        rp.get();
    }
    assert(message_factory::created == 4);
    assert(deleted == 4);

    auto x = rp.get();
    assert(message_factory::created == 5);
    strncpy(reinterpret_cast<char *>(x->data()), "HELLO", 6);
    rp.put(std::move(x));

    auto y = rp.get();
    assert(message_factory::created == 5);
    assert(strncmp(reinterpret_cast<char *>(y->data()), "HELLO", 6) == 0);
}

struct thing
{
    int number = 0;
};

struct thing_factory
{
    static int created;
    int remain_;

    thing_factory(int remain)
        : remain_(remain)
    {
    }

    std::unique_ptr<thing> operator()()
    {
        if (remain_ == 0)
        {
            return nullptr;
        }
        else
        {
            ++created;
            --remain_;
            return std::make_unique<thing>();
        }
    }
};

int thing_factory::created = 0;

TXL_UNIT_TEST(resource_pool_limited)
{
    txl::resource_pool<thing, thing_factory> rp(thing_factory(2), 0);
    assert(thing_factory::created == 0);
    assert(!rp.get().empty());
    assert(thing_factory::created == 1);
    assert(!rp.get().empty());
    assert(thing_factory::created == 2);
    assert(rp.get().empty());
    assert(thing_factory::created == 2);
}

TXL_RUN_TESTS()
