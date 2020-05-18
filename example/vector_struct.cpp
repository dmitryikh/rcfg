#include <string>
#include <filesystem>
#include <fstream>

#include <rcfg/rcfg.h>
#include <rcfg/sink.h>
#include <nlohmann/json.hpp>

using namespace std::literals;

struct LoggerConfig
{
    std::string name;
    std::string path;
    uint64_t severity;
};

using LoggersVector = std::vector<LoggerConfig>;

// Initialize parser with rules
auto GetParser()
{
    rcfg::ClassParser<LoggerConfig> p;
    p.member(&LoggerConfig::name, "Name", rcfg::NotEmpty);
    p.member(&LoggerConfig::path, "Path", rcfg::Default{"/var/lib/log/"s});
    p.member(&LoggerConfig::severity, "Severity", rcfg::Bounds{0, 6}, rcfg::Default{4}, rcfg::Updatable);

    rcfg::VectorParser<LoggerConfig> p2(p);
    return p2;
}

const char * jsonStr = R"(
[
    {
        "Name": "Root",
        "Path" : "/root",
        "Severity": 5
    },
    {
        "Name": "Engine",
        "Path" : "/app/engine"
    },
    {
        "Name": "Net",
        "Severity": 3
    }
]
)";

const char * jsonStr2 = R"(
[
    {
        "Name": "Root",
        "Path" : "/root",
        "Severity": 3
    },
    {
        "Name": "Engine",
        "Path" : "/app/engine"
    },
    {
        "Name": "Net"
    }
]
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
    LoggersVector l{};
    std::cout << "Read config from json:\n\"\"\"\n" << jsonStr << "\"\"\"" << std::endl;
    Parse(l, GetParser(), jsonStr, false);
    // Update config
    std::cout << "\n\nUpdate config from json:\n\"\"\"\n" << jsonStr2 << "\"\"\"" << std::endl;
    Parse(l, GetParser(), jsonStr2, true);

    nlohmann::json j;
    GetParser().dump(l, j);
    std::cout << "\n\nDump config back to json:" << std::endl;
    std::cout << j.dump(4) << std::endl;
}

/*
Program output:

Read config from json:
"""

[
    {
        "Name": "Root",
        "Path" : "/root",
        "Severity": 5
    },
    {
        "Name": "Engine",
        "Path" : "/app/engine"
    },
    {
        "Name": "Net",
        "Severity": 3
    }
]
"""
+0.Name=Root
+0.Path=/root
+0.Severity=5
+1.Name=Engine
+1.Path=/app/engine
+1.Severity=4 (defalut)
+2.Name=Net
+2.Path=/var/lib/log/ (defalut)
+2.Severity=3


Update config from json:
"""

[
    {
        "Name": "Root",
        "Path" : "/root",
        "Severity": 3
    },
    {
        "Name": "Engine",
        "Path" : "/app/engine"
    },
    {
        "Name": "Net"
    }
]
"""
+0.Severity=5->3
+2.Severity=3->4 (defalut)


Dump config back to json:
[
    {
        "Name": "Root",
        "Path": "/root",
        "Severity": 3
    },
    {
        "Name": "Engine",
        "Path": "/app/engine",
        "Severity": 4
    },
    {
        "Name": "Net",
        "Path": "/var/lib/log/",
        "Severity": 4
    }
]
*/