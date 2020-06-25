#pragma once

namespace rcfg::utils
{
	template <typename CONT>
	std::string join(const CONT & c, const std::string sep)
	{
		std::string cSep;
		std::string result;
		for (const auto & token : c)
		{
			result += cSep + token;
			if (cSep.empty())
				cSep = sep;
		}
		return result;
	}

	template<typename T>
	struct IsVectorContainer : std::false_type{};
	template<typename... Args>
	struct IsVectorContainer<std::vector<Args...>> : std::true_type{};


	template<typename Cont>
	struct ContainerValueType
	{
		using value_type = Cont;
	};

	template<typename ValueType, typename... Args>
	struct ContainerValueType<std::vector<ValueType, Args...>>
	{
		using value_type = ValueType;
	};
}