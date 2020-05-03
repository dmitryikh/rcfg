#pragma once

#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <nlohmann/json.hpp>

#include "check.h"
#include "sink.h"

namespace rcfg
{
	using Node = nlohmann::json;

	// Methods to parse the parameter from Node, get string representation, etc.
	template<typename P>
	struct ParamTrait
	{
		static P From(const Node & node)
		{
			return node.template get<P>();
		}

		static void To(const P & p, Node & node)
		{
			node = p;
		}

		static std::string ToString(const P & p)
		{
			return std::to_string(p);
		}
	};

	template<>
	inline std::string ParamTrait<std::string>::ToString(const std::string & p)
	{
		return p;
	}

	struct UpdatableTag {};
	constexpr UpdatableTag Updatable;

	template<typename P>
	struct Default
	{
		Default(const P & p)
			: p(p)
		{}

		P p;
	};

	template<typename C>
	struct IParser
	{
		virtual void parse(ISink & sink, C & c, const Node & node, bool isUpdate) const = 0;
		virtual void dump(const C & c, Node & node) const = 0;
		virtual ~IParser() = default;
	};

	template<typename P>
	class Parser
	{
		using IParserSPtr = std::shared_ptr<IParser<P>>;
	public:
		template<typename ParserType>
		Parser(ParserType && parser)
		{
			using T = std::decay_t<ParserType>;
			if constexpr (std::is_same_v<T, Parser>)
			{
				*this = std::forward<ParserType>(parser);
			}
			else
			{
				static_assert(std::is_base_of_v<IParser<P>, T>, "parser should be derived from IParser<P>");
				_parserImpl = std::make_shared<T>(std::forward<ParserType>(parser));
			}
		}

		void parse(ISink & sink, P & p, const Node & node, bool isUpdate) const
		{
			return _parserImpl->parse(sink, p, node, isUpdate);
		}

		void dump(const P & p, Node & node) const
		{
			_parserImpl->dump(p, node);
		}

	private:
		IParserSPtr _parserImpl;
	};

	template<typename C, typename P>
	struct MemberParser : public IParser<C>
	{
	private:
		using checkFunc = std::function<void(const P & p)>;

	public:
		MemberParser(P C::* ptr, const std::string & name, Parser<P> parser)
			: name(name)
			, ptr(ptr)
			, parser(std::move(parser))
		{}

		void parse(ISink & sink, C & c, const Node & node, bool isUpdate) const override
		{
			sink.Push(name);
			if (node.count(name) > 0)
			{
				parser.parse(sink, c.*ptr, node[name], isUpdate);
			}
			else
			{
				parser.parse(sink, c.*ptr, Node{}, isUpdate);
			}
			sink.Pop();
		}

		void dump(const C & c, Node & node) const override
		{
			parser.dump(c.*ptr, node[name]);
		}

		std::string name;
		P C::* ptr;
		Parser<P> parser;
	};

	template<typename P>
	struct ParamParser : public IParser<P>
	{
	private:
		using checkFunc = std::function<void(const P & p)>;

	public:
		template<typename... Ops>
		ParamParser(Ops&&... ops)
		{
			(addOp(std::forward<Ops>(ops)), ...);
		}

		void parse(ISink & sink, P & p, const Node & node, bool isUpdate) const override
		{
			P pp;
			bool isDefault = false;
			if (!node.is_null())
			{
				try
				{
					pp = ParamTrait<P>::From(node);
				}
				catch(const std::exception& ex)
				{
					sink.Error(std::string("parsing error: ") + ex.what());
					return;
				}
			}
			else
			{
				if (!def)
				{
					sink.Error("required parameter not found");
					return;
				}

				pp = *def;
				isDefault = true;
			}

			bool checkErrors = false;
			for (const auto & f : checkFuncs)
			{
				try
				{
					f(pp);
				}
				catch(const std::exception& ex)
				{
					sink.Error(ex.what());
					checkErrors = true;
				}
			}

			if (checkErrors)
				return;

			const auto ppStr = ParamTrait<P>::ToString(pp);
			if (p != pp && isUpdate)
			{
				if (isUpdatable)
				{
					const auto pStr = ParamTrait<P>::ToString(p);
					sink.Changed(pStr, ppStr, isDefault);
					p = pp;
				}
				else if (isUpdate)
				{
					const auto pStr = ParamTrait<P>::ToString(p);
					sink.NotUpdatable(pStr, ppStr);
				}
			}
			else if (isUpdate) {}
			else
			{
				sink.Set(ppStr, isDefault);
				p = pp;
			}
		}

		void dump(const P & p, Node & node) const override
		{
			ParamTrait<P>::To(p, node);
		}

		template<typename P2>
		void addOp(const Default<P2> & val)
		{
			// TODO: enable if P2 -> P
			def = val.p;
		}

		void addOp(const UpdatableTag &)
		{
			isUpdatable = true;
		}

		template<typename Op>
		auto addOp(Op && op) -> std::enable_if_t<std::is_base_of_v<CheckOpBase, std::decay_t<Op>>>
		{
			checkFuncs.emplace_back(std::forward<Op>(op));
		}

		std::string name;
		std::optional<P> def;
		bool isUpdatable = false;
		std::vector<checkFunc> checkFuncs;
	};

	template<typename K, typename P, template <class, class, class ... > class MapType = std::map>
	struct MapParser : public IParser<MapType<K, P>>
	{
		using Map = MapType<K, P>;
	public:
		MapParser(Parser<P> parser = ParamParser<P>())
			: parser(std::move(parser))
		{}

		static K ToKey(const std::string & key)
		{
			return K{ key };
		}

		static std::string FromKey(const K & key)
		{
			return ParamTrait<K>::ToString(key);
		}

		void parse(ISink & sink, Map & c, const Node & node, bool isUpdate) const override
		{
			for (const auto & item : node.items())
			{
				sink.Push(item.key());
				parser.parse(sink, c[ToKey(item.key())], item.value(), isUpdate);
				sink.Pop();
			}
		}

		void dump(const Map & c, Node & node) const override
		{
			for (const auto & [key, value] : c)
			{
				parser.dump(value, node[FromKey(key)]);
			}
		}

		Parser<P> parser;
	};

	// template<typename P, template <class, class ... > class VectorType = std::vector>
	// struct VectorParser : public IParser<VectorType<P>>
	// {
	// 	using Vector = VectorType<P>;
	// public:
	// 	VectorParser(Parser<P> parser = ParamParser<P>())
	// 		: parser(std::move(parser))
	// 	{}

	// 	void parse(ISink & sink, Vector & c, const Node & node, bool isUpdate) const override
	// 	{
	// 		if (!node.is_array())
	// 		{
	// 			sink.Error("Expecting array");
	// 			return;
	// 		}

	// 		if (c.size() != node.size())
	// 		{
	// 			c.clear();
	// 			c.resize(node.size(), {});
	// 		}

	// 		for (auto it = node.begin(), size_t i = 0; it != node.end(); ++it, ++i) {
	// 			sink.Push(std::to_string(i));
	// 			parser.parse(sink, c[i], *it, isUpdate);
	// 			sink.Pop();
	// 		}
	// 	}

	// 	void dump(const Vector & c, Node & node) const override
	// 	{
	// 		for (const auto & value : c)
	// 		{
	// 			parser.dump(value, node.push_back(P{}));
	// 		}
	// 	}

	// 	Parser<P> parser;
	// };

	template<typename C>
	class ClassParser : public IParser<C>
	{
	public:
		template<typename P, typename ParserType>
		auto member(P C::* dest, const std::string & name, ParserType parser)
			-> std::enable_if_t<std::is_base_of_v<IParser<P>, std::decay_t<ParserType>>>
		{
			auto d = MemberParser<C, P>(dest, name, std::move(parser));
			paramParsers.emplace_back(std::move(d));
		}

		template<typename P, typename... Ops>
		void member(P C::* dest, const std::string & name, Ops&&... ops)
		{
			member(dest, name, ParamParser<P>(std::forward<Ops>(ops)...));
		}

		void parse(ISink & sink, C & c, const Node & node, bool isUpdate) const override
		{
			for (auto & paramParser : paramParsers)
				paramParser.parse(sink, c, node, isUpdate);
		}

		void dump(const C & c, Node & node) const override
		{
			for (auto & paramParser : paramParsers)
				paramParser.dump(c, node);
		}

		std::vector<Parser<C>> paramParsers;
	};

}