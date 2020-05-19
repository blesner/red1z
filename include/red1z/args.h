// -*- C++ -*-
#ifndef RED1Z_ARGS_H
#define RED1Z_ARGS_H

namespace red1z {
  namespace impl {
    /// Argument conversion
    struct int2str {
      static std::string cvt(std::int64_t i) { return std::to_string(i); }
    };

    struct double2str {
      static std::string cvt(double d) { return std::to_string(d); }
    };

    struct identity {
      template <class T>
      static T const& cvt(T const& x) { return x; }
    };


    template <int N>
    struct nth_arg {
      static constexpr bool match(int i) {
        return i == N;
      }
    };

    template <int Step=1>
    struct every_arg {
      static constexpr bool match(int i) {
        return i % Step == 0;
      }
    };

    template <class Pattern=every_arg<>, class Cvt=identity, int Skip=0>
    struct converter {
      template <int I, class T>
      static decltype(auto) cvt(T const& x) {
        if constexpr(I >= Skip and Pattern::match(I-Skip)) {
          return Cvt::cvt(x);
        } else {
          return x;
        }
      }
    };

    template <class T>
    struct has_tuple_protocol : std::false_type {};

    template <class... Ts>
    struct has_tuple_protocol<std::tuple<Ts...>> : std::true_type {};

    template <class A, class B>
    struct has_tuple_protocol<std::pair<A,B>> : std::true_type {};

    template <class T, std::size_t N>
    struct has_tuple_protocol<std::array<T, N>> : std::true_type {};

    template <class T>
    static constexpr int has_tuple_protocol_v = has_tuple_protocol<T>::value;


    template <class T, bool IsTuple=has_tuple_protocol_v<T>>
    struct value_size;

    template <class T>
    struct value_size<T, true> : std::tuple_size<T> {};

    template <class T>
    struct value_size<T, false> : std::integral_constant<std::size_t, 1> {};

    template <class T>
    static constexpr int value_size_v = value_size<T>::value;


    template <class ItBeg, class ItEnd>
    class ArgPack {
      ItBeg m_begin;
      ItEnd m_end;
    public:
      using value_type = typename std::iterator_traits<ItBeg>::value_type;
      static constexpr int value_size = value_size_v<value_type>;

      ArgPack(ItBeg b, ItEnd e) : m_begin(b), m_end(e) {}

      std::size_t size() const {
        return std::distance(m_begin, m_end);
      }

      ItBeg begin() const { return m_begin; }
      ItEnd end() const { return m_end; }

      template <class Cvt>
      void encode(Cvt const& cvt, Encoder& e) const {
        for (auto const& v : *this) {
          encode(cvt, e, v, std::make_index_sequence<value_size>());
        }
      }

      template <int I>
      using nth_type = std::conditional_t<has_tuple_protocol_v<value_type>,
                                          std::tuple_element_t<I, value_type>,
                                          value_type>;
    private:
      template <class Pattern, class Cvt, int Skip, std::size_t... I>
      static void encode(converter<Pattern, Cvt, Skip> const&,
                         Encoder& e, value_type const& v, std::index_sequence<I...>) {
        using Conv = converter<Pattern, Cvt, 0>; ///ignore skip
        int _[] = {e.encode(Conv::template cvt<I>(get<I>(v)))...};
        (void)_;
      }

      template <std::size_t I>
      static decltype(auto) get(value_type const& v) {
        if constexpr (has_tuple_protocol_v<value_type>) {
          return std::get<I>(v);
        } else {
          return v;
        }
      }
    };



    template <int I, class... Args>
    struct pack_count : std::integral_constant<int, I> {};

    template <int I, class I1, class I2, class... Args>
    struct pack_count<I, ArgPack<I1, I2>, Args...> : pack_count<I+1, Args...> {};

    template <int I, class Arg, class... Args>
    struct pack_count<I, Arg, Args...> : pack_count<I, Args...> {};

    template <class... Args>
    static constexpr int pack_count_v = pack_count<0, Args...>::value;

    template <class... Args>
    static constexpr bool has_pack_v = pack_count_v<Args...> > 0;


    template <class... Args>
    struct single_checker : std::true_type {};

    template <class Arg, class... Args>
    struct single_checker<Arg, Args...> : single_checker<Args...> {};

    template <class I1, class I2, class... Args>
    struct single_checker<ArgPack<I1, I2>, Args...> :
      std::conditional_t<ArgPack<I1, I2>::value_size == 1,
                         single_checker<Args...>,
                         std::false_type>
    {};


    template <class... Args>
    struct pair_checker;

    template <class Arg>
    struct pair_checker<Arg>: std::false_type {};

    template <class I1, class I2>
    struct pair_checker<ArgPack<I1, I2>> :
      std::integral_constant<bool, ArgPack<I1, I2>::value_size == 2>
    {};


    template <>
    struct pair_checker<>: std::true_type {};

    template <class Arg0, class Arg1, class... Args>
    struct pair_checker<Arg0, Arg1, Args...> : pair_checker<Args...> {};

    template <class I1, class I2, class A, class... Args>
    struct pair_checker<A, ArgPack<I1, I2>,  Args...> : std::false_type {};

    template <class I1, class I2, class... Args>
    struct pair_checker<ArgPack<I1, I2>, Args...> :
      std::conditional_t<ArgPack<I1, I2>::value_size == 2,
                         pair_checker<Args...>,
                         std::false_type
                         >
    {};

    template <class I1, class I2, class A, class... Args>
    struct pair_checker<ArgPack<I1, I2>, A, Args...> :
      std::conditional_t<ArgPack<I1, I2>::value_size == 2,
                         pair_checker<A, Args...>,
                         std::false_type
                         >
    {};
  }
}

#endif
