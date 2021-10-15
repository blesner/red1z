// -*- C++ -*-
#ifndef RED1Z_COMMAND_GENERIC_H
#define RED1Z_COMMAND_GENERIC_H

namespace red1z {
  namespace impl {
    template <class K> struct ScanCommand : Command<ScanCommand<K>> {
      using Base = Command<ScanCommand<K>>;
      using T = auto_type_t<K, std::string>;

      template <class... Flags>
      ScanCommand(std::int64_t cursor, Flags const &... flags)
          : Base("SCAN",
                 sig::signature<sig::arg<sig::_int>, sig::flag<flags::_match>,
                                sig::flag<flags::_count>,
                                sig::flag<flags::_type>>(),
                 std::to_string(cursor), flags...) {}

      static std::tuple<std::uint64_t, std::vector<T>> process(Reply &&r) {
        std::vector<T> out;
        auto cursor = process_into(std::move(r), &out);
        return {cursor, std::move(out)};
      }

      static std::uint64_t process_into(Reply &&r, std::vector<T> *out) {
        out->clear();
        return process_into(std::move(r), std::back_inserter(*out));
      }

      template <class OutputIt>
      static std::uint64_t process_into(Reply &&r, OutputIt out) {
        auto elements = std::move(r).array(2);
        ArrayCommand<K>::process_into(std::move(elements[1]), out);
        return atol(std::move(elements[0]).string().c_str());
      }
    };

    template <class Executor> class GenericCommands {
      template <class Cmd> decltype(auto) run(Cmd &&cmd) {
        return static_cast<Executor *>(this)->_run(std::move(cmd));
      }

    public:
      template <class Key, class... Keys>
      decltype(auto) del(Key const &key0, Keys const &... keys) {
        return run(IntegerCommand("DEL", key0, keys...));
      }

      template <class Kout = auto_t, class K>
      decltype(auto) dump(K const &key) {
        return run(BulkStringCommand<Kout>("DUMP", key));
      }

      template <class Key, class... Keys>
      decltype(auto) exists(Key const &key0, Keys const &... keys) {
        return run(IntegerCommand("EXISTS", key0, keys...));
      }

      template <class K>
      decltype(auto) expire(K const &key, std::int64_t seconds) {
        return run(IntegerCommand("EXPIRE", key, std::to_string(seconds)));
      }

      template <class K>
      decltype(auto) expireat(K const &key, std::int64_t timestamp) {
        return run(IntegerCommand("EXPIREAT", key, std::to_string(timestamp)));
      }

      template <class Kout = auto_t>
      decltype(auto) keys(std::string_view pattern) {
        return run(ArrayCommand<Kout>("KEYS", pattern));
      }

      // MIGRATE

      template <class K> decltype(auto) move(K const &key, std::int64_t db) {
        return run(IntegerCommand("MOVE", key, std::to_string(db)));
      }

      // decltype(auto) object(impl::_flag<flags::_refcount> const& f) {
      //   return run(OptionalIntegerCommand("OBJECT", f));
      // }

      // decltype(auto) object(impl::_flag<flags::_idletime> const& f) {
      //   return run(OptionalIntegerCommand("OBJECT", f));
      // }

      // decltype(auto) object(impl::_flag<flags::_freq> const& f) {
      //   return run(OptionalIntegerCommand("OBJECT", f));
      // }

      // decltype(auto) object(impl::_flag<flags::_encoding> const& f) {
      //   return run(BulkStringCommand<std::string>("OBJECT", f));
      // }

      template <class K> decltype(auto) persist(K const &key) {
        return run(IntegerCommand("PERSIST", key));
      }

      template <class K>
      decltype(auto) pexpire(K const &key, std::int64_t milliseconds) {
        return run(
            IntegerCommand("PEXPIRE", key, std::to_string(milliseconds)));
      }

      template <class K>
      decltype(auto) pexpireat(K const &key, std::int64_t millitimestamp) {
        return run(
            IntegerCommand("PEXPIREAT", key, std::to_string(millitimestamp)));
      }

      template <class K> decltype(auto) pttl(K const &key) {
        return run(IntegerCommand("PTTL", key));
      }

      template <class Kout = auto_t> decltype(auto) randomkey() {
        return run(BulkStringCommand<Kout>("RANDOMKEY"));
      }

      template <class K0, class K1>
      decltype(auto) rename(K0 const &old_name, K1 const &new_name,
                            bool nx = false) {
        if (nx) {
          return renamenx(old_name, new_name);
        }
        return run(StatusCommand("RENAME", old_name, new_name));
      }

      template <class K0, class K1>
      decltype(auto) renamenx(K0 const &old_name, K1 const &new_name) {
        return run(StatusCommand("RENAMENX", old_name, new_name));
      }

      template <class K, class V, class... Flags>
      decltype(auto) restore(K const &key, std::int64_t ttl, V const &val,
                             Flags const &... flags) {
        using s = sig::signature<
            sig::arg<>, sig::arg<sig::_int>, sig::arg<>,
            sig::flag<flags::_replace>, sig::flag<flags::_absttl>,
            sig::flag<flags::_idletime>, sig::flag<flags::_freq>>;
        return run(StatusCommand("RESTORE", s(), key, ttl, val, flags...));
      }

      template <class V = auto_t, class K, class... Flags>
      decltype(auto) sort(K const &key, Flags const &... flags) {

        using s = sig::signature<
            sig::arg<>, sig::flag<flags::_by>, sig::flag<flags::_limit>,
            sig::many<sig::flag<flags::_get>>, sig::flag<flags::_order>,
            sig::flag<flags::_alpha>, sig::flag<flags::_store>>;

        if constexpr (has_flag_v<flags::_store, Flags...>) {
          return run(IntegerCommand("SORT", s(), key, flags...));
        } else {
          return run(ArrayCommand<V>("SORT", s(), key, flags...));
        }
      }

      template <class Key0, class... Keys>
      decltype(auto) touch(Key0 const &k0, Keys const &... ks) {
        return run(IntegerCommand("TOUCH", k0, ks...));
      }

      template <class K> decltype(auto) ttl(K const &key) {
        return run(IntegerCommand("TTL", key));
      }

      template <class K> decltype(auto) type(K const &key) {
        return run(SimpleStringCommand("TYPE", key));
      }

      template <class Key, class... Keys>
      decltype(auto) unlink(Key const &key0, Keys const &... keys) {
        return run(IntegerCommand("UNLINK", key0, keys...));
      }

      decltype(auto) wait(std::int64_t numreplicas, std::int64_t timeout) {
        return run(IntegerCommand("WAIT", std::to_string(numreplicas),
                                  std::to_string(timeout)));
      }

      template <class K = auto_t, class... Flags>
      decltype(auto) scan(std::int64_t cursor, Flags const &... flags) {
        return run(ScanCommand<K>(cursor, flags...));
      }

      template <class M>
      decltype(auto) publish(std::string_view channel, M &&msg) {
        return run(IntegerCommand("PUBLISH", channel, std::forward<M>(msg)));
      }
    };
  } // namespace impl
} // namespace red1z

#endif
