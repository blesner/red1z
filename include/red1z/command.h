// -*- C++ -*-
#ifndef RED1Z_COMMAND_H
#define RED1Z_COMMAND_H

#include "error.h"
#include "io.h"
#include "reply.h"
#include "flags.h"
#include "args.h"

#include <memory>
#include <vector>
#include <stdexcept>
#include <string>
#include <optional>
#include <array>
#include <map>
#include <sstream>
#include <cstdlib>

using namespace std::literals::string_literals;
namespace red1z {
  namespace impl {
    template <class... Args>
    static constexpr auto args_indices_v = std::make_index_sequence<sizeof...(Args)>();

    /// access type from a pack by index
    ///from https://ldionne.com/2015/11/29/efficient-parameter-pack-indexing/
    template <std::size_t I, typename T>
    struct indexed {
      using type = T;
    };

    template <typename Is, typename ...Ts>
    struct indexer;

    template <std::size_t ...Is, typename ...Ts>
    struct indexer<std::index_sequence<Is...>, Ts...> : indexed<Is, Ts>... { };

    template <std::size_t I, typename T>
    static indexed<I, T> select(indexed<I, T>);

    template <std::size_t I, typename ...Ts>
    using nth_element =
      typename decltype(select<I>(indexer<std::index_sequence_for<Ts...>, Ts...>{}))::type;


    template <class Cvt, int I, class Arg0, class... Args>
    void encode_args(Cvt const & cvt, std::integral_constant<int, I>,
                     Encoder& e, Arg0 const& arg0, Args const&... args)
    {
      e.encode(Cvt::template cvt<I>(arg0));
      encode_args(cvt, std::integral_constant<int, I+1>(), e, args...);
    }


    template <class Cvt, int I, class Tag, class... Flag, class... Args>
    void encode_args(Cvt const & cvt, std::integral_constant<int, I>,
                    Encoder& e, impl::flag<Tag, Flag...> const& flag, Args const&... args)
    {
      flag.encode(e);
      encode_args(cvt, std::integral_constant<int, I+1>(), e, args...);
    }

    template <class Cvt, int I, class It1, class It2, class... Args>
    void encode_args(Cvt const& cvt, std::integral_constant<int, I>, Encoder& e,
                     ArgPack<It1, It2> const& pack, Args const&... args) {
      pack.encode(cvt, e);
      encode_args(cvt, std::integral_constant<int, I+1>(), e, args...);
    }

    template <class Cvt, int I>
    void encode_args(Cvt const&, std::integral_constant<int, I>, Encoder&) {}


    template <class Arg>
    constexpr int arg_size(Arg const&) { return 1; }

    template <class Tag, class... Flag>
    int arg_size(flag<Tag, Flag...> const& f) {
      return f.size();
    }

    template <class It1, class It2>
    int arg_size(ArgPack<It1, It2> const& pack) {
      return pack.size() * ArgPack<It1, It2>::value_size;
    }

    template <class Cvt=converter<>, class... Args>
    std::string encode_command(Args const&... args) {
      Encoder e((arg_size(args) + ...));
      encode_args(Cvt(), std::integral_constant<int, -1>(), e, args...);
      return e.value();
    }

    struct auto_t {};
    template <class V, class T> struct auto_type { using type = V; };
    template <class T> struct auto_type<auto_t, T> { using type = T; };
    template <class V, class T> using auto_type_t = typename auto_type<V, T>::type;


    template <class Derived>
    class Command {
      std::string m_cmd;
    public:
      struct raw {};

      template <class Pattern, class Cvt, int Skip, class... Args>
      Command(converter<Pattern, Cvt, Skip> const&, std::string_view cmd, Args const&... args) :
        m_cmd(encode_command<converter<Pattern, Cvt, Skip>>(cmd, args...))
      {
      }

      template <class... Args>
      Command(std::string_view cmd, Args const&... args) :
        Command(converter<>(), cmd, args...)
      {
      }

      Command(raw, std::string cmd) : m_cmd(std::move(cmd)) {}

      Command(const Command&) = default;
      Command(Command&&) = default;

      std::string const& cmd() const & { return m_cmd; }

      std::string cmd() && { return std::move(m_cmd); }

      Derived const& derived() const {
        return *static_cast<Derived const*>(this);
      }

      decltype(auto) process(Reply&& r) const {
        return derived().process(std::move(r));
      }

      template <class Out>
      decltype(auto) process_into(Reply&& r, Out out) const {
        return derived().process_into(std::move(r), out);
      }
     };

    struct SimpleStringCommand : Command<SimpleStringCommand> {
      using Command<SimpleStringCommand>::Command;

      static std::string process(Reply&& r) {
        return std::move(r).string();
      }

      template <class V=void>
      static std::string& process_into(Reply&& r, std::string* out) {
        return *out = std::move(r).string();
      }
    };

    struct StatusCommand : Command<StatusCommand> {
      using Command<StatusCommand>::Command;

      static bool process(Reply&& r) {
        return r.ok();
      }

      template <class V=void>
      static bool process_into(Reply&& r, bool* out) {
        return *out = r.ok();
      }
    };

    struct IntegerCommand : Command<IntegerCommand> {
      using Command<IntegerCommand>::Command;

      static std::int64_t process(Reply&& r) {
        return r.integer();
      }

      template <class V=void, class T>
      static std::int64_t process_into(Reply&& r, T* out) {
        //ASSERT is_integral<T> ?
        return *out = r.integer();
      }
    };

    struct OptionalIntegerCommand : Command<OptionalIntegerCommand> {
      using Command<OptionalIntegerCommand>::Command;

      static std::optional<std::int64_t> process(Reply&& r) {
        if (r) {
          return r.integer();
        }
        return std::nullopt;
      }

      template <class T>
      static bool process_into(Reply&& r, T* out) {
        //ASSERT is_integral<T> ?
        if (r) {
          *out = r.integer();
          return true;
        }
        return false;
      }
    };

    struct FloatCommand : Command<FloatCommand> {
      using Command<FloatCommand>::Command;

      static double process(Reply&& r) {
        return r.floating_point();
      }

      static double process_into(Reply&& r, double* out) {
        return *out = r.floating_point();
      }

      static double process_into(Reply&& r, float* out) {
        auto const fp = r.floating_point();
        *out = fp;
        return fp;
      }
    };


    template <class V>
    struct BulkStringCommand : Command<BulkStringCommand<V>> {
      using Command<BulkStringCommand<V>>::Command;
      using T = auto_type_t<V, std::string>;

      static std::optional<T> process(Reply&& r) {
        using T = auto_type_t<V, std::string>;
        // T out;
        // if (!process_into(std::move(r), &out)) {
        //   return std::nullopt;
        // }
        // return out;
        if (!r) {
          return std::nullopt;
        }
        return std::move(r).get<T>();
      }

      template <class U>
      static bool process_into(Reply&& r, U* out) {
        if (!r) {
          return false;
        }
        *out = std::move(r).get<auto_type_t<V, U>>();
        return true;
      }
    };

    template <class T> struct remove_optional { using type = T; };
    template <class T> struct remove_optional<std::optional<T>> {
      using type = T;
    };

    template <class T> using remove_optional_t = typename remove_optional<T>::type;


    template <class Container>
    using is_reservable_t = std::void_t<decltype(std::declval<Container>().reserve(1)),
                                        decltype(std::declval<Container>().size())>;

    template <class Container, class Enable=void>
    struct reserve_impl {
      static void reserve(Container&, std::int64_t) {}
    };

    template <class Container>
    struct reserve_impl<Container, is_reservable_t<Container>> {
      static void reserve(Container& c, std::int64_t capacity) {
        c.reserve(c.size() + capacity);
      }
    };

    template <class Container>
    void reserve(Container& c, std::int64_t capacity) {
      reserve_impl<Container>::reserve(c, capacity);
    }

    template <class Iterator>
    struct Reserver : Iterator {
      Reserver(Iterator b, int n) : Iterator(b) {
        reserve(*this->container, n);
      }
    };

    template <class It>
    Reserver(It i, int n) -> Reserver<It>;


    template <class V, class Derived, class Default>
    struct BasicArrayCommand :
      Command<BasicArrayCommand<V, Derived, Default>>
    {
      using Command<BasicArrayCommand<V, Derived, Default>>::Command;
      using T = auto_type_t<V, Default>;

      static std::vector<T> process(Reply&& r) {
        std::vector<T> out;
        process_into(std::move(r), std::back_inserter(out));
        return out;
      }

      static std::int64_t process_into(Reply&& r, std::vector<T>* out) {
        out->clear();
        return process_into(std::move(r), std::back_inserter(*out));
      }

      template <class OutputIt>
      static std::int64_t process_into(Reply&& r, OutputIt out) {
        using U = auto_type_t<V, typename std::iterator_traits<OutputIt>::value_type>;
        return Derived::template process_into_impl<U>(std::move(r), out);
      }

      template <class Container>
      static std::int64_t process_into(Reply&& r, std::back_insert_iterator<Container> out) {
        Reserver(out, r.array_size());
        using U = auto_type_t<V, typename Container::value_type>;
        return Derived::template process_into_impl<U>(std::move(r), out);
      }

      template <class Container>
      static std::int64_t process_into(Reply&& r, std::front_insert_iterator<Container> out) {
        Reserver(out, r.array_size());
        using U = auto_type_t<V, typename Container::value_type>;
        return Derived::template process_into_impl<U>(std::move(r), out);
      }

      template <class Container>
      static std::int64_t process_into(Reply&& r, std::insert_iterator<Container> out) {
        using U = auto_type_t<V, typename Container::value_type>;
        return Derived::template process_into_impl<U>(std::move(r), out);
      }
    };

    template <class T>
    struct ArrayCommand :
      BasicArrayCommand<T, ArrayCommand<T>, std::string>
    {
      using Base = BasicArrayCommand<T, ArrayCommand<T>, std::string>;
      using Base::Base;

      template <class U, class OutputIt>
      static std::int64_t process_into_impl(Reply&& r, OutputIt out) {
        auto elements = std::move(r).array();
        for (auto& rr : elements) {
          *out++ = std::move(rr).get<U>();
        }
        return elements.size();
      }
    };

    template <class T=std::string>
    struct ArrayOptCommand :
      BasicArrayCommand<std::optional<T>, ArrayOptCommand<T>, std::optional<std::string>>
    {
      using Base = BasicArrayCommand<std::optional<T>, ArrayOptCommand<T>, std::optional<std::string>>;
      using Base::Base;

      template <class O, class OutputIt>
      static std::int64_t process_into_impl(Reply&& r, OutputIt out) {
        using U = remove_optional_t<O>;
        auto elements = std::move(r).array();
        for (auto& rr : elements) {
          if (rr) {
            *out++ = std::move(rr).get<U>();
          }
          else {
            *out++ = std::nullopt;
          }
        }
        return elements.size();
      }
    };


    template <class... Ts>
    struct _TupleOptCommand : Command<_TupleOptCommand<Ts...>> {
      using Command<_TupleOptCommand<Ts...>>::Command;
      using result_type = std::tuple<std::optional<Ts>...>;

      static result_type process(Reply&& r) {
        result_type out;
        process_into(std::move(r), &out);
        return out;
      }

      template <class V=void>
      static std::int64_t process_into(Reply&& r, std::tuple<std::optional<Ts>...>* out) {
        auto elements = std::move(r).array();
        if (elements.size() != sizeof...(Ts)) {
          throw Error("unexpected array reply size");
        }
        *out = pack(elements, std::make_index_sequence<sizeof...(Ts)>());
        return sizeof...(Ts);
      }
    private:
      template <std::size_t... I>
      static result_type pack(std::vector<Reply>& elements, std::index_sequence<I...>) {
        return {get_opt<nth_element<I, Ts...>>(elements[I])...};
      }

      template <class T>
      static std::optional<T> get_opt(Reply& r) {
        if (!r) {
          return std::nullopt;
        }
        return std::move(r).get<T>();
      }
    };


    template <class... Ts>
    struct group {};

    template <class Out, class Ts, class Args>
    struct add_default_types;

    template <class... Out, class T, class... Ts, class A, class... Args>
    struct add_default_types<group<Out...>, group<T, Ts...>, group<A, Args...>>
      : add_default_types<group<Out..., T>, group<Ts...>, group<Args...>> {};

    template <class... Out,  class A, class... Args>
    struct add_default_types<group<Out...>, group<>, group<A, Args...>>
      : add_default_types<group<Out..., std::string>, group<>, group<Args...>> {};

    template <class... Out, class... Ts>
    struct add_default_types<group<Out...>, group<Ts...>, group<>> {
      using type = _TupleOptCommand<Out...>;
    };

    // template<class... Ts, class... Args>
    // TupleOptCommand(std::string_view cmd, Args const&... args) -> typename add_default_types<group<>, group<Ts...>, group<Args...>>::type;

    template<class... Ts, class... Args>
    decltype(auto) TupleOptCommand(std::string_view cmd, Args const&... args) {
      return typename add_default_types<group<>, group<Ts...>, group<Args...>>::type(cmd, args...);
    }

    template <class... Ts>
    struct OptTupleCommand : Command<OptTupleCommand<Ts...>> {
      using Command<OptTupleCommand<Ts...>>::Command;
      using result_type = std::optional<std::tuple<Ts...>>;

      static result_type process(Reply&& r) {
        std::tuple<Ts...> out;
        if (process_into(std::move(r), &out)) {
          return out;
        }
        return std::nullopt;
      }

      static bool process_into(Reply&& r, std::tuple<Ts...>* out) {
        if (!r) {
          return false;
        }
        auto elements = std::move(r).array();
        //auto const n = elements.size();
        if (elements.size() != sizeof...(Ts)) {
          throw Error("unexpected array reply size");
        }
        *out = pack(elements, std::make_index_sequence<sizeof...(Ts)>());
        return true;
      }
    private:
      template <std::size_t... I>
      static std::tuple<Ts...> pack(std::vector<Reply>& elements, std::index_sequence<I...>) {
        //auto elements = std::move(r).array();
        return std::make_tuple(std::move(elements[I]).get<nth_element<I, Ts...>>()...);
      }
    };

  }
}

#endif
