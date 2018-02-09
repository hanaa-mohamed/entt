#include <tuple>
#include <queue>
#include <vector>
#include <gtest/gtest.h>
#include <entt/entity/registry.hpp>

template<typename Storage>
struct OutputArchive {
    OutputArchive(Storage &storage)
        : storage{storage}
    {}

    template<typename Value>
    void operator()(const Value &value) {
        std::get<std::queue<Value>>(storage).push(value);
    }

private:
    Storage &storage;
};

template<typename Storage>
struct InputArchive {
    InputArchive(Storage &storage)
        : storage{storage}
    {}

    template<typename Value>
    void operator()(Value &value) {
        auto &queue = std::get<std::queue<Value>>(storage);
        value = queue.front();
        queue.pop();
    }

private:
    Storage &storage;
};

struct AComponent {};
struct AnotherComponent {};

struct Foo {
    entt::DefaultRegistry::entity_type bar;
    std::vector<entt::DefaultRegistry::entity_type> quux;
};

TEST(Snapshot, Dump) {
    entt::DefaultRegistry registry;

    auto e0 = registry.create();
    registry.assign<int>(e0, 42);
    registry.assign<char>(e0, 'c');
    registry.assign<double>(e0, .1);

    auto e1 = registry.create();

    auto e2 = registry.create();
    registry.assign<int>(e2, 3);

    auto e3 = registry.create();
    registry.assign<char>(e3, '0');
    registry.attach<float>(e3, .3f);

    auto e4 = registry.create();
    registry.attach<AComponent>(e4);

    registry.destroy(e1);
    auto v1 = registry.current(e1);

    using storage_type = std::tuple<
        std::queue<entt::DefaultRegistry::entity_type>,
        std::queue<int>,
        std::queue<char>,
        std::queue<double>,
        std::queue<float>,
        std::queue<bool>,
        std::queue<AComponent>,
        std::queue<AnotherComponent>,
        std::queue<Foo>
    >;

    storage_type storage;
    OutputArchive<storage_type> output{storage};
    InputArchive<storage_type> input{storage};

    registry.snapshot()
            .entities(output)
            .destroyed(output)
            .component<int, char, AnotherComponent, double>(output)
            .tag<float, bool, AComponent>(output)
            ;

    registry = {};

    ASSERT_TRUE(registry.empty());
    ASSERT_EQ(registry.capacity(), entt::DefaultRegistry::size_type{});

    ASSERT_FALSE(registry.valid(e0));
    ASSERT_FALSE(registry.valid(e1));
    ASSERT_FALSE(registry.valid(e2));
    ASSERT_FALSE(registry.valid(e3));
    ASSERT_FALSE(registry.valid(e4));

    registry.restore()
            .entities(input)
            .destroyed(input)
            .component<int, char, AnotherComponent, double>(input)
            .tag<float, bool, AComponent>(input)
            .orphans()
            ;

    ASSERT_TRUE(registry.valid(e0));
    ASSERT_FALSE(registry.valid(e1));
    ASSERT_TRUE(registry.valid(e2));
    ASSERT_TRUE(registry.valid(e3));
    ASSERT_TRUE(registry.valid(e4));

    ASSERT_FALSE(registry.orphan(e0));
    ASSERT_FALSE(registry.orphan(e2));
    ASSERT_FALSE(registry.orphan(e3));
    ASSERT_FALSE(registry.orphan(e4));

    ASSERT_EQ(registry.get<int>(e0), 42);
    ASSERT_EQ(registry.get<char>(e0), 'c');
    ASSERT_EQ(registry.get<double>(e0), .1);
    ASSERT_EQ(registry.current(e1), v1);
    ASSERT_EQ(registry.get<int>(e2), 3);
    ASSERT_EQ(registry.get<char>(e3), '0');

    ASSERT_TRUE(registry.has<float>());
    ASSERT_EQ(registry.attachee<float>(), e3);
    ASSERT_EQ(registry.get<float>(), .3f);

    ASSERT_TRUE(registry.has<AComponent>());
    ASSERT_EQ(registry.attachee<AComponent>(), e4);

    ASSERT_TRUE(registry.empty<AnotherComponent>());
    ASSERT_FALSE(registry.has<long int>());
}

TEST(Snapshot, Partial) {
    entt::DefaultRegistry registry;

    auto e0 = registry.create();
    registry.assign<int>(e0, 42);
    registry.assign<char>(e0, 'c');
    registry.assign<double>(e0, .1);

    auto e1 = registry.create();

    auto e2 = registry.create();
    registry.assign<int>(e2, 3);

    auto e3 = registry.create();
    registry.assign<char>(e3, '0');
    registry.attach<float>(e3, .3f);

    auto e4 = registry.create();
    registry.attach<AComponent>(e4);

    registry.destroy(e1);
    auto v1 = registry.current(e1);

    using storage_type = std::tuple<
        std::queue<entt::DefaultRegistry::entity_type>,
        std::queue<int>,
        std::queue<char>,
        std::queue<double>,
        std::queue<float>,
        std::queue<bool>,
        std::queue<AComponent>,
        std::queue<Foo>
    >;

    storage_type storage;
    OutputArchive<storage_type> output{storage};
    InputArchive<storage_type> input{storage};

    registry.snapshot()
            .entities(output)
            .destroyed(output)
            .component<char, int>(output)
            .tag<bool, float>(output)
            ;

    registry = {};

    ASSERT_TRUE(registry.empty());
    ASSERT_EQ(registry.capacity(), entt::DefaultRegistry::size_type{});

    ASSERT_FALSE(registry.valid(e0));
    ASSERT_FALSE(registry.valid(e1));
    ASSERT_FALSE(registry.valid(e2));
    ASSERT_FALSE(registry.valid(e3));
    ASSERT_FALSE(registry.valid(e4));

    registry.restore()
            .entities(input)
            .destroyed(input)
            .component<char, int>(input)
            .tag<bool, float>(input)
            ;

    ASSERT_TRUE(registry.valid(e0));
    ASSERT_FALSE(registry.valid(e1));
    ASSERT_TRUE(registry.valid(e2));
    ASSERT_TRUE(registry.valid(e3));
    ASSERT_TRUE(registry.valid(e4));

    ASSERT_EQ(registry.get<int>(e0), 42);
    ASSERT_EQ(registry.get<char>(e0), 'c');
    ASSERT_FALSE(registry.has<double>(e0));
    ASSERT_EQ(registry.current(e1), v1);
    ASSERT_EQ(registry.get<int>(e2), 3);
    ASSERT_EQ(registry.get<char>(e3), '0');
    ASSERT_TRUE(registry.orphan(e4));

    ASSERT_TRUE(registry.has<float>());
    ASSERT_EQ(registry.attachee<float>(), e3);
    ASSERT_EQ(registry.get<float>(), .3f);
    ASSERT_FALSE(registry.has<long int>());

    registry.snapshot()
            .tag<float>(output)
            .destroyed(output)
            .entities(output)
            ;

    registry = {};

    ASSERT_TRUE(registry.empty());
    ASSERT_EQ(registry.capacity(), entt::DefaultRegistry::size_type{});

    ASSERT_FALSE(registry.valid(e0));
    ASSERT_FALSE(registry.valid(e1));
    ASSERT_FALSE(registry.valid(e2));
    ASSERT_FALSE(registry.valid(e3));
    ASSERT_FALSE(registry.valid(e4));

    registry.restore()
            .tag<float>(input)
            .destroyed(input)
            .entities(input)
            .orphans()
            ;

    ASSERT_FALSE(registry.valid(e0));
    ASSERT_FALSE(registry.valid(e1));
    ASSERT_FALSE(registry.valid(e2));
    ASSERT_TRUE(registry.valid(e3));
    ASSERT_FALSE(registry.valid(e4));
}

TEST(Snapshot, Progressive) {
    // TODO
}