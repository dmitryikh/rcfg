#pragma once

#include <sstream>
#include <string>
#include <functional>

#include "utils.h"

namespace rcfg
{
    // ISink object is used to notify client code about events due parsing
	class ISink
	{
	public:
        // Go to parse nested key
		virtual void Push(const std::string & key) {};
        // Go one level up from nested key
		virtual void Pop() {};

        // Notify about parsing error
		virtual void Error(const std::string & error) {};
        // Notify about a try to modify not updatable parameter during parse with update
		virtual void NotUpdatable(const std::string & old, const std::string & neww) {};
		virtual void Changed(const std::string & old, const std::string & neww, const bool isDefault) {};
		virtual void Set(const std::string & value, const bool isDefault) {};

		virtual ~ISink() = default;
	};

    // SimpleSink that log every event to `LogFunc` and keep error flag
    class LoggerSink : public ISink
    {
    public:
        using LogFunc = std::function<void(std::string string)>;

        LoggerSink(LogFunc logFunc)
            : _logFunc(std::move(logFunc))
        {}

		void Push(const std::string & key) override
		{
			_keys.push_back(key);
		}

		void Pop() override
		{
			_keys.pop_back();
		}

		void Error(const std::string & error) override
		{
            _isError = true;
            std::stringstream ss;
			ss << "!!!" << Key() << ": " << error;
            _logFunc(std::move(ss).str());
		}

		void NotUpdatable(const std::string & old, const std::string & neww) override
		{
            std::stringstream ss;
			ss << "!" << Key() << " changed " << old << "->" << neww
				<< " but will make effect only after RESTART";
            _logFunc(std::move(ss).str());
		}

		void Changed(const std::string & old, const std::string & neww, const bool isDefault) override
		{
            std::stringstream ss;
			ss << "+" << Key() << "=" << old << "->" << neww
				<< (isDefault ? " (defalut)" : "");
            _logFunc(std::move(ss).str());
		}

		void Set(const std::string & value, const bool isDefault) override
		{
            std::stringstream ss;
			ss << "+" << Key() << "=" << value
				<< (isDefault ? " (defalut)" : "");
            _logFunc(std::move(ss).str());
		}

        bool IsError() const
        {
            return _isError;
        }

    private:
        std::string Key() const
        {
            return utils::join(_keys, ".");
        }

        bool _isError = false;
		std::vector<std::string> _keys;
        LogFunc _logFunc;
    };
}
