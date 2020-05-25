#include <gtest/gtest.h>

#include <nlohmann/json.hpp>
#include <rcfg/rcfg.h>
#include "TestEventSink.h"

using json = nlohmann::json;

namespace
{
	struct Config
	{
		std::string s1;
		int i2;
	};

	auto getParser()
	{
		rcfg::ClassParser<Config> p;
		p.member(&Config::s1, "s1", rcfg::Default{std::string("aba")}, rcfg::Updatable, rcfg::NotEmpty);
		p.member(&Config::i2, "i2", rcfg::Bounds{0, 10});

		return p;
	}
}

TEST(VectorParser, Parse)
{
	const auto p = rcfg::VectorParser<std::string, std::vector>();

	{
		SCOPED_TRACE("Correct Parse");
		std::vector<std::string> v;

		json j;
		j.push_back("e1");
		j.push_back("e2");
		j.push_back("e3");

		TestEventSink sink;
		p.parse(sink, v, j, false);

		ASSERT_EQ(sink.errorCount, 0);
		ASSERT_EQ(sink.setCount, 3);

		ASSERT_EQ(v.size(), 3);
		ASSERT_EQ(v.at(0), "e1");
		ASSERT_EQ(v.at(1), "e2");
		ASSERT_EQ(v.at(2), "e3");
	}

	{
		SCOPED_TRACE("Empty Parse");
		std::vector<std::string> v;

		json j;

		TestEventSink sink;
		p.parse(sink, v, j, false);

		ASSERT_EQ(sink.errorCount, 1);
		ASSERT_EQ(sink.setCount, 0);

		ASSERT_EQ(v.size(), 0);
	}

	{
		SCOPED_TRACE("Empty list Parse");
		std::vector<std::string> v;

		json j = json::array();

		TestEventSink sink;
		p.parse(sink, v, j, false);

		ASSERT_EQ(sink.errorCount, 0);
		ASSERT_EQ(sink.setCount, 0);

		ASSERT_EQ(v.size(), 0);
	}
}

TEST(VectorParser, ParseWithClass)
{
	const auto p = rcfg::VectorParser<Config, std::vector>(getParser());

	{
		SCOPED_TRACE("Correct Parse");
		std::vector<Config> v;

		json j;
		json j1;
		j1["i2"] = 9;
		j.push_back(j1);
		json j2;
		j2["s1"] = "lala";
		j2["i2"] = 5;
		j.push_back(j2);
		json j3;
		j3["i2"] = 3;
		j.push_back(j3);

		TestEventSink sink;
		p.parse(sink, v, j, false);

		ASSERT_EQ(sink.errorCount, 0);
		ASSERT_EQ(sink.setCount, 6);

		ASSERT_EQ(v.size(), 3);
		ASSERT_EQ(v.at(0).s1, "aba");
		ASSERT_EQ(v.at(0).i2, 9);
		ASSERT_EQ(v.at(1).s1, "lala");
		ASSERT_EQ(v.at(1).i2, 5);
		ASSERT_EQ(v.at(2).s1, "aba");
		ASSERT_EQ(v.at(2).i2, 3);
	}
}