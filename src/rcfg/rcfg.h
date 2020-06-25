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

	struct SecretTag {};
	constexpr SecretTag Secret;

	template<typename P>
	struct Default
	{
		Default(const P & p)
			: p(p)
		{}

		P p;
	};

	template<typename P>
	struct IParser
	{
		virtual void parse(ISink & sink, P & c, const Node & node, bool isUpdate) const = 0;
		virtual void dump(const P & c, Node & node) const = 0;
		virtual void remove(ISink & sink, const P & c) const = 0;
		virtual ~IParser() = default;
	};

	template<typename P>
	class Parser
	{
		using Type = P;
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

		void remove(ISink & sink, const P & p) const
		{
			_parserImpl->remove(sink, p);
		}

	private:
		IParserSPtr _parserImpl;
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

			const auto ppStr = toString(pp);
			if (p != pp && isUpdate)
			{
				if (isUpdatable)
				{
					sink.Changed(toString(p), ppStr, isDefault);
					p = pp;
				}
				else if (isUpdate)
				{
					sink.NotUpdatable(toString(p), ppStr);
				}
			}
			else if (isUpdate) {}
			else
			{
				sink.Set(ppStr, isDefault);
				p = pp;
			}
		}

		std::string toString(const P & p) const
		{
			if (isSecret)
				return "***";
			else
				return ParamTrait<P>::ToString(p);
		}

		void dump(const P & p, Node & node) const override
		{
			if (isSecret)
				ParamTrait<std::string>::To("***", node);
			else
				ParamTrait<P>::To(p, node);
		}

		void remove(ISink & sink, const P & p) const override
		{
			sink.Remove(toString(p));
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

		void addOp(const SecretTag &)
		{
			isSecret = true;
		}

		template<typename Op>
		auto addOp(Op && op) -> std::enable_if_t<std::is_base_of_v<CheckOpBase, std::decay_t<Op>>>
		{
			checkFuncs.emplace_back(std::forward<Op>(op));
		}

		std::string name;
		std::optional<P> def;
		bool isUpdatable = false;
		bool isSecret = false;
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

		void remove(ISink & sink, const Map & c) const override
		{
			for (const auto & [key, value] : c)
			{
				sink.Push(FromKey(key));
				parser.remove(sink, value);
				sink.Pop();
			}
		}

		Parser<P> parser;
	};

	template<typename P, template <class, class ... > class VectorType = std::vector>
	struct VectorParser : public IParser<VectorType<P>>
	{
		using Vector = VectorType<P>;
	public:
		VectorParser(Parser<P> parser = ParamParser<P>())
			: parser(std::move(parser))
		{}

		void parse(ISink & sink, Vector & c, const Node & node, bool isUpdate) const override
		{
			if (!node.is_array())
			{
				sink.Error("Expecting array");
				return;
			}

			if (c.size() != node.size())
			{
				c.clear();
				c.resize(node.size(), {});
			}

			size_t i = 0;
			for (auto it = node.begin(); it != node.end(); ++it, ++i) {
				sink.Push(std::to_string(i));
				parser.parse(sink, c[i], *it, isUpdate);
				sink.Pop();
			}
		}

		void dump(const Vector & c, Node & node) const override
		{
			size_t i = 0;
			for (const auto & value : c)
			{
				node.emplace_back();
				parser.dump(value, node[i]);
				i++;
			}
		}

		void remove(ISink & sink, const Vector & c) const override
		{
			removeInternal(sink, c, 0);
		}

		void removeInternal(ISink & sink, const Vector & c, const size_t start) const
		{
			for (size_t i = start; i < c.size(); i++)
			{
				sink.Push(std::to_string(i));
				parser.remove(sink, c[i]);
				sink.Pop();
			}
		}

		Parser<P> parser;
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
			if (name.empty())
			{
				// It makes possible parse the case like this:
				// struct A { int i1; };
				// struct B { A a; int i2;}
				// json : `{"i1": 1, "i2": 2}`
				parser.parse(sink, c.*ptr, node, isUpdate);
				return;
			}

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

		void remove(ISink & sink, const C & c) const override
		{
			if (!name.empty())
				sink.Push(name);

			parser.remove(sink, c.*ptr);

			if (!name.empty())
				sink.Pop();
		}

		std::string name;
		P C::* ptr;
		Parser<P> parser;
	};

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
			if constexpr (utils::IsVectorContainer<P>::value)
			{
				// syntactic sugar to automatically determaine std::vector class
				// member and use VectorParser implicitly
				using value_type = typename utils::ContainerTrait<P>::value_type;
				member(dest, name, VectorParser<value_type>(ParamParser<value_type>(std::forward<Ops>(ops)...)));
			}
			else if constexpr (utils::IsMapContainer<P>::value)
			{
				// syntactic sugar to automatically determaine std::map/unordered_map
				// member and use MapParser implicitly
				using value_type = typename utils::ContainerTrait<P>::value_type;

				using MapParserType = MapParser<
					typename utils::ContainerTrait<P>::key_type,
					value_type,
					utils::ContainerTrait<P>::template container>;

				member(dest, name, MapParserType(ParamParser<value_type>(std::forward<Ops>(ops)...)));
			}
			else
			{
				member(dest, name, ParamParser<P>(std::forward<Ops>(ops)...));
			}
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

		void remove(ISink & sink, const C & c) const override
		{
			for (auto & paramParser : paramParsers)
				paramParser.remove(sink, c);
		}

		std::vector<Parser<C>> paramParsers;
	};

}