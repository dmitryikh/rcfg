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

struct EmbConf
{
	int i1;
};

struct Conf
{
	EmbConf e;
	std::string s1;
};

auto getConfParser()
{
	rcfg::ClassParser<EmbConf> p1;
	p1.member(&EmbConf::i1, "i1");

	rcfg::ClassParser<Conf> p2;
	p2.member(&Conf::e, "", p1);
	p2.member(&Conf::s1, "s1");

	return p2;
}

TEST(ClassParser, FlatMemberParser)
{
	const auto p = getConfParser();
	{
		SCOPED_TRACE("Correct Parse");
		Conf c{};
		json j;
		j["s1"] = "lalaland";
		j["i1"] = 10;

		TestEventSink sink;
		p.parse(sink, c, j, false);

		ASSERT_EQ(sink.errorCount, 0);
		ASSERT_EQ(sink.setCount, 2);

		ASSERT_EQ(c.e.i1, 10);
		ASSERT_EQ(c.s1, "lalaland");
	}
}

namespace
{
	struct Config2
	{
		std::vector<std::string> v1;
	};

	auto getConfig2Parser()
	{
		rcfg::ClassParser<Config2> p1;
		p1.member(&Config2::v1, "");
		return p1;
	}
}

TEST(ClassParser, VectorMemberParser)
{
	const auto p = getConfig2Parser();
	{
		SCOPED_TRACE("Correct Parse");
		Config2 c{};
		json j = {"aaa", "bbb", "ccc"};

		TestEventSink sink;
		p.parse(sink, c, j, false);

		ASSERT_EQ(sink.errorCount, 0);
		ASSERT_EQ(sink.setCount, 3);

		const auto expected = std::vector<std::string>{"aaa", "bbb", "ccc"};
		ASSERT_EQ(c.v1, expected);
	}
}

struct Config3
{
	std::map<std::string, std::string> m1;
	std::unordered_map<std::string, int> m2;
};

auto getConfig3Parser()
{
	rcfg::ClassParser<Config3> p1;
	p1.member(&Config3::m1, "m1");
	p1.member(&Config3::m2, "m2");
	return p1;
}

TEST(ClassParser, MapMemberParser)
{
	const auto p = getConfig3Parser();
	{
		SCOPED_TRACE("Correct Parse");
		Config3 c{};
		json j;
		j["m1"]["a"] = "A";
		j["m1"]["b"] = "B";
		j["m2"]["c"] = 3;

		TestEventSink sink;
		p.parse(sink, c, j, false);

		ASSERT_EQ(sink.errorCount, 0);
		ASSERT_EQ(sink.setCount, 3);

		ASSERT_EQ(c.m1.size(), 2);
		ASSERT_EQ(c.m1.at("a"), "A");
		ASSERT_EQ(c.m1.at("b"), "B");

		ASSERT_EQ(c.m2.size(), 1);
		ASSERT_EQ(c.m2.at("c"), 3);
	}
}