#include <gtest/gtest.h>

#include <nlohmann/json.hpp>
#include <rcfg/rcfg.h>
#include "TestEventSink.h"

using json = nlohmann::json;

TEST(SetParser, Parse)
{
	const auto p = rcfg::SetParser<int, std::set>();

	{
		SCOPED_TRACE("Correct Parse");
		std::set<int> s;

		json j = json::array();
		j.push_back(2);
		j.push_back(3);
		j.push_back(1);

		TestEventSink sink;
		p.parse(sink, s, j, false);

		ASSERT_EQ(sink.errorCount, 0);
		ASSERT_EQ(sink.setCount, 3);

		ASSERT_EQ(s.size(), 3);
		ASSERT_EQ(s.count(1), 1);
		ASSERT_EQ(s.count(2), 1);
		ASSERT_EQ(s.count(3), 1);
	}

	{
		SCOPED_TRACE("Empty Parse");
		std::set<int> s;

		json j = json::array();

		TestEventSink sink;
		p.parse(sink, s, j, false);

		ASSERT_EQ(sink.errorCount, 0);
		ASSERT_EQ(sink.setCount, 0);

		ASSERT_EQ(s.size(), 0);
	}

	{
		SCOPED_TRACE("Not int Parse");
		std::set<int> s;

		json j;
		j.push_back("aa");

		TestEventSink sink;
		p.parse(sink, s, j, false);

		ASSERT_EQ(sink.errorCount, 1);
	}
}

TEST(SetParser, ParseUnordered)
{
	const auto p = rcfg::SetParser<int, std::unordered_set>();

	{
		SCOPED_TRACE("Correct Parse");
		std::unordered_set<int> s;

		json j = json::array();
		j.push_back(2);
		j.push_back(3);
		j.push_back(1);

		TestEventSink sink;
		p.parse(sink, s, j, false);

		ASSERT_EQ(sink.errorCount, 0);
		ASSERT_EQ(sink.setCount, 3);

		ASSERT_EQ(s.size(), 3);
		ASSERT_EQ(s.count(1), 1);
		ASSERT_EQ(s.count(2), 1);
		ASSERT_EQ(s.count(3), 1);
	}

	{
		SCOPED_TRACE("Empty Parse");
		std::unordered_set<int> s;

		json j = json::array();

		TestEventSink sink;
		p.parse(sink, s, j, false);

		ASSERT_EQ(sink.errorCount, 0);
		ASSERT_EQ(sink.setCount, 0);

		ASSERT_EQ(s.size(), 0);
	}

	{
		SCOPED_TRACE("Not int Parse");
		std::unordered_set<int> s;

		json j;
		j.push_back("aa");

		TestEventSink sink;
		p.parse(sink, s, j, false);

		ASSERT_EQ(sink.errorCount, 1);
	}
}

TEST(SetParser, Update)
{
	const auto p = rcfg::SetParser<int, std::unordered_set>();
	std::string res;
	rcfg::LoggerSink sink([&res](const auto & msg) { res += msg + "\n"; });

	{
		SCOPED_TRACE("Not updatable");
		std::unordered_set<int> s;

		json j = json::array();
		j.push_back(2);
		j.push_back(3);
		j.push_back(1);

		res.clear();
		p.parse(sink, s, j, false);

		ASSERT_EQ(sink.IsError(), false);
		ASSERT_EQ(res,
			"+0=2\n"
			"+1=3\n"
			"+2=1\n"
		);
		ASSERT_EQ(s.size(), 3);
		ASSERT_EQ(s.count(1), 1);
		ASSERT_EQ(s.count(2), 1);
		ASSERT_EQ(s.count(3), 1);

		j.push_back(5);
		res.clear();
		p.parse(sink, s, j, true);

		ASSERT_EQ(sink.IsError(), false);
		ASSERT_EQ(res,
			"! changed size(3)->size(4) but will make effect only after RESTART\n"
		);

		ASSERT_EQ(s.size(), 3);
		ASSERT_EQ(s.count(1), 1);
		ASSERT_EQ(s.count(2), 1);
		ASSERT_EQ(s.count(3), 1);

		j = json::array();
		j.push_back(10);
		j.push_back(1);
		j.push_back(2);
		res.clear();
		p.parse(sink, s, j, true);

		ASSERT_EQ(sink.IsError(), false);
		ASSERT_EQ(res,
			"! changed size(3)->size(3) but will make effect only after RESTART\n"
		);

		ASSERT_EQ(s.size(), 3);
		ASSERT_EQ(s.count(1), 1);
		ASSERT_EQ(s.count(2), 1);
		ASSERT_EQ(s.count(3), 1);
	}

	{
		const auto p = rcfg::SetParser<int, std::unordered_set>(rcfg::ParamParser<int>(), rcfg::Updatable);
		SCOPED_TRACE("Updatable");
		std::unordered_set<int> s;

		json j = json::array();
		j.push_back(2);
		j.push_back(3);
		j.push_back(1);

		res.clear();
		p.parse(sink, s, j, false);

		ASSERT_EQ(sink.IsError(), false);
		ASSERT_EQ(res,
			"+0=2\n"
			"+1=3\n"
			"+2=1\n"
		);

		ASSERT_EQ(s.size(), 3);
		ASSERT_EQ(s.count(1), 1);
		ASSERT_EQ(s.count(2), 1);
		ASSERT_EQ(s.count(3), 1);

		j = json::array();
		j.push_back(2);
		j.push_back(3);
		j.push_back(10);

		res.clear();
		p.parse(sink, s, j, true);

		ASSERT_EQ(sink.IsError(), false);
		ASSERT_EQ(res,
			"+2=10\n"
			"-*=1\n"
		);

		ASSERT_EQ(s.size(), 3);
		ASSERT_EQ(s.count(10), 1);
		ASSERT_EQ(s.count(2), 1);
		ASSERT_EQ(s.count(3), 1);

		j = json::array();
		j.push_back(2);
		j.push_back(3);
		j.push_back(1);
		j.push_back(10);
		res.clear();
		p.parse(sink, s, j, true);

		ASSERT_EQ(sink.IsError(), false);
		ASSERT_EQ(res,
			"+2=1\n"
		);
		ASSERT_EQ(s.size(), 4);
		ASSERT_EQ(s.count(1), 1);
		ASSERT_EQ(s.count(10), 1);
		ASSERT_EQ(s.count(2), 1);
		ASSERT_EQ(s.count(3), 1);

		j = json::array();
		j.push_back(1);
		res.clear();
		p.parse(sink, s, j, true);

		ASSERT_EQ(sink.IsError(), false);
		ASSERT_EQ(res,
			"-*=2\n"
			"-*=3\n"
			"-*=10\n"
		);
		ASSERT_EQ(s.size(), 1);
		ASSERT_EQ(s.count(1), 1);


		// update to the same
		res.clear();
		p.parse(sink, s, j, true);

		ASSERT_EQ(sink.IsError(), false);
		ASSERT_EQ(res, "");
		ASSERT_EQ(s.size(), 1);
		ASSERT_EQ(s.count(1), 1);
	}
}

TEST(SetParser, ParseError)
{
	const auto p = rcfg::SetParser<int, std::set>(rcfg::ParamParser<int>(rcfg::Bounds{0, 10}));
	std::string res;
	rcfg::LoggerSink sink([&res](const auto & msg) { res += msg + "\n"; });

	std::set<int> s;

	json j = json::array();
	j.push_back(2);
	j.push_back(11);
	j.push_back(3);

	p.parse(sink, s, j, false);

	ASSERT_EQ(sink.IsError(), true);
	ASSERT_EQ(res,
		"+0=2\n"
		"!!!1: should be in bounds [0;10]\n"
	);

	ASSERT_EQ(s.size(), 0);
}