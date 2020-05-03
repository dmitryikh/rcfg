#include <gtest/gtest.h>

#include <nlohmann/json.hpp>
#include <rcfg/rcfg.h>
#include "TestEventSink.h"

using json = nlohmann::json;

TEST(ParamParser, Parse)
{
    {
        SCOPED_TRACE("Parse int from nil");
        const auto p = rcfg::ParamParser<int>{};
        int val = 0;
        json j;

        TestEventSink sink;
        p.parse(sink, val, j, false);

        ASSERT_EQ(sink.errorCount, 1);
        ASSERT_EQ(sink.setCount, 0);
        ASSERT_EQ(val, 0);
    }

    {
        SCOPED_TRACE("Parse int from 10");
        const auto p = rcfg::ParamParser<int>{};
        int val = 0;
        json j = 10;

        TestEventSink sink;
        p.parse(sink, val, j, false);

        ASSERT_EQ(sink.errorCount, 0);
        ASSERT_EQ(sink.setCount, 1);
        ASSERT_EQ(val, 10);
    }

    {
        SCOPED_TRACE("Parse int from bool");
        const auto p = rcfg::ParamParser<int>{};
        int val = 0;
        json j = false;

        TestEventSink sink;
        p.parse(sink, val, j, false);

        // NOTE: false implicitly converted to int
        ASSERT_EQ(sink.errorCount, 0);
        ASSERT_EQ(sink.setCount, 1);
        ASSERT_EQ(val, 0);
    }

    {
        SCOPED_TRACE("Parse int with lower bound");
        const auto p = rcfg::ParamParser<int>{rcfg::LowerBound{10}};
        int val = 0;

        {
            json j = 10;
            TestEventSink sink;
            p.parse(sink, val, j, false);
            ASSERT_EQ(sink.errorCount, 0);
            ASSERT_EQ(sink.setCount, 1);
            ASSERT_EQ(val, 10);
        }

        {
            val = 0;
            json j = 5;
            TestEventSink sink;
            p.parse(sink, val, j, false);
            ASSERT_EQ(sink.errorCount, 1);
            ASSERT_EQ(sink.setCount, 0);
            ASSERT_EQ(val, 0);
        }
    }

    {
        SCOPED_TRACE("Parse int with default");
        const auto p = rcfg::ParamParser<int>{rcfg::Default{11}};
        int val = 0;

        {
            json j;
            TestEventSink sink;
            p.parse(sink, val, j, false);
            ASSERT_EQ(sink.setCount, 1);
            ASSERT_EQ(val, 11);
        }

        {
            json j = 5;
            TestEventSink sink;
            p.parse(sink, val, j, false);
            ASSERT_EQ(sink.setCount, 1);
            ASSERT_EQ(val, 5);
        }
    }
}
