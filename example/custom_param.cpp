#include <string>
#include <fstream>

#include <rcfg/rcfg.h>
#include <rcfg/sink.h>
#include <nlohmann/json.hpp>

using namespace std::literals;

enum class Severity
{
    Debug = 1,
    Info = 2,
    Warning = 3,
    Error = 4,
    Critical = 5
};

namespace rcfg
{
    // In order to use custom type as config parameter one needs to define
    // ParamTrait<P> specialization
    template<>
    struct ParamTrait<Severity>
    {
        // Parse value from Node
        static Severity From(const Node & node)
        {
            std::string repr = node.template get<std::string>();
            if (repr == "Debug")
                return Severity::Debug;
            else if (repr == "Info")
                return Severity::Info;
            else if (repr == "Warning")
                return Severity::Warning;
            else if (repr == "Error")
                return Severity::Error;
            else if (repr == "Critical")
                return Severity::Critical;

            throw std::runtime_error("invalid severity "s + repr);
        }

        // Write value to Node
        static void To(const Severity & p, Node & node)
        {
            node = ToString(p);
        }

        // Get text representation of the value
        static std::string ToString(const Severity & p)
        {
            switch (p)
            {
                case Severity::Debug:
                    return "Debug";
                case Severity::Info:
                    return "Info";
                case Severity::Warning:
                    return "Warning";
                case Severity::Error:
                    return "Error";
                case Severity::Critical:
                    return "Critical";
            }
        }
    };
}

struct LoggerConfig
{
    std::string dir;
    Severity severity;
};

// Initialize parser with rules
auto GetParser()
{
    rcfg::ClassParser<LoggerConfig> p;
    p.member(&LoggerConfig::dir, "Dir", rcfg::NotEmpty);
    p.member(&LoggerConfig::severity, "Severity", rcfg::Default{Severity::Info});
    return p;
}

const char * jsonStr = R"(
{
    "Dir" : "/c/abs/path",
    "Severity": "Debug"
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
    LoggerConfig l{};
    std::cout << "Read config from json:\n\"\"\"\n" << jsonStr << "\"\"\"" << std::endl;
    Parse(l, GetParser(), jsonStr, false);
    // Update config

    std::cout << "\nDump config back to json:" << std::endl;
    nlohmann::json j;
    GetParser().dump(l, j);
    std::cout << j << std::endl;
}

/*
Program output:

Read config from json:
"""

{
    "Dir" : "/c/abs/path",
    "Severity": "Debug"
}
"""
+Dir=/c/abs/path
+Severity=Debug

Dump config back to json:
{"Dir":"/c/abs/path","Severity":"Debug"}
*/
