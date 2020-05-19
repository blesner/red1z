// -*- C++ -*-
#ifndef RED1Z_COMMAND_SET_H
#define RED1Z_COMMAND_SET_H

namespace red1z {
  namespace impl {
    template <class Executor>
    class SetCommands {
      template <class Cmd>
      decltype(auto) run(Cmd&& cmd) {
        return static_cast<Executor*>(this)->_run(std::move(cmd));
      }
    public:
      template <class K, class M, class... Ms>
      decltype(auto) sadd(K const& key, M const& m0, Ms const&... ms) {
        return run(IntegerCommand("SADD", key, m0, ms...));
      }

      template <class K>
      decltype(auto) scard(K const& key) {return run(IntegerCommand("SCARD", key));  }

      template <class V=auto_t, class K, class... Ks>
      decltype(auto) sdiff(K const& key, Ks const&... keys) {
        return run(ArrayCommand<V>("SDIFF", key, keys...));
      }

      template <class Kdst, class K, class... Ks>
      decltype(auto) sdiffstore(Kdst const& dst, K const& key, Ks const&... keys) {
        return run(IntegerCommand("SDIFFSTORE", dst, key, keys...));
      }

      template <class V=auto_t, class K, class... Ks>
      decltype(auto) sinter(K const& key, Ks const&... keys) {
        return run(ArrayCommand<V>("SINTER", key, keys...));
      }

      template <class Kdst, class K, class... Ks>
      decltype(auto) sinterstore(Kdst const& dst, K const& key, Ks const&... keys) {
        return run(IntegerCommand("SINTERSTORE", dst, key, keys...));
      }

      template <class K, class M>
      decltype(auto) sismember(K const& key, M const& member) {
        return run(IntegerCommand("SISMEMBER", key, member));
      }

      template <class V=auto_t, class K>
      decltype(auto) smembers(K const& key) {
        return run(ArrayCommand<V>("SMEMBERS", key));
      }

      template <class Ksrc, class Kdst, class M>
      decltype(auto) smove(Ksrc const& src, Kdst const& dst, M const& member) {
        return run(IntegerCommand("SMOVE", src, dst, member));
      }

      template <class V=auto_t, class K>
      decltype(auto) spop(K const& key, std::int64_t count) {
        return run(ArrayCommand<V>("SPOP", key, std::to_string(count)));
      }

      template <class V=auto_t, class K>
      decltype(auto) spop(K const& key) {
        return run(BulkStringCommand<V>("SPOP", key));
      }

      template <class V=auto_t, class K>
      decltype(auto) srandmember(K const& key, std::int64_t count) {
        return run(ArrayCommand<V>("SRANDMEMBER", key, std::to_string(count)));
      }

      template <class V=auto_t, class K>
      decltype(auto) srandmember(K const& key) {
        return run(BulkStringCommand<V>("SRANDMEMBER", key));
      }

      template <class V=auto_t, class K, class... Ks>
      decltype(auto) sunion(K const& key, Ks const&... keys) {
        return run(ArrayCommand<V>("SUNION", key, keys...));
      }

      template <class Kdst, class K, class... Ks>
      decltype(auto) sunionstore(Kdst const& dst, K const& key, Ks const&... keys) {
        return run(IntegerCommand("SUNIONSTORE", dst, key, keys...));
      }

      template <class K, class M, class... Ms>
      decltype(auto) srem(K const& key, M const& m0, Ms const&... ms) {
        return run(IntegerCommand("SREM", key, m0, ms...));
      }

    };
  }
}

#endif
