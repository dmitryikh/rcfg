#pragma once
#include <iostream>
#include <rcfg/rcfg.h>

class TestEventSink : public rcfg::ISink
{
public:
    void Push(const std::string & key) {};
    void Pop() {};

    void Error(const std::string & error) { errorCount++; };
    void NotUpdatable(const std::string & old, const std::string & neww) { notUpdetableCount++; };
    void Changed(const std::string & old, const std::string & neww, const bool isDefault) { changedCount++; };
    void Set(const std::string & value, const bool isDefault) { setCount++; };

    uint64_t errorCount = 0;
    uint64_t notUpdetableCount = 0;
    uint64_t changedCount = 0;
    uint64_t setCount = 0;
};