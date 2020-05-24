#include <string>
#include <filesystem>
#include <fstream>

#include <rcfg/rcfg.h>
#include <rcfg/sink.h>
#include <nlohmann/json.hpp>

using namespace std::literals;

// Destination config
struct Config
{
    std::string dir;
    uint64_t severity;
    bool feature;
    std::string name;
    double velocity;
    std::string password;
};

// Initialize parser with rules
auto GetParser()
{
    rcfg::ClassParser<Config> p;
    p.member(&Config::dir, "Dir", rcfg::NotEmpty);
    p.member(&Config::severity, "Severity", rcfg::Bounds{0, 6}, rcfg::Default{4});
    p.member(&Config::feature, "Feature");
    p.member(&Config::name, "Name", rcfg::NotEmpty, rcfg::Default("MyName"s), rcfg::Updatable);
    p.member(&Config::velocity, "Vel", rcfg::Bounds{0.0, 100.0});
    // secret means that the field value won't be revealed after reading
    p.member(&Config::password, "Password", rcfg::NotEmpty, rcfg::Secret);
    return p;
}

const char * jsonStr = R"(
{
    "Dir" : "/c/abs/path",
    "Feature": true,
    "Vel": 99.0,
    "Password": "secret"
}
)";

const char * jsonStr2 = R"(
{
    "Dir" : "/c/abs/path",
    "Feature": true,
    "Vel": 70.0,
    "Name": "Monk",
    "Password": "new secret"
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
    Config c{};
    std::cout << "Read config from json:\n\"\"\"\n" << jsonStr << "\"\"\"" << std::endl;
    Parse(c, GetParser(), jsonStr, false);
    // Update config
    std::cout << "\nUpdate config from json:\n\"\"\"\n" << jsonStr2 << "\"\"\"" << std::endl;
    Parse(c, GetParser(), jsonStr2, true);
}

/*
Program output:

Read config from json:
"""

{
    "Dir" : "/c/abs/path",
    "Feature": true,
    "Vel": 99.0,
    "Password": "secret"
}
"""
+Dir=/c/abs/path
+Severity=4 (defalut)
+Feature=1
+Name=MyName (defalut)
+Vel=99.000000
+Password=***

Update config from json:
"""

{
    "Dir" : "/c/abs/path",
    "Feature": true,
    "Vel": 70.0,
    "Name": "Monk",
    "Password": "new secret"
}
"""
+Name=MyName->Monk
!Vel changed 99.000000->70.000000 but will make effect only after RESTART
!Password changed ***->*** but will make effect only after RESTART
*/