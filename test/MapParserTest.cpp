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
}

TEST(MapParser, Parse)
{
	const auto p = rcfg::MapParser<std::string, int, std::map>();

	{
		SCOPED_TRACE("Correct Parse");
		std::map<std::string, int> m;

		json j;
		j["e1"] = 11;
		j["e2"] = 12;
		j["e3"] = 13;

		TestEventSink sink;
		p.parse(sink, m, j, false);

		ASSERT_EQ(sink.errorCount, 0);
		ASSERT_EQ(sink.setCount, 3);

		ASSERT_EQ(m.size(), 3);
		ASSERT_EQ(m.at("e1"), 11);
		ASSERT_EQ(m.at("e2"), 12);
		ASSERT_EQ(m.at("e3"), 13);
	}

	{
		SCOPED_TRACE("Empty Parse");
		std::map<std::string, int> m;

		json j;

		TestEventSink sink;
		p.parse(sink, m, j, false);

		ASSERT_EQ(sink.errorCount, 0);
		ASSERT_EQ(sink.setCount, 0);

		ASSERT_EQ(m.size(), 0);
	}

	{
		SCOPED_TRACE("Not int Parse");
		std::map<std::string, int> m;

		json j;
		j["e1"] = "a";
		j["e2"] = 10;

		TestEventSink sink;
		p.parse(sink, m, j, false);

		ASSERT_EQ(sink.errorCount, 1);
		ASSERT_EQ(sink.setCount, 1);

		ASSERT_EQ(m.size(), 2);
		ASSERT_EQ(m.at("e1"), 0);
		ASSERT_EQ(m.at("e2"), 10);
	}
}

TEST(MapParser, ParseUnorderedMap)
{
	const auto p = rcfg::MapParser<std::string, int, std::unordered_map>();

	{
		SCOPED_TRACE("Correct Parse");
		std::unordered_map<std::string, int> m;

		json j;
		j["e1"] = 11;
		j["e2"] = 12;
		j["e3"] = 13;

		TestEventSink sink;
		p.parse(sink, m, j, false);

		ASSERT_EQ(sink.errorCount, 0);
		ASSERT_EQ(sink.setCount, 3);

		ASSERT_EQ(m.size(), 3);
		ASSERT_EQ(m.at("e1"), 11);
		ASSERT_EQ(m.at("e2"), 12);
		ASSERT_EQ(m.at("e3"), 13);
	}

	{
		SCOPED_TRACE("Empty Parse");
		std::unordered_map<std::string, int> m;

		json j;

		TestEventSink sink;
		p.parse(sink, m, j, false);

		ASSERT_EQ(sink.errorCount, 0);
		ASSERT_EQ(sink.setCount, 0);

		ASSERT_EQ(m.size(), 0);
	}

	{
		SCOPED_TRACE("Not int Parse");
		std::unordered_map<std::string, int> m;

		json j;
		j["e1"] = "a";
		j["e2"] = 10;

		TestEventSink sink;
		p.parse(sink, m, j, false);

		ASSERT_EQ(sink.errorCount, 1);
		ASSERT_EQ(sink.setCount, 1);

		ASSERT_EQ(m.size(), 2);
		ASSERT_EQ(m.at("e1"), 0);
		ASSERT_EQ(m.at("e2"), 10);
	}
}

TEST(MapParser, ParseWithClass)
{
	const auto p = rcfg::MapParser<std::string, Config, std::map>(getParser());

	{
		SCOPED_TRACE("Correct Parse");
		std::map<std::string, Config> m;

		json j;
		j["e1"]["i2"] = 9;
		j["e2"]["s1"] = "lala";
		j["e2"]["i2"] = 5;
		j["e2"]["b3"] = false;
		j["e3"]["i2"] = 3;

		TestEventSink sink;
		p.parse(sink, m, j, false);

		ASSERT_EQ(sink.errorCount, 0);
		ASSERT_EQ(sink.setCount, 9);

		ASSERT_EQ(m.size(), 3);
		ASSERT_EQ(m.at("e1").s1, "aba");
		ASSERT_EQ(m.at("e1").i2, 9);
		ASSERT_EQ(m.at("e1").b3, true);
		ASSERT_EQ(m.at("e2").s1, "lala");
		ASSERT_EQ(m.at("e2").i2, 5);
		ASSERT_EQ(m.at("e2").b3, false);
		ASSERT_EQ(m.at("e3").s1, "aba");
		ASSERT_EQ(m.at("e3").i2, 3);
		ASSERT_EQ(m.at("e3").b3, true);
	}
}

namespace
{
	struct Config2
	{
		std::string s1;
		int i2;
	};

	auto getParser2()
	{
		rcfg::ClassParser<Config2> p;
		p.member(&Config2::s1, "s1", rcfg::Default{std::string("aba")}, rcfg::Updatable, rcfg::NotEmpty);
		p.member(&Config2::i2, "i2", rcfg::Bounds{0, 10}, rcfg::Updatable);

		return p;
	}
}

TEST(MapParser, Update)
{
	const auto p = rcfg::MapParser<std::string, Config2, std::map>(getParser2());
	std::string res;
	rcfg::LoggerSink sink([&res](const auto & msg) { res += msg + "\n"; });

	{
		SCOPED_TRACE("Not updatable");
		std::map<std::string, Config2> m;

		json j;
		j["e1"]["i2"] = 9;
		j["e2"]["s1"] = "lala";
		j["e2"]["i2"] = 5;

		res.clear();
		p.parse(sink, m, j, false);

		ASSERT_EQ(sink.IsError(), false);
		ASSERT_EQ(res,
			"+e1.s1=aba (default)\n"
			"+e1.i2=9\n"
			"+e2.s1=lala\n"
			"+e2.i2=5\n"
		);

		ASSERT_EQ(m.size(), 2);
		ASSERT_EQ(m.at("e1").s1, "aba");
		ASSERT_EQ(m.at("e1").i2, 9);
		ASSERT_EQ(m.at("e2").s1, "lala");
		ASSERT_EQ(m.at("e2").i2, 5);

		j["e2"]["s1"] = "hello";
		res.clear();
		p.parse(sink, m, j, true);

		ASSERT_EQ(sink.IsError(), false);
		ASSERT_EQ(res,
			"+e2.s1=lala->hello\n"
		);

		ASSERT_EQ(m.size(), 2);
		ASSERT_EQ(m.at("e1").s1, "aba");
		ASSERT_EQ(m.at("e1").i2, 9);
		ASSERT_EQ(m.at("e2").s1, "hello");
		ASSERT_EQ(m.at("e2").i2, 5);

		j["e3"]["i2"] = 4;
		res.clear();
		auto mCopy = m;
		p.parse(sink, mCopy, j, true);

		ASSERT_EQ(sink.IsError(), false);
		ASSERT_EQ(res,
			"! changed size(2)->size(3) but will make effect only after RESTART\n"
		);

		j.erase("e3");
		j.erase("e2");
		res.clear();
		mCopy = m;
		p.parse(sink, mCopy, j, true);

		ASSERT_EQ(sink.IsError(), false);
		ASSERT_EQ(res,
			"! changed size(2)->size(1) but will make effect only after RESTART\n"
		);
	}

	{
		const auto p = rcfg::MapParser<std::string, Config2, std::map>(getParser2(), rcfg::Updatable);
		SCOPED_TRACE("Updatable");
		std::map<std::string, Config2> m;

		json j;
		j["e1"]["i2"] = 9;
		j["e2"]["s1"] = "lala";
		j["e2"]["i2"] = 5;

		res.clear();
		p.parse(sink, m, j, false);

		ASSERT_EQ(sink.IsError(), false);
		ASSERT_EQ(res,
			"+e1.s1=aba (default)\n"
			"+e1.i2=9\n"
			"+e2.s1=lala\n"
			"+e2.i2=5\n"
		);

		ASSERT_EQ(m.size(), 2);
		ASSERT_EQ(m.at("e1").s1, "aba");
		ASSERT_EQ(m.at("e1").i2, 9);
		ASSERT_EQ(m.at("e2").s1, "lala");
		ASSERT_EQ(m.at("e2").i2, 5);

		j["e2"]["s1"] = "hello";
		res.clear();
		p.parse(sink, m, j, true);

		ASSERT_EQ(sink.IsError(), false);
		ASSERT_EQ(res,
			"+e2.s1=lala->hello\n"
		);

		ASSERT_EQ(m.size(), 2);
		ASSERT_EQ(m.at("e1").s1, "aba");
		ASSERT_EQ(m.at("e1").i2, 9);
		ASSERT_EQ(m.at("e2").s1, "hello");
		ASSERT_EQ(m.at("e2").i2, 5);

		j["e3"]["i2"] = 4;
		res.clear();
		p.parse(sink, m, j, true);

		ASSERT_EQ(sink.IsError(), false);
		ASSERT_EQ(res,
			"+e3.s1=aba (default)\n"
			"+e3.i2=4\n"
		);
		ASSERT_EQ(m.size(), 3);
		ASSERT_EQ(m.at("e1").s1, "aba");
		ASSERT_EQ(m.at("e1").i2, 9);
		ASSERT_EQ(m.at("e2").s1, "hello");
		ASSERT_EQ(m.at("e2").i2, 5);
		ASSERT_EQ(m.at("e3").s1, "aba");
		ASSERT_EQ(m.at("e3").i2, 4);

		j.erase("e3");
		j.erase("e2");
		res.clear();
		p.parse(sink, m, j, true);

		ASSERT_EQ(sink.IsError(), false);
		ASSERT_EQ(res,
			"-e2.s1=hello\n"
			"-e2.i2=5\n"
			"-e3.s1=aba\n"
			"-e3.i2=4\n"
		);
		ASSERT_EQ(m.size(), 1);
		ASSERT_EQ(m.at("e1").s1, "aba");
		ASSERT_EQ(m.at("e1").i2, 9);


		// update to the same
		res.clear();
		p.parse(sink, m, j, true);

		ASSERT_EQ(sink.IsError(), false);
		ASSERT_EQ(res, "");
		ASSERT_EQ(m.size(), 1);
		ASSERT_EQ(m.at("e1").s1, "aba");
		ASSERT_EQ(m.at("e1").i2, 9);
	}
}