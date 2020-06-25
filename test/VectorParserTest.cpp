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
		p.member(&Config::i2, "i2", rcfg::Bounds{0, 10}, rcfg::Updatable);

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

TEST(VectorParser, Update)
{
	const auto p = rcfg::VectorParser<Config, std::vector>(getParser(), rcfg::Updatable);

	{
		std::vector<Config> v;
		std::string res;

		json j;
		j[0]["i2"] = 0;
		j[0]["s1"] = "aa";
		j[1]["i2"] = 1;
		j[1]["s1"] = "bb";

		rcfg::LoggerSink sink([&res](const auto & msg) { res += msg + "\n"; });
		p.parse(sink, v, j, false);

		ASSERT_EQ(sink.IsError(), false);
		ASSERT_EQ(res,
			"+0.s1=aa\n"
			"+0.i2=0\n"
			"+1.s1=bb\n"
			"+1.i2=1\n");
		ASSERT_EQ(v.size(), 2);
		ASSERT_EQ(v.at(0).s1, "aa");
		ASSERT_EQ(v.at(0).i2, 0);
		ASSERT_EQ(v.at(1).s1, "bb");
		ASSERT_EQ(v.at(1).i2, 1);

		j = nullptr;
		res.clear();
		j[0]["i2"] = 1;
		j[0]["s1"] = "bb";

		p.parse(sink, v, j, true);
		ASSERT_EQ(res,
			"+0.s1=aa->bb\n"
			"+0.i2=0->1\n"
			"-1.s1=bb\n"
			"-1.i2=1\n");
		ASSERT_EQ(v.size(), 1);
		ASSERT_EQ(v.at(0).s1, "bb");
		ASSERT_EQ(v.at(0).i2, 1);

		j = json::array();
		res.clear();
		p.parse(sink, v, j, true);
		ASSERT_EQ(res,
			"-0.s1=bb\n"
			"-0.i2=1\n");
		ASSERT_EQ(v.size(), 0);

		j = nullptr;
		res.clear();
		j[0]["i2"] = 1;
		j[0]["s1"] = "bb";
		p.parse(sink, v, j, true);

		j[1]["i2"] = 5;
		res.clear();
		p.parse(sink, v, j, true);
		ASSERT_EQ(res,
			"+1.s1=aba (default)\n"
			"+1.i2=5\n");
		ASSERT_EQ(v.size(), 2);
		ASSERT_EQ(v.at(0).s1, "bb");
		ASSERT_EQ(v.at(0).i2, 1);
		ASSERT_EQ(v.at(1).s1, "aba");
		ASSERT_EQ(v.at(1).i2, 5);
	}
}

TEST(VectorParser, Unique)
{
	const auto p = rcfg::VectorParser<int>(rcfg::ParamParser<int>{}, rcfg::Unique);
	std::string res;
	rcfg::LoggerSink sink([&res](const auto & msg) { res += msg + "\n"; });

	{
		SCOPED_TRACE("Correct Parse");
		std::vector<int> v;

		json j;
		j[0] = 0;
		j[1] = 1;

		res.clear();
		p.parse(sink, v, j, false);

		ASSERT_EQ(sink.IsError(), false);
		ASSERT_EQ(res,
			"+0=0\n"
			"+1=1\n"
		);
		ASSERT_EQ(v.size(), 2);
		ASSERT_EQ(v.at(0), 0);
		ASSERT_EQ(v.at(1), 1);
	}

	{
		SCOPED_TRACE("Error Parse");
		std::vector<int> v;

		json j;
		j[0] = 1;
		j[1] = 1;

		res.clear();
		p.parse(sink, v, j, false);

		ASSERT_EQ(sink.IsError(), true);
		ASSERT_EQ(res,
			"+0=1\n"
			"+1=1\n"
			"!!!: not unique\n");
	}
}

TEST(VectorParser, Empty)
{
	const auto p = rcfg::VectorParser<int>(rcfg::ParamParser<int>{}, rcfg::NotEmpty);
	std::string res;
	rcfg::LoggerSink sink([&res](const auto & msg) { res += msg + "\n"; });

	{
		SCOPED_TRACE("Correct Parse");
		std::vector<int> v;

		json j;
		j[0] = 0;

		res.clear();
		p.parse(sink, v, j, false);

		ASSERT_EQ(sink.IsError(), false);
		ASSERT_EQ(res,
			"+0=0\n"
		);
		ASSERT_EQ(v.size(), 1);
		ASSERT_EQ(v.at(0), 0);
	}

	{
		SCOPED_TRACE("Error Parse");
		std::vector<int> v;

		json j = json::array();

		res.clear();
		p.parse(sink, v, j, false);

		ASSERT_EQ(sink.IsError(), true);
		ASSERT_EQ(res,
			"!!!: should be not empty\n");
	}
}