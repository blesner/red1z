// -*- C++ -*-

namespace red1z {
  namespace impl {
    template <class Executor> class HashCommands {
      template <class Cmd> decltype(auto) run(Cmd &&cmd) {
        return static_cast<Executor *>(this)->_run(std::move(cmd));
      }

    public:
      template <class K, class... Fs>
      decltype(auto) hdel(K const &key, Fs const &... fields) {
        static_assert(sizeof...(Fs) > 0, "hdel() no field specified");
        return run(IntegerCommand("HDEL", key, fields...));
      }

      template <class T, class K, class F>
      decltype(auto) hexists(K const &key, F const &field) {
        return run(IntegerCommand("HEXISTS", key, field));
      }

      template <class T = auto_t, class K, class F>
      decltype(auto) hget(K const &key, F const &field) {
        return run(BulkStringCommand<T>("HGET", key, field));
      }

      // TODO
      // template <class K=std::string, class V=std::string, class Key>
      // std::map<K, V> hgetall(Key const& key) {
      //   auto r = cmd("HGETALL", key);
      //   auto items = r.array();
      //   int const n = items.size() / 2;
      //   std::map<K, V> out;

      //   for (int i = 0; i < n; i += n) {
      //     out.emplace(items[2*i].template as<K>().value(),
      //     items[2*i+1].template as<V>().value());
      //   }
      //   return out;
      // }

      template <class K, class F>
      decltype(auto) hincrby(K const &key, F const &field,
                             std::int64_t increment) {
        return run(
            IntegerCommand("HINCRBY", key, field, std::to_string(increment)));
      }

      template <class K, class F>
      decltype(auto) hincrbyfloat(K const &key, F const &field,
                                  double increment) {
        return run(FloatCommand("HINCRBYFLOAT", key, field,
                                std::to_string(increment)));
      }

      template <class Kout = auto_t, class K>
      decltype(auto) hkeys(K const &key) {
        return run(ArrayCommand<Kout>("HKEYS", key));
      }

      template <class K> decltype(auto) hlen(K const &key) {
        return run(IntegerCommand("HLEN", key));
      }

      template <class... Ts, class K, class... Fs>
      decltype(auto) hmget(K const &key, Fs const &... fields) {
        if constexpr (has_pack_v<Fs...>) {
          static_assert(sizeof...(Ts) <= 1);
          return run(ArrayOptCommand<Ts...>("HMGET", key, fields...));
        } else {
          return run(TupleOptCommand<Ts...>("HMGET", key, fields...));
        }
      }

      template <class K, class... FVs>
      decltype(auto) hmset(K const &key, FVs const &... field_value_pairs) {
        using s = sig::signature<sig::arg<>, sig::pair<sig::arg<>, sig::arg<>>,
                                 sig::many<sig::pair<sig::arg<>, sig::arg<>>>>;
        // static_assert(pair_checker<FVs...>::value,
        //               "hmset(): invalid number of arguments");
        return run(StatusCommand("HMSET", s(), key, field_value_pairs...));
      }

      template <class K, class... FVs>
      decltype(auto) hset(K const &key, FVs const &... field_value_pairs) {
        using s = sig::signature<sig::arg<>, sig::pair<sig::arg<>, sig::arg<>>,
                                 sig::many<sig::pair<sig::arg<>, sig::arg<>>>>;
        // static_assert(pair_checker<FVs...>::value,
        //               "hset(): invalid number of arguments");
        return run(IntegerCommand("HSET", s(), key, field_value_pairs...));
      }

      template <class K, class F, class V>
      decltype(auto) hsetnx(K const &key, F const &field, V const &value) {
        return run(IntegerCommand("HSETNX", key, field, value));
      }

      template <class K, class F>
      decltype(auto) hstrlen(K const &key, F const &field) {
        return run(IntegerCommand("HSTRLEN", key, field));
      }

      template <class T = auto_t, class K> decltype(auto) hvals(K const &key) {
        return run(ArrayCommand<T>("HVALS", key));
      }

      // HSCAN
    };
  } // namespace impl
} // namespace red1z
