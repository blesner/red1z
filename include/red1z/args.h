// -*- C++ -*-
#ifndef RED1Z_ARGS_H
#define RED1Z_ARGS_H

#include "io.h"

#include <array>
#include <string>

namespace red1z {
  namespace impl {
    template <class T> struct has_tuple_protocol : std::false_type {};

    template <class... Ts>
    struct has_tuple_protocol<std::tuple<Ts...>> : std::true_type {};

    template <class A, class B>
    struct has_tuple_protocol<std::pair<A, B>> : std::true_type {};

    template <class T, std::size_t N>
    struct has_tuple_protocol<std::array<T, N>> : std::true_type {};

    template <class T>
    static constexpr int has_tuple_protocol_v = has_tuple_protocol<T>::value;

    template <class T, bool IsTuple = has_tuple_protocol_v<T>>
    struct value_size;

    template <class T> struct value_size<T, true> : std::tuple_size<T> {};

    template <class T>
    struct value_size<T, false> : std::integral_constant<std::size_t, 1> {};

    template <class T> static constexpr int value_size_v = value_size<T>::value;

    template <class ItBeg, class ItEnd> class ArgPack {
      ItBeg m_begin;
      ItEnd m_end;

    public:
      using value_type = typename std::iterator_traits<ItBeg>::value_type;
      static constexpr int value_size = value_size_v<value_type>;

      ArgPack(ItBeg b, ItEnd e) : m_begin(b), m_end(e) {}

      std::size_t size() const {
        return std::distance(m_begin, m_end);
      }

      bool empty() const {
        return m_begin == m_end;
      }

      ItBeg begin() const {
        return m_begin;
      }
      ItEnd end() const {
        return m_end;
      }

      void encode(Encoder &e) const {
        for (auto const &v : *this) {
          encode(e, v, std::make_index_sequence<value_size>());
        }
      }

      template <int I>
      using nth_type =
          std::conditional_t<has_tuple_protocol_v<value_type>,
                             std::tuple_element_t<I, value_type>, value_type>;

    private:
      template <std::size_t... I>
      static void encode(Encoder &e, value_type const &v,
                         std::index_sequence<I...>) {
        int _[] = {(e.encode(get<I>(v)), 0)...};
        (void)_;
      }

      template <std::size_t I> static decltype(auto) get(value_type const &v) {
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
    struct pack_count<I, ArgPack<I1, I2>, Args...>
        : pack_count<I + 1, Args...> {};

    template <int I, class Arg, class... Args>
    struct pack_count<I, Arg, Args...> : pack_count<I, Args...> {};

    template <class... Args>
    static constexpr int pack_count_v = pack_count<0, Args...>::value;

    template <class... Args>
    static constexpr bool has_pack_v = pack_count_v<Args...> > 0;
  } // namespace impl
} // namespace red1z

#endif
