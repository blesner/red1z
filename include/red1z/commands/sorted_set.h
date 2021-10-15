// -*- C++ -*-
#ifndef RED1Z_COMMAND_SORTED_SET_H
#define RED1Z_COMMAND_SORTED_SET_H

namespace red1z {
  namespace impl {

    template <class K_, class V_>
    struct BZPopCommand : Command<BZPopCommand<K_, V_>> {
      using Command<BZPopCommand<K_, V_>>::Command;
      using K = auto_type_t<K_, std::string>;
      using V = auto_type_t<V_, std::string>;

      using result_type = std::optional<std::tuple<K, V, double>>;

      static result_type process(Reply &&r) {
        std::tuple<K, V, double> out;
        if (!process_into(std::move(r), &out)) {
          return std::nullopt;
        }
        return out;
      }

      static bool process_into(Reply &&r, std::tuple<K, V, double> *out) {
        std::tuple<K, V, std::string> t;
        if (!OptTupleCommand<K, V, std::string>::process_into(std::move(r),
                                                              &t)) {
          return false;
        }
        *out = std::make_tuple(std::move(std::get<0>(t)),
                               std::move(std::get<1>(t)),
                               atof(std::get<2>(t).c_str()));
        return true;
      }
    };

    template <class V>
    struct ZRangeWithScoresCommand
        : BasicArrayCommand<std::tuple<auto_type_t<V, std::string>, double>,
                            ZRangeWithScoresCommand<V>,
                            std::tuple<std::string, double>> {
      using Base =
          BasicArrayCommand<std::tuple<auto_type_t<V, std::string>, double>,
                            ZRangeWithScoresCommand<V>,
                            std::tuple<std::string, double>>;

      template <class K>
      ZRangeWithScoresCommand(K const &key, std::int64_t start,
                              std::int64_t stop)
          : Base("ZRANGE", key, std::to_string(start), std::to_string(stop),
                 "WITHSCORES") {}

      template <class U, class OutputIt>
      static std::int64_t process_into_impl(Reply &&r, OutputIt out) {
        using W = std::tuple_element_t<0, U>;
        auto elements = std::move(r).array();
        for (auto i = elements.begin(), end = elements.end(); i != end;
             i += 2) {
          *out++ =
              std::make_pair(std::move(i[0]).get<W>(), i[1].floating_point());
        }
        return r.array_size();
      }
    };

    template <class Executor> class SortedSetCommands {
      template <class Cmd> decltype(auto) run(Cmd &&cmd) {
        return static_cast<Executor *>(this)->_run(std::move(cmd));
      }

      template <class K, class V, class Key, class... Args>
      decltype(auto) bzpop(char const *cmd, Key const &key,
                           Args const &... args) {
        // static constexpr int N = sizeof...(Args);
        // static_assert(N > 0, "bzpop*(), missing timeout argument");
        // static_assert(
        //     std::is_convertible_v<nth_element<N - 1, Args...>, std::int64_t>,
        //     "timeout must be convertible to integer");
        // return run(BZPopCommand<K, V>(converter<nth_arg<N>, int2str>(), cmd,
        //                               key, args...));

        using s = sig::signature<sig::arg<>, sig::many<sig::arg<>>,
                                 sig::arg<sig::_int>>;

        return run(BZPopCommand<K, V>(cmd, s(), key, args...));
      }

    public:
      template <class K = auto_t, class V = auto_t, class Key, class... Args>
      decltype(auto) bzpopmax(Key const &key, Args const &... args) {
        return bzpop<K, V>("BZPOPMAX", key, args...);
      }

      template <class K = auto_t, class V = auto_t, class Key, class... Args>
      decltype(auto) bzpopmin(Key const &key, Args const &... args) {
        return bzpop<K, V>("BZPOPMIN", key, args...);
      }

      // template <class K, class... Args>
      // decltype(auto) zadd(K const &key, Args const &... args) {
      //   using R = impl::flags_range<Args...>;
      //   static_assert(
      //       R::first == 0,
      //       "usage: zadd(key, [flags], score, member, [score, member ...]");
      //   CHECK_FLAGS("invalid flags for zadd()", Args..., flags::_nx_xx,
      //               flags::_ch, flags::_incr);
      //   static constexpr bool has_incr_flag =
      //       impl::has_flag_v<flags::_incr, Args...>;
      //   static auto const cvt =
      //       converter<every_arg<2>, double2str, R::last + 1>();
      //   if constexpr (has_incr_flag) {
      //     return run(FloatCommand(cvt, "ZADD", key, args...));
      //   } else {
      //     return run(IntegerCommand(cvt, "ZADD", key, args...));
      //   }
      // }

      template <class K, class... Args>
      decltype(auto) zadd(K const &key, Args const &... args) {
        using s = sig::signature<
            sig::arg<>, sig::flag<flags::_nx_xx>, sig::flag<flags::_ch>,
            sig::flag<flags::_incr>,
            sig::pair<sig::arg<sig::_float>, sig::arg<>>,
            sig::many<sig::pair<sig::arg<sig::_float>, sig::arg<>>>>;

        static constexpr bool has_incr_flag =
            impl::has_flag_v<flags::_incr, Args...>;

        if constexpr (has_incr_flag) {
          return run(FloatCommand("ZADD", s(), key, args...));
        } else {
          return run(IntegerCommand("ZADD", s(), key, args...));
        }
      }

      template <class K> decltype(auto) zcard(K const &key) {
        return run(IntegerCommand("ZCARD", key));
      }

      template <class K>
      decltype(auto) zcount(K const &key, double min, double max) {
        return run(IntegerCommand("ZCOUNT", key, std::to_string(min),
                                  std::to_string(max)));
      }

      template <class K, class M>
      decltype(auto) zincrby(K const &key, double increment, M const &member) {
        return run(
            FloatCommand("ZINCRBY", key, std::to_string(increment), member));
      }

      template <class Kdst, class K0, class... Args>
      decltype(auto) zinterstore(Kdst const &dst, std::int64_t numkeys,
                                 K0 const &key, Args &&... args) {
        // using R = impl::flags_range<Args...>;
        // static_assert(R::empty or R::last == sizeof...(Args),
        //               "flags must come last");
        // CHECK_FLAGS("invalid flags for zinterstore()", Args...,
        // flags::_weights,
        //             flags::_aggregate);

        using s =
            sig::signature<sig::arg<>, sig::arg<sig::_int>, sig::arg<>,
                           sig::many<sig::arg<>>, sig::flag<flags::_weights>,
                           sig::flag<flags::_aggregate>>;
        return run(IntegerCommand("ZINTERSTORE", s(), dst, numkeys, key,
                                  std::forward<Args>(args)...));
      }

      template <class K>
      decltype(auto) zlexcount(K const &key, double min, double max) {
        return run(IntegerCommand("ZLEXCOUNT", key, std::to_string(min),
                                  std::to_string(max)));
      }

      template <class V = auto_t, class K>
      decltype(auto) zpopmax(K const &key, std::int64_t count = 1) {
        return run(ArrayCommand<V>("ZPOPMAX", key, std::to_string(count)));
      }

      template <class V = auto_t, class K>
      decltype(auto) zpopmin(K const &key, std::int64_t count = 1) {
        return run(ArrayCommand<V>("ZPOPMIN", key, std::to_string(count)));
      }

      template <class V = auto_t, class K>
      decltype(auto) zrange(K const &key, std::int64_t start,
                            std::int64_t stop) {
        return run(ArrayCommand<V>("ZRANGE", key, std::to_string(start),
                                   std::to_string(stop)));
      }

      template <class V = auto_t, class K>
      decltype(auto) zrange(K const &key, std::int64_t start, std::int64_t stop,
                            const impl::flag<flags::_withscores> &) {
        return run(ZRangeWithScoresCommand<V>(key, start, stop));
      }
    };
  } // namespace impl
} // namespace red1z

#endif
