#pragma once

namespace rcfg
{
	class invalid_parameter : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	class CheckOpBase
	{ };

	struct NotEmptyType : public CheckOpBase
	{
		void operator() (const std::string & s)
		{
			if (s.empty())
				throw invalid_parameter("should be not empty");
		}
	};
	constexpr NotEmptyType NotEmpty;

	template<typename T>
	struct Bounds : public CheckOpBase
	{
		Bounds(T lower, T upper)
			: lower(lower)
			, upper(upper)
		{}

		void operator() (T v)
		{
			if (v < lower || v > upper)
				throw invalid_parameter("should be in bounds ["
					+ std::to_string(lower) + ";" + std::to_string(upper) + "]");
		}
		T lower, upper;
	};

	template<typename T>
	struct LowerBound : public CheckOpBase
	{
		LowerBound(T lower)
			: lower(lower)
		{}

		void operator() (T v)
		{
			if (v < lower)
				throw invalid_parameter("should be >= " + std::to_string(lower));
		}
		T lower;
	};

	template<typename T>
	struct UpperBound : public CheckOpBase
	{
		UpperBound(const T upper)
			: upper(upper)
		{}

		void operator() (T v)
		{
			if (v > upper)
				throw invalid_parameter("should be <= " + std::to_string(upper));
		}
		T upper;
	};
}
