// -*- C++ -*-
#ifndef RED1Z_COMMAND_STRING_H
#define RED1Z_COMMAND_STRING_H

namespace red1z {
  namespace impl {
    template <class Executor> class StringCommands {
      template <class Cmd> decltype(auto) run(Cmd &&cmd) {
        return static_cast<Executor *>(this)->_run(std::move(cmd));
      }

    public:
      template <class K, class V>
      decltype(auto) append(K const &key, V const &value) {
        return run(IntegerCommand("APPEND", key, value));
      }

      template <class K>
      decltype(auto) bitcount(K const &key, std::int64_t start,
                              std::int64_t end) {
        return run(IntegerCommand("BITCOUNT", key, std::to_string(start),
                                  std::to_string(end)));
      }

      template <class K> decltype(auto) bitcount(K const &key) {
        return run(IntegerCommand("BITCOUNT", key));
      }

      template <class K, class... Flags>
      decltype(auto) bitfield(K const &key, Flags &&... flags) {
        // CHECK_FLAGS("invalid flags for bitfield()", Flags...,
        //             impl::many<flags::_getbf>, impl::many<flags::_setbf>,
        //             impl::many<flags::_incrby>,
        //             impl::many<flags::_overflow>);
        using s =
            sig::signature<sig::arg<>, sig::flag<flags::_getbf>,
                           sig::flag<flags::_setbf>, sig::flag<flags::_incrby>,
                           sig::flag<flags::_overflow>>;
        return run(ArrayCommand<std::int64_t>("BITFIELD", s(), key,
                                              std::forward<Flags>(flags)...));
      }

      template <class Kdst, class K, class... Ks>
      decltype(auto) bitop_and(Kdst const &dst, K const &key,
                               Ks const &... keys) {
        return run(IntegerCommand("BITOP", "AND", dst, key, keys...));
      }

      template <class Kdst, class K, class... Ks>
      decltype(auto) bitop_or(Kdst const &dst, K const &key,
                              Ks const &... keys) {
        return run(IntegerCommand("BITOP", "OR", dst, key, keys...));
      }

      template <class Kdst, class K, class... Ks>
      decltype(auto) bitop_xor(Kdst const &dst, K const &key,
                               Ks const &... keys) {
        return run(IntegerCommand("BITOP", "XOR", dst, key, keys...));
      }

      template <class Kdst, class K>
      decltype(auto) bitop_not(Kdst const &dst, K const &key) {
        return run(IntegerCommand("BITOP", "NOT", dst, key));
      }

      template <class K>
      decltype(auto) bitpos(K const &key, bool bit, std::int64_t start,
                            std::int64_t end) {
        return run(IntegerCommand("BITPOS", key, bit ? "1" : "0",
                                  std::to_string(start), std::to_string(end)));
      }

      template <class K> decltype(auto) bitpos(K const &key, bool bit) {
        return run(IntegerCommand("BITPOS", key, bit ? "1" : "0"));
      }

      template <class K> decltype(auto) decr(K const &key) {
        return run(IntegerCommand("DECR", key));
      }

      template <class K> decltype(auto) decrby(K const &key, std::int64_t i) {
        return run(IntegerCommand("DECRBY", key, std::to_string(i)));
      }

      template <class T = auto_t, class K> decltype(auto) get(K const &key) {
        return run(BulkStringCommand<T>("GET", key));
      }

      template <class K>
      decltype(auto) getbit(K const &key, std::int64_t offset) {
        return run(IntegerCommand("GETBIT", key, std::to_string(offset)));
      }

      template <class K>
      decltype(auto) getrange(K const &key, std::int64_t start,
                              std::int64_t end) {
        return run(BulkStringCommand("GETRANGE", key, std::to_string(start),
                                     std::to_string(end)));
      }

      template <class K, class V>
      decltype(auto) getset(K const &key, V const &value) {
        return run(BulkStringCommand("GETSET", key, value));
      }

      template <class K> decltype(auto) incr(K const &key) {
        return run(IntegerCommand("INCR", key));
      }

      template <class K> decltype(auto) incrby(K const &key, std::int64_t i) {
        return run(IntegerCommand("INCRBY", key, std::to_string(i)));
      }

      // template <class K>
      // decltype(auto) incrby(K const& key, double i) {
      //   return run(FloatCommand("INCRBYFLOAT", key, std::to_string(i)));
      // }

      template <class K> decltype(auto) incrbyfloat(K const &key, double i) {
        return run(FloatCommand("INCRBYFLOAT", key, std::to_string(i)));
      }

      template <class... Ts, class... Keys>
      decltype(auto) mget(Keys const &... keys) {
        if constexpr (has_pack_v<Keys...>) {
          static_assert(sizeof...(Ts) <= 1);
          return run(ArrayOptCommand<Ts...>("MGET", keys...));
        } else {
          return run(TupleOptCommand<Ts...>("MGET", keys...));
        }
      }

      template <class... KV> decltype(auto) mset(KV &&... key_value_pairs) {
        using s = sig::signature<sig::pair<sig::arg<>, sig::arg<>>,
                                 sig::many<sig::pair<sig::arg<>, sig::arg<>>>>;
        return run(
            StatusCommand("MSET", s(), std::forward<KV>(key_value_pairs)...));
      }

      template <class... KV> decltype(auto) msetnx(KV &&... key_value_pairs) {
        using s = sig::signature<sig::pair<sig::arg<>, sig::arg<>>,
                                 sig::many<sig::pair<sig::arg<>, sig::arg<>>>>;
        return run(IntegerCommand("MSETNX", s(),
                                  std::forward<KV>(key_value_pairs)...));
      }

      template <class K, class V>
      decltype(auto) psetex(K const &key, std::int64_t ms, V const &value) {
        return run(BulkStringCommand("PSETEX", key, std::to_string(ms), value));
      }

      template <class K, class V, class... Flags>
      decltype(auto) set(K const &key, V const &value, Flags const &... flags) {
        using s =
            sig::signature<sig::pair<sig::arg<>, sig::arg<>>,
                           sig::flag<flags::_ex_px>, sig::flag<flags::_nx_xx>>;
        return run(StatusCommand("SET", s(), key, value, flags...));
      }

      template <class K>
      decltype(auto) setbit(K const &key, std::int64_t ms, bool value) {
        return run(IntegerCommand("SETBIT", key, std::to_string(ms),
                                  value ? "1" : "0"));
      }

      template <class K, class V>
      decltype(auto) setex(K const &key, std::int64_t s, V const &value) {
        return run(BulkStringCommand("SETEX", key, std::to_string(s), value));
      }

      template <class K, class V>
      decltype(auto) setnx(K const &key, V const &value) {
        return run(BulkStringCommand("SETNX", key, value));
      }

      template <class K, class V>
      decltype(auto) setrange(K const &key, std::int64_t offset,
                              V const &value) {
        return run(
            IntegerCommand("SETRANGE", key, std::to_string(offset), value));
      }

      // STRALGO

      template <class K> decltype(auto) strlen(K const &key) {
        return run(IntegerCommand("STRLEN", key));
      }
    };
  } // namespace impl
} // namespace red1z

#endif
