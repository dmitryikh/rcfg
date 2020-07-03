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
		// Notify about value removal while updating config. It's needed for vector and maps
		virtual void Remove(const std::string & value) {};

		virtual ~ISink() = default;
	};

	class VoidSink : public ISink
	{
	public:
		bool IsError() const { return isError; }
		void Error(const std::string & error) override { isError = true;}
	private:
		bool isError = false;
	};

	// SimpleSink that log every event to `LogFunc` and keep error flag
	class LoggerSink : public ISink
	{
	public:
		using LogFunc = std::function<void(std::string string)>;

		LoggerSink(LogFunc logInfoFunc)
			: _logInfoFunc(std::move(logInfoFunc))
		{}

		LoggerSink(LogFunc logInfoFunc, LogFunc logWarningFunc, LogFunc logErrorFunc)
			: _logInfoFunc(std::move(logInfoFunc))
			, _logWarningFunc(std::move(logWarningFunc))
			, _logErrorFunc(std::move(logErrorFunc))
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
			if (_logErrorFunc)
				_logErrorFunc(std::move(ss).str());
			else
				_logInfoFunc(std::move(ss).str());
		}

		void NotUpdatable(const std::string & old, const std::string & neww) override
		{
			std::stringstream ss;
			ss << "!" << Key() << " changed " << old << "->" << neww
				<< " but will make effect only after RESTART";
			if (_logWarningFunc)
				_logWarningFunc(std::move(ss).str());
			else
				_logInfoFunc(std::move(ss).str());
		}

		void Changed(const std::string & old, const std::string & neww, const bool isDefault) override
		{
			std::stringstream ss;
			ss << "+" << Key() << "=" << old << "->" << neww
				<< (isDefault ? " (default)" : "");
			_logInfoFunc(std::move(ss).str());
		}

		void Set(const std::string & value, const bool isDefault) override
		{
			std::stringstream ss;
			ss << "+" << Key() << "=" << value
				<< (isDefault ? " (default)" : "");
			_logInfoFunc(std::move(ss).str());
		}

		void Remove(const std::string & value) override
		{
			std::stringstream ss;
			ss << "-" << Key() << "=" << value;
			_logInfoFunc(std::move(ss).str());
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
		LogFunc _logInfoFunc;
		LogFunc _logWarningFunc = nullptr;
		LogFunc _logErrorFunc = nullptr;
	};
}
