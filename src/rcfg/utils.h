#pragma once

#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>

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

	template<typename T>
	struct IsMapContainer : std::false_type{};
	template<typename... Args>
	struct IsMapContainer<std::map<Args...>> : std::true_type{};
	template<typename... Args>
	struct IsMapContainer<std::unordered_map<Args...>> : std::true_type{};

	template<typename T>
	struct IsSetContainer : std::false_type{};
	template<typename... Args>
	struct IsSetContainer<std::set<Args...>> : std::true_type{};
	template<typename... Args>
	struct IsSetContainer<std::unordered_set<Args...>> : std::true_type{};


	template<typename Cont>
	struct ContainerTrait
	{
		using value_type = Cont;
		using container = void;
	};

	template<typename ValueType, typename... Args>
	struct ContainerTrait<std::vector<ValueType, Args...>>
	{
		using value_type = ValueType;
		template<typename V, typename... Args2>
		using container = std::vector<V, Args2...>;
	};

	template<typename KeyType, typename ValueType, typename... Args>
	struct ContainerTrait<std::map<KeyType, ValueType, Args...>>
	{
		using key_type = KeyType;
		using value_type = ValueType;

		template<typename K, typename V, typename... Args2>
		using container = std::map<K, V, Args2...>;
	};

	template<typename KeyType, typename ValueType, typename... Args>
	struct ContainerTrait<std::unordered_map<KeyType, ValueType, Args...>>
	{
		using key_type = KeyType;
		using value_type = ValueType;

		template<typename K, typename V, typename... Args2>
		using container = std::unordered_map<K, V, Args2...>;
	};

	template<typename ValueType, typename... Args>
	struct ContainerTrait<std::set<ValueType, Args...>>
	{
		using value_type = ValueType;
		template<typename V, typename... Args2>
		using container = std::set<V, Args2...>;
	};

	template<typename ValueType, typename... Args>
	struct ContainerTrait<std::unordered_set<ValueType, Args...>>
	{
		using value_type = ValueType;
		template<typename V, typename... Args2>
		using container = std::unordered_set<V, Args2...>;
	};
}