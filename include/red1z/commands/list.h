// -*- C++ -*-
#ifndef RED1Z_COMMAND_LIST_H
#define RED1Z_COMMAND_LIST_H

namespace red1z {
  namespace impl {
    template <class Executor>
    class ListCommands {
      template <class Cmd>
      decltype(auto) run(Cmd&& cmd) {
        return static_cast<Executor*>(this)->_run(std::move(cmd));
      }
    public:
      template <class K=std::string, class V=std::string, class Key, class... Args>
      decltype(auto) blpop(Key const& key, Args const&... args) {
        static constexpr int N = sizeof...(Args);
        static_assert(sizeof...(Args) > 0);
        return run(OptTupleCommand<K, V>(converter<nth_arg<N>, int2str>(), "BLPOP", key, args...));
      }

      template <class K=std::string, class V=std::string, class Key, class... Args>
      decltype(auto) brpop(Key const& key, Args const&... args) {
        static constexpr int N = sizeof...(Args);
        static_assert(sizeof...(Args) > 0);
        return run(OptTupleCommand<K, V>(converter<nth_arg<N>, int2str>(), "BRPOP", key, args...));
      }

      template <class V=auto_t, class Ksrc, class Kdst>
      decltype(auto) brpoplpush(Ksrc const& src, Kdst const& dst, std::int64_t timeout) {
        return run(BulkStringCommand<V>("BRPOPLPUSH", src, dst, std::to_string(timeout)));
      }

      template <class V=auto_t, class K>
      decltype(auto) lindex(K const& key, std::int64_t index) {
        return run(BulkStringCommand<V>("LINDEX", key, std::to_string(index)));
      }

      // template <class K, class Vp, class Ve>
      // decltype(auto) linsert_before(K const& key, Vp const& pivot, Ve const& element) {
      //   return run(IntegerCommand("LINSERT", key, "BEFORE", pivot, element));
      // }

      // template <class K, class Vp, class Ve>
      // decltype(auto) linsert_after(K const& key, Vp const& pivot, Ve const& element) {
      //   return run(IntegerCommand("LINSERT", key, "AFTER", pivot, element));
      // }

      template <class K, class Vp, class Ve>
      decltype(auto) linsert(K const& key, flag<flags::_linsert> f,
                             Vp const& pivot, Ve const& element)
      {
        return run(IntegerCommand("LINSERT", key, f, pivot, element));
      }

      template <class K>
      decltype(auto) llen(K const& key) { return run(IntegerCommand("LLEN", key)); }

      template <class V=auto_t, class K>
      decltype(auto) lpop(K const& key) {
        return run(BulkStringCommand<V>("LPOP", key));
      }

      template <class K, class E, class... Es>
      decltype(auto) lpush(K const& key, E const& e0, Es const&... es) {
        return run(IntegerCommand("LPUSH", key, e0, es...));
      }

      template <class K, class E, class... Es>
      decltype(auto) lpushx(K const& key, E const& e0, Es const&... es) {
        return run(IntegerCommand("LPUSHX", key, e0, es...));
      }

      template <class V=auto_t, class K>
      decltype(auto) lrange(K const& key, std::int64_t start, std::int64_t stop) {
        return run(ArrayCommand<V>("LRANGE", key, std::to_string(start), std::to_string(stop)));
      }

      template <class K, class E>
      decltype(auto) lrem(K const& key, std::int64_t count, E const& element) {
        return run(IntegerCommand("LREM", key, std::to_string(count), element));
      }

      template <class K, class E>
      decltype(auto) lset(K const& key, std::int64_t index, E const& element) {
        return run(StatusCommand("LSET", key, std::to_string(index), element));
      }

      template <class K>
      decltype(auto) ltrim(K const& key, std::int64_t start, std::int64_t stop) {
        return run(StatusCommand("LTRIM", key, std::to_string(start), std::to_string(stop)));
      }

      template <class V=auto_t, class K>
      decltype(auto) rpop(K const& key) {
        return run(BulkStringCommand<V>("RPOP", key));
      }

      template <class K, class E, class... Es>
      decltype(auto) rpush(K const& key, E const& e0, Es const&... es) {
        return run(IntegerCommand("RPUSH", key, e0, es...));
      }

      template <class K, class E, class... Es>
      decltype(auto) rpushx(K const& key, E const& e0, Es const&... es) {
        return run(IntegerCommand("RPUSHX", key, e0, es...));
      }

      template <class V=auto_t, class Ksrc, class Kdst>
      decltype(auto) rpoplpush(Ksrc const& src, Kdst const& dst) {
        return run(BulkStringCommand<V>("RPOPLPUSH", src, dst));
      }

    };
  }
}
#endif
