#include <string>
#include <fstream>

#include <rcfg/rcfg.h>
#include <rcfg/sink.h>
#include <nlohmann/json.hpp>

using namespace std::literals;

struct LoggerConfig
{
	std::string path;
	uint64_t severity;
};

using LoggersMap = std::map<std::string, LoggerConfig>;

// Initialize parser with rules
auto GetParser()
{
	rcfg::ClassParser<LoggerConfig> p;
	p.member(&LoggerConfig::path, "Path", rcfg::NotEmpty);
	p.member(&LoggerConfig::severity, "Severity", rcfg::Bounds{0, 6}, rcfg::Default{4}, rcfg::Updatable);

	rcfg::MapParser<std::string, LoggerConfig> p2(p);
	return p2;
}

const char * jsonStr = R"(
{
	"Root":
	{
		"Path" : "/root",
		"Severity": 5
	},
	"Engine":
	{
		"Path" : "/app/engine"
	},
	"Net":
	{
		"Path" : "/app/net",
		"Severity": 3
	}
}
)";

const char * jsonStr2 = R"(
{
	"Root":
	{
		"Path" : "/root"
	},
	"Engine":
	{
		"Path" : "/app/engine"
	},
	"Net":
	{
		"Path" : "/app/net",
		"Severity": 2
	}
}
)";

template<typename C, typename Parser>
void Parse(C & c, const Parser & parser, const char * jsonStr, bool isUpdate)
{
	const auto j = nlohmann::json::parse(jsonStr);
	rcfg::LoggerSink sink([] (const std::string & msg)
	{
		std::cout << msg << std::endl;
	});
	parser.parse(sink, c, j, isUpdate);
	if (sink.IsError())
		std::cout << "Parsed with errors!" << std::endl;
}

int main()
{
	LoggersMap l{};
	std::cout << "Read config from json:\n\"\"\"\n" << jsonStr << "\"\"\"" << std::endl;
	Parse(l, GetParser(), jsonStr, false);
	// Update config
	std::cout << "\nUpdate config from json:\n\"\"\"\n" << jsonStr2 << "\"\"\"" << std::endl;
	Parse(l, GetParser(), jsonStr2, true);

	nlohmann::json j;
	GetParser().dump(l, j);
	std::cout << j << std::endl;
}

/*
Program output:

Read config from json:
"""

{
	"Root":
	{
		"Path" : "/root",
		"Severity": 5
	},
	"Engine":
	{
		"Path" : "/app/engine"
	},
	"Net":
	{
		"Path" : "/app/net",
		"Severity": 3
	}
}
"""
+Engine.Path=/app/engine
+Engine.Severity=4 (defalut)
+Net.Path=/app/net
+Net.Severity=3
+Root.Path=/root
+Root.Severity=5

Update config from json:
"""

{
	"Root":
	{
		"Path" : "/root"
	},
	"Engine":
	{
		"Path" : "/app/engine"
	},
	"Net":
	{
		"Path" : "/app/net",
		"Severity": 2
	}
}
"""
+Net.Severity=3->2
+Root.Severity=5->4 (defalut)
*/