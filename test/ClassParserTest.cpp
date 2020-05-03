#include <gtest/gtest.h>

#include <nlohmann/json.hpp>
#include <rcfg/rcfg.h>
#include "TestEventSink.h"

using json = nlohmann::json;

struct Config
{
    std::string s1;
    int i2;
    bool b3;
};

auto getParser()
{
    rcfg::ClassParser<Config> p;
    p.member(&Config::s1, "s1", rcfg::Default{std::string("aba")}, rcfg::Updatable, rcfg::NotEmpty);
    p.member(&Config::i2, "i2", rcfg::Bounds{0, 10});
    p.member(&Config::b3, "b3", rcfg::Default{true});

    return p;
}

TEST(ClassParser, Parse)
{
    const auto p = getParser();

    {
        SCOPED_TRACE("Correct Parse");
        Config c{};
        ASSERT_EQ(c.s1, "");
        ASSERT_EQ(c.i2, 0);
        ASSERT_EQ(c.b3, false);

        json j;
        j["s1"] = "lalaland";
        j["i2"] = 10;
        j["b3"] = true;

        TestEventSink sink;
        p.parse(sink, c, j, false);

        ASSERT_EQ(sink.errorCount, 0);
        ASSERT_EQ(sink.setCount, 3);

        ASSERT_EQ(c.s1, "lalaland");
        ASSERT_EQ(c.i2, 10);
        ASSERT_EQ(c.b3, true);
    }

    {
        SCOPED_TRACE("Default s1");
        Config c{};

        json j;
        j["i2"] = 1;
        j["b3"] = true;

        TestEventSink sink;
        p.parse(sink, c, j, false);

        ASSERT_EQ(sink.errorCount, 0);
        ASSERT_EQ(sink.setCount, 3);

        ASSERT_EQ(c.s1, "aba");
        ASSERT_EQ(c.i2, 1);
        ASSERT_EQ(c.b3, true);
    }

    {
        SCOPED_TRACE("Default b3");
        Config c{};

        json j;
        j["s1"] = "a";
        j["i2"] = 0;

        TestEventSink sink;
        p.parse(sink, c, j, false);

        ASSERT_EQ(sink.errorCount, 0);
        ASSERT_EQ(sink.setCount, 3);

        ASSERT_EQ(c.s1, "a");
        ASSERT_EQ(c.i2, 0);
        ASSERT_EQ(c.b3, true);
    }

    {
        SCOPED_TRACE("Empty node");
        Config c{};
        ASSERT_EQ(c.s1, "");
        ASSERT_EQ(c.i2, 0);
        ASSERT_EQ(c.b3, false);

        json j;

        TestEventSink sink;
        p.parse(sink, c, j, false);

        // i2 don't have defaults
        ASSERT_EQ(sink.errorCount, 1);
        // s1 & b3 have defaults
        ASSERT_EQ(sink.setCount, 2);

        ASSERT_EQ(c.s1, "aba");
        ASSERT_EQ(c.i2, 0);
        ASSERT_EQ(c.b3, true);
    }
}
