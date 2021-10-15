// -*- C++ -*-
#ifndef RED1Z_SIGNATURE_H
#define RED1Z_SIGNATURE_H

#include <iostream>

namespace red1z::impl::sig {

  struct _any {
    template <class T> static decltype(auto) cvt(T &&x) {
      return std::forward<T>(x);
    }
  };

  struct _int {
    static std::string cvt(std::int64_t i) {
      return std::to_string(i);
    }
  };
  struct _float {
    static std::string cvt(double x) {
      return std::to_string(x);
    }
  };

  struct generic {};

  template <class Param> struct opt {};
  template <class Type = _any> struct arg {};
  template <class A, class B> struct pair {};
  template <int Idx> struct kw {};
  template <class FTag, bool Opt = true> struct flag {};
  template <class Param> struct many {};

  template <class... Params> struct kw_count;

  template <int I, class... Params>
  struct kw_count<kw<I>, Params...>
      : std::integral_constant<int, 1 + kw_count<Params...>::value> {};

  template <class P, class... Params>
  struct kw_count<P, Params...> : kw_count<Params...> {};

  template <> struct kw_count<> : std::integral_constant<int, 0> {};

  template <class... Params> struct signature {};

  template <class Sig, class...>
  struct match : std::integral_constant<bool, false> {};

  template <std::size_t N> using KW = std::array<char const *, N>;

  template <std::size_t N, class Arg, class... Args>
  void encode(Encoder &e, KW<N> const &kw_, signature<generic> s, Arg &&arg,
              Args &&... args) {
    e.encode(std::forward<Arg>(arg));
    encode(e, kw_, s, std::forward<Args>(args)...);
  }

  template <std::size_t N, class... Flag, class... Args>
  void encode(Encoder &e, KW<N> const &kw_, signature<generic> s,
              impl::flag<Flag...> f, Args &&... args) {
    f.encode(e);
    encode(e, kw_, s, std::forward<Args>(args)...);
  }

  template <std::size_t N, class ItBeg, class ItEnd, class... Args>
  void encode(Encoder &e, KW<N> const &kw_, signature<generic> s,
              ArgPack<ItBeg, ItEnd> pack, Args &&... args) {
    pack.encode(e);
    encode(e, kw_, s, std::forward<Args>(args)...);
  }

  // terminate generic signature
  template <std::size_t N>
  void encode(Encoder &, KW<N> const &, signature<generic>) {}

  ////
  template <int I, class... Params, class... Args>
  struct match<signature<kw<I>, Params...>, Args...>
      : match<signature<Params...>, Args...> {}; // OK skip keyword

  // encode static keyword
  template <std::size_t N, int I, class... Params, class... Args>
  void encode(Encoder &e, KW<N> const &kw_, signature<kw<I>, Params...>,
              Args &&... args) {
    static_assert(I < N, "missing command keyword");
    e.encode(kw_[I]);
    encode(e, kw_, signature<Params...>(), std::forward<Args>(args)...);
  }

  //////

  template <class FTag, class... Params, class... Flag, class... Args>
  struct match<signature<flag<FTag, true>, Params...>,
               impl::flag<FTag, Flag...>, Args...>
      : match<signature<Params...>, Args...> {}; // OK flag is optional and set

  // encode set optional flag
  template <std::size_t N, class FTag, class... Params, class... Flag,
            class... Args>
  void encode(Encoder &e, KW<N> const &kw,
              signature<flag<FTag, true>, Params...>,
              impl::flag<FTag, Flag...> flag, Args &&... args) {
    flag.encode(e);
    encode(e, kw, signature<Params...>(), std::forward<Args>(args)...);
  }

  /////////////////////

  template <class FTag, class... Params, class... Flag, class... Args>
  struct match<signature<flag<FTag, false>, Params...>,
               impl::flag<FTag, Flag...>, Args...>
      : match<signature<Params...>, Args...> {}; // OK flag is mandatory and set

  // encode set mandatory flag
  template <std::size_t N, class FTag, class... Params, class... Flag,
            class... Args>
  void encode(Encoder &e, KW<N> const &kw,
              signature<flag<FTag, false>, Params...>,
              impl::flag<FTag, Flag...> flag, Args &&... args) {
    flag.encode(e);
    encode(e, kw, signature<Params...>(), std::forward<Args>(args)...);
  }

  /////////////////////

  template <class FTag, class... Params, class Arg, class... Args>
  struct match<signature<flag<FTag, true>, Params...>, Arg, Args...>
      : match<signature<Params...>, Arg, Args...> {
  }; // OK flag is optional and not set

  // skip unset optional flag
  template <std::size_t N, class FTag, class... Params, class Arg,
            class... Args>
  void encode(Encoder &e, KW<N> const &kw,
              signature<flag<FTag, true>, Params...>, Arg &&arg,
              Args &&... args) {
    encode(e, kw, signature<Params...>(), std::forward<Arg>(arg),
           std::forward<Args>(args)...);
  }

  // terminate many<flag>
  template <std::size_t N, class FTag, class... Params, class Arg,
            class... Args>
  void encode(Encoder &e, KW<N> const &kw,
              signature<flag<FTag, true>, many<flag<FTag, true>>, Params...>,
              Arg &&arg, Args &&... args) {
    encode(e, kw, signature<Params...>(), std::forward<Arg>(arg),
           std::forward<Args>(args)...);
  }

  //////////////

  template <class T, class... Params, class Arg, class... Args>
  struct match<signature<arg<T>, Params...>, Arg, Args...>
      : match<signature<Params...>, Args...> {}; // OK: arg as expected

  template <class T> struct is_pack : std::false_type {};
  template <class B, class E> struct is_pack<ArgPack<B, E>> : std::true_type {};

  // encode single arg<>
  template <std::size_t N, class Type, class... Params, class Arg,
            class... Args>
  std::enable_if_t<!is_pack<Arg>::value> encode(Encoder &e, KW<N> const &kw,
                                                signature<arg<Type>, Params...>,
                                                Arg &&arg, Args &&... args) {
    e.encode(Type::cvt(std::forward<Arg>(arg)));
    encode(e, kw, signature<Params...>(), std::forward<Args>(args)...);
  }

  // encode pair<arg, arg>
  template <std::size_t N, class Type1, class Type2, class... Params,
            class Arg1, class Arg2, class... Args>
  std::enable_if_t<!is_pack<Arg1>::value && !is_pack<Arg2>::value>
  encode(Encoder &e, KW<N> const &kw,
         signature<pair<arg<Type1>, arg<Type2>>, Params...>, Arg1 &&arg1,
         Arg2 &&arg2, Args &&... args) {
    e.encode(Type1::cvt(std::forward<Arg1>(arg1)));
    e.encode(Type2::cvt(std::forward<Arg2>(arg2)));
    encode(e, kw, signature<Params...>(), std::forward<Args>(args)...);
  }

  template <class T, class... Params, class Arg, class... Args>
  struct match<signature<many<T>, Params...>, Arg, Args...>
      : match<signature<many<T>, Params...>, Args...> {}; // OK: arg as expected

  /////////////////////

  template <class T>
  struct match<signature<many<T>>> : std::integral_constant<bool, true> {
  }; // OK: done: no more args to consume for many<>

  // terminate many
  template <std::size_t N, class T>
  void encode(Encoder &, KW<N> const &, signature<many<T>>) {}

  // expand many<T> to T, many<T> when there is at least one remaining argument
  template <std::size_t N, class T, class... Params, class Arg, class... Args>
  void encode(Encoder &e, KW<N> const &kw, signature<many<T>, Params...>,
              Arg &&a, Args &&... args) {
    encode(e, kw, signature<T, many<T>, Params...>(), std::forward<Arg>(a),
           std::forward<Args>(args)...);
  }

  // handle trailing arg<> after many<>
  template <std::size_t N, class T, class Type, class Arg>
  void encode(Encoder &e, KW<N> const &kw, signature<many<T>, arg<Type>>,
              Arg &&x) {
    encode(e, kw, signature<arg<Type>>(), std::forward<Arg>(x));
  }

  // handle trailing pair<arg,arg> after many<>
  template <std::size_t N, class T, class Type1, class Arg1, class Type2,
            class Arg2>
  void encode(Encoder &e, KW<N> const &kw,
              signature<many<T>, pair<arg<Type1>, arg<Type2>>>, Arg1 &&a,
              Arg2 &&b) {
    encode(e, kw, signature<pair<arg<Type1>, arg<Type2>>>(),
           std::forward<Arg1>(a), std::forward<Arg2>(b));
  }

  // handle multiple trailing flags<> after many<>
  template <std::size_t N, class T, class Type, class... Params, class... Args>
  std::enable_if_t<all_flags_v<Args...>> encode(Encoder &e, KW<N> const &kw,
                                                signature<many<T>, Params...>,
                                                Args &&... args) {
    encode(e, kw, signature<Params...>(), std::forward<Args>(args)...);
  }

  // handle many<arg> from pack
  template <std::size_t N, class Type, class... Params, class ItBegin,
            class ItEnd, class... Args>
  void encode(Encoder &e, KW<N> const &kw,
              signature<many<arg<Type>>, Params...> s,
              ArgPack<ItBegin, ItEnd> pack, Args &&... args) {
    for (auto &&v : pack) {
      e.encode(Type::cvt(v));
    }
    encode(e, kw, s, std::forward<Args>(args)...);
  }

  // handle many<pair<arg, arg>> from pack
  template <std::size_t N, class Type1, class Type2, class... Params,
            class ItBegin, class ItEnd, class... Args>
  void encode(Encoder &e, KW<N> const &kw,
              signature<many<pair<arg<Type1>, arg<Type2>>>, Params...> s,
              ArgPack<ItBegin, ItEnd> pack, Args &&... args) {
    static_assert(pack.value_size == 2);
    for (auto &&v : pack) {
      e.encode(Type1::cvt(std::get<0>(v)));
      e.encode(Type2::cvt(std::get<1>(v)));
    }
    encode(e, kw, s, std::forward<Args>(args)...);
  }

  template <class Type, class T>
  void encode_pack_element(Encoder &e, arg<Type>, T &&elem) {
    e.encode(Type::cvt(std::forward<T>(elem)));
  }

  template <class Type1, class Type2, class T>
  void encode_pack_element(Encoder &e, pair<arg<Type1>, arg<Type2>>, T &&elem) {
    e.encode(Type1::cvt(std::get<0>(elem)));
    e.encode(Type2::cvt(std::get<1>(elem)));
  }

  template <std::size_t N, class P, class... Params, class ItBegin, class ItEnd,
            class... Args>
  void encode_from_pack(Encoder &e, KW<N> const &kw, signature<P, Params...> s,
                        ArgPack<ItBegin, ItEnd> const &pack, Args &&... args) {

    if (pack.empty()) {
      encode(e, kw, s, std::forward<Args>(args)...);
    } else {
      auto it = pack.begin();
      encode_pack_element(e, P(), *it);
      if (++it != pack.end()) {
        encode(e, kw, signature<Params...>(),
               ArgPack<ItBegin, ItEnd>(it, pack.end()),
               std::forward<Args>(args)...);
      } else {
        encode(e, kw, signature<Params...>(), std::forward<Args>(args)...);
      }
    }
  }

  // handle arg<> from pack
  template <std::size_t N, class Type, class... Params, class ItBegin,
            class ItEnd, class... Args>
  void encode(Encoder &e, KW<N> const &kw, signature<arg<Type>, Params...> s,
              ArgPack<ItBegin, ItEnd> pack, Args &&... args) {
    static_assert(pack.value_size == 1);
    encode_from_pack(e, kw, s, pack, std::forward<Args>(args)...);
  }

  // handle pair<arg, arg> from pack
  template <std::size_t N, class Type1, class Type2, class... Params,
            class ItBegin, class ItEnd, class... Args>
  void encode(Encoder &e, KW<N> const &kw,
              signature<pair<arg<Type1>, arg<Type2>>, Params...> s,
              ArgPack<ItBegin, ItEnd> pack, Args &&... args) {
    static_assert(pack.value_size == 2);
    encode_from_pack(e, kw, s, pack, std::forward<Args>(args)...);
  }

  // should not be called but needs to exists to handle empty packs
  template <std::size_t N, class P, class... Params>
  void encode(Encoder &, KW<N> const &, signature<P, Params...>) {
    throw std::runtime_error("missing arguments");
  }

  // handle optional flag when arguments are exhausted
  template <std::size_t N, class Flag, class... Params>
  void encode(Encoder &e, KW<N> const &kw,
              signature<flag<Flag, true>, Params...>) {
    encode(e, kw, signature<Params...>());
  }

  ///////

  template <>
  struct match<signature<>> : std::integral_constant<bool, true> {}; // OK: done

  template <std::size_t N> void encode(Encoder &, KW<N> const &, signature<>) {}

  inline constexpr int arg_size(...) {
    return 1;
  }

  template <class Tag, class... Flag>
  int arg_size(impl::flag<Tag, Flag...> const &f) {
    return f.size();
  }

  template <class It1, class It2> int arg_size(ArgPack<It1, It2> const &pack) {
    return pack.size() * ArgPack<It1, It2>::value_size;
  }

  template <std::size_t N, class... Params, class... Args>
  std::string encode(std::string_view c, KW<N> kw, signature<Params...> s,
                     Args &&... args) {
    Encoder e((sig::arg_size(args) + ... + 1) + kw_count<Params...>::value);
    e.encode(c);
    encode(e, kw, s, std::forward<Args>(args)...);
    return e.value();
  }

} // namespace red1z::impl::sig

#endif // RED1Z_SIGNATURE_H
