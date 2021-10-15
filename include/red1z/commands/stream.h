// -*- C++ -*-
#ifndef RED1Z_COMMAND_STREAM_H
#define RED1Z_COMMAND_STREAM_H

#include <type_traits>
#include <unordered_map>

#include "red1z/signature.h"

namespace red1z {

  namespace streams {
    using default_entry_type = std::unordered_map<std::string, std::string>;

    template <class Entry> struct entry_type_traits {};

    template <class T>
    struct entry_type_traits<std::unordered_map<std::string, T>> {
      using name_type = std::string;
      using value_type = T;
      static void set_field(std::unordered_map<std::string, T> &entry,
                            std::string name, T value) {
        entry.emplace(std::move(name), std::move(value));
      }
    };

    template <class T>
    struct entry_type_traits<std::vector<std::tuple<std::string, T>>> {
      using name_type = std::string;
      using value_type = T;
      static void set_field(std::vector<std::tuple<std::string, T>> &entry,
                            std::string name, T value) {
        entry.emplace_back(std::move(name), std::move(value));
      }
    };
  } // namespace streams

  namespace impl {
    struct _STREAMS {
      static constexpr char const *txt = "STREAMS";
    };

    template <class T = std::string>
    struct SimpleXPendingCommand : Command<SimpleXPendingCommand<T>> {
      using Command<SimpleXPendingCommand<T>>::Command;
      using result_type = std::tuple<std::int64_t, std::string, std::string,
                                     std::vector<std::tuple<T, std::int64_t>>>;

      static void process_into(Reply &&r, result_type *out) {
        auto elements = std::move(r).array(4);
        auto c = std::move(elements[3]).array();
        std::vector<std::tuple<std::string, std::int64_t>> cinfo;
        cinfo.reserve(c.size());
        for (auto &&cons : c) {
          auto info = std::move(cons).array(2);
          cinfo.emplace_back(std::move(info[0]).string(),
                             std::stoi(std::move(info[1]).string()));
        }
        *out = std::make_tuple(
            elements[0].integer(), std::move(elements[1]).string(),
            std::move(elements[2]).string(), std::move(cinfo));
      }

      static result_type process(Reply &&r) {
        result_type out;
        process_into(std::move(r), &out);
        return out;
      }
    };

    template <class T = std::string>
    struct ExtendedXPendingCommand
        : BasicArrayCommand<
              std::tuple<std::string, T, std::int64_t, std::int64_t>,
              ExtendedXPendingCommand<T>, std::string> {
      using Base = BasicArrayCommand<
          std::tuple<std::string, T, std::int64_t, std::int64_t>,
          ExtendedXPendingCommand<T>, std::string>;
      using Base::Base;

      template <class U, class OutputIt>
      static std::int64_t process_into_impl(Reply &&r, OutputIt out) {
        auto elements = std::move(r).array();
        for (auto &rr : elements) {
          auto info = std::move(rr).array(4);
          *out++ = std::make_tuple(std::move(info[0]).string(),
                                   std::move(info[1]).get<T>(),
                                   info[2].integer(), info[3].integer());
        }
        return elements.size();
      }
    };

    template <class EntryType>
    struct StreamReadCommand : Command<StreamReadCommand<EntryType>> {
      using Command<StreamReadCommand<EntryType>>::Command;
      using result_type = std::vector<std::tuple<std::string, EntryType>>;

      static void process_into(Reply &&r, result_type *out) {
        auto elements = std::move(r).array();
        for (auto &rr : elements) {
          auto entry = std::move(rr).array(2);
          auto fields = std::move(entry[1]).array();
          if (fields.size() % 2) {
            throw Error("unexpected fields reply size");
          }
          int const n = fields.size() / 2;

          EntryType ent;
          using Traits = streams::entry_type_traits<EntryType>;
          for (int i = 0; i < n; ++i) {
            Traits::set_field(
                ent, std::move(fields[2 * i]).get<typename Traits::name_type>(),
                std::move(fields[2 * i + 1])
                    .get<typename Traits::value_type>());
          }

          out->emplace_back(std::move(entry[0]).string(), std::move(ent));
        }
      }

      static result_type process(Reply &&r) {
        result_type out;
        process_into(std::move(r), &out);
        return out;
      }
    };

    template <class EntryType>
    struct MultiStreamReadCommand : Command<MultiStreamReadCommand<EntryType>> {
      using Command<MultiStreamReadCommand<EntryType>>::Command;
      using result_type = std::vector<std::tuple<
          std::string, typename StreamReadCommand<EntryType>::result_type>>;

      static void process_into(Reply &&r, result_type *out) {
        auto elements = std::move(r).array();
        for (auto &rr : elements) {
          auto e = std::move(rr).array(2);
          out->emplace_back(
              std::move(e[0]).string(),
              StreamReadCommand<EntryType>::process(std::move(e[1])));
        }
      }

      static result_type process(Reply &&r) {
        result_type out;
        process_into(std::move(r), &out);
        return out;
      }
    };

    template <class EntryType>
    struct OptMultiStreamReadCommand
        : Command<OptMultiStreamReadCommand<EntryType>> {
      using Command<OptMultiStreamReadCommand<EntryType>>::Command;
      using result_type = std::optional<
          typename MultiStreamReadCommand<EntryType>::result_type>;

      static void process_into(Reply &&r, result_type *out) {
        if (!r) {
          *out = std::nullopt;
        } else {
          *out = MultiStreamReadCommand<EntryType>::process(std::move(r));
        }
      }

      static result_type process(Reply &&r) {
        result_type out;
        process_into(std::move(r), &out);
        return out;
      }
    };

    template <class EntryType>
    struct AutoClaimCommand : Command<AutoClaimCommand<EntryType>> {
      using Command<AutoClaimCommand<EntryType>>::Command;
      using result_type =
          std::tuple<std::string,
                     typename StreamReadCommand<EntryType>::result_type>;

      static void process_into(Reply &&r, result_type *out) {
        auto tup = std::move(r).array(2);
        *out = std::make_tuple(
            std::move(tup[0]).string(),
            StreamReadCommand<EntryType>::process(std::move(tup[1])));
      }

      static result_type process(Reply &&r) {
        result_type out;
        process_into(std::move(r), &out);
        return out;
      }
    };

    struct AutoClaimCommandJustId : Command<AutoClaimCommandJustId> {
      using Command<AutoClaimCommandJustId>::Command;
      using result_type = std::tuple<std::string, std::vector<std::string>>;

      static void process_into(Reply &&r, result_type *out) {
        auto tup = std::move(r).array(2);
        *out = std::make_tuple(
            std::move(tup[0]).string(),
            ArrayCommand<std::string>::process(std::move(tup[1])));
      }

      static result_type process(Reply &&r) {
        result_type out;
        process_into(std::move(r), &out);
        return out;
      }
    };

    template <class Executor> class StreamCommands {
      template <class Cmd> decltype(auto) run(Cmd &&cmd) {
        return static_cast<Executor *>(this)->_run(std::move(cmd));
      }

    public:
      template <class K, class Grp, class Id, class... Ids>
      decltype(auto) xack(K const &key, Grp const &group, Id const &id,
                          Ids &&... ids) {
        return run(
            IntegerCommand("XACK", key, group, id, std::forward<Ids>(ids)...));
      }

      template <class K = std::string, class... FVs>
      decltype(auto) xadd(K const &key, FVs &&... fields) {
        using s = sig::signature<sig::arg<>, sig::flag<flags::_nomkstream>,
                                 sig::flag<flags::_maxlen_or_minid>, sig::arg<>,
                                 sig::pair<sig::arg<>, sig::arg<>>,
                                 sig::many<sig::pair<sig::arg<>, sig::arg<>>>>;
        return run(SimpleStringCommand("XADD", s(), key,
                                       std::forward<FVs>(fields)...));
      }

      template <class T = std::string, class K, class Grp, class C,
                class... Args>
      decltype(auto) xautoclaim(K const &key, Grp const &group,
                                C const &consumer, std::int64_t min_idle_time,
                                Args &&... args) {
        using s =
            sig::signature<sig::arg<>, sig::arg<>, sig::arg<>,
                           sig::arg<sig::_int>, sig::arg<>,
                           sig::many<sig::arg<>>, sig::flag<flags::_count>,
                           sig::flag<flags::_justid>>;

        if constexpr (has_flag_v<flags::_justid, Args...>) {
          return run(ArrayCommand<std::string>("XAUTOCLAIM", s(), key, group,
                                               consumer, min_idle_time,
                                               std::forward<Args>(args)...));
        } else {
          return run(AutoClaimCommand<T>("XAUTOCLAIM", s(), key, group,
                                         consumer, min_idle_time,
                                         std::forward<Args>(args)...));
        }
      }

      template <class T = std::string, class K, class Grp, class C,
                class... Args>
      decltype(auto) xclaim(K const &key, Grp const &group, C const &consumer,
                            std::int64_t min_idle_time, Args &&... args) {
        using s = sig::signature<
            sig::arg<>, sig::arg<>, sig::arg<>, sig::arg<sig::_int>, sig::arg<>,
            sig::many<sig::arg<>>, sig::flag<flags::_idle>,
            sig::flag<flags::_time>, sig::flag<flags::_retrycount>,
            sig::flag<flags::_force>, sig::flag<flags::_justid>>;

        if constexpr (has_flag_v<flags::_justid, Args...>) {
          return run(ArrayCommand<std::string>("XCLAIM", s(), key, group,
                                               consumer, min_idle_time,
                                               std::forward<Args>(args)...));
        } else {
          return run(StreamReadCommand<T>("XCLAIM", s(), key, group, consumer,
                                          min_idle_time,
                                          std::forward<Args>(args)...));
        }
      }

      template <class K, class Id, class... Ids>
      decltype(auto) xdel(K const &key, Id const &id, Ids &&... ids) {
        return run(IntegerCommand("XDEL", key, id, std::forward<Ids>(ids)...));
      }

      template <class K, class Grp, class Id>
      decltype(auto) xgroup_create(K const &key, Grp const &groupname,
                                   Id const &id) {
        return run(StatusCommand("XGROUP", "CREATE", key, groupname, id));
      }

      template <class K, class Grp, class Id>
      decltype(auto) xgroup_create(K const &key, Grp const &groupname,
                                   Id const &id, flag<flags::_mkstream> f) {
        return run(StatusCommand("XGROUP", "CREATE", key, groupname, id, f));
      }

      template <class K, class Grp, class Id>
      decltype(auto) xgroup_setid(K const &key, Grp const &groupname,
                                  Id const &id) {
        return run(StatusCommand("XGROUP", "SETID", key, groupname, id));
      }

      template <class K, class Grp, class C>
      decltype(auto) xgroup_createconsumer(K const &key, Grp const &groupname,
                                           C const &consumername) {
        return run(IntegerCommand("XGROUP", "CREATECONSUMER", key, groupname,
                                  consumername));
      }

      template <class K, class Grp, class C>
      decltype(auto) xgroup_delconsumer(K const &key, Grp const &groupname,
                                        C const &consumername) {
        return run(IntegerCommand("XGROUP", "DELCONSUMER", key, groupname,
                                  consumername));
      }

      // XINFO

      template <class K> decltype(auto) xlen(K const &key) {
        return run(IntegerCommand("XLEN", key));
      }

      template <class T = std::string, class K, class Grp>
      decltype(auto) xpending(K const &key, Grp const &group) {
        return run(SimpleXPendingCommand<T>("XPENDING", key, group));
      }

      template <class T = std::string, class K, class Grp, class... Flag,
                class C>
      decltype(auto) xpending(K const &key, Grp const &group,
                              flag<flags::_idle, Flag...> f,
                              std::string_view start, std::string_view end,
                              std::int64_t count, C &&consumer) {
        return run(ExtendedXPendingCommand<T>("XPENDING", key, group, f, start,
                                              end, std::to_string(count),
                                              std::forward<C>(consumer)));
      }

      template <class T = std::string, class K, class Grp, class C>
      decltype(auto) xpending(K const &key, Grp const &group,
                              std::string_view start, std::string_view end,
                              std::int64_t count, C &&consumer) {
        return run(ExtendedXPendingCommand<T>("XPENDING", key, group, start,
                                              end, std::to_string(count),
                                              std::forward<C>(consumer)));
      }

      template <class T = std::string, class K, class Grp, class... Flag>
      decltype(auto) xpending(K const &key, Grp const &group,
                              flag<flags::_idle, Flag...> f,
                              std::string_view start, std::string_view end,
                              std::int64_t count) {
        return run(ExtendedXPendingCommand<T>("XPENDING", key, group, f, start,
                                              end, std::to_string(count)));
      }

      template <class T = std::string, class K, class Grp>
      decltype(auto) xpending(K const &key, Grp const &group,
                              std::string_view start, std::string_view end,
                              std::int64_t count) {
        return run(ExtendedXPendingCommand<T>("XPENDING", key, group, start,
                                              end, std::to_string(count)));
      }

      template <class T = streams::default_entry_type, class K>
      decltype(auto) xrange(K &&key, std::string_view start,
                            std::string_view end) {
        return run(
            StreamReadCommand<T>("XRANGE", std::forward<K>(key), start, end));
      }

      template <class T = streams::default_entry_type, class K, class... Flag>
      decltype(auto) xrange(K &&key, std::string_view start,
                            std::string_view end,
                            flag<flags::_count, Flag...> const &f) {
        return run(StreamReadCommand<T>("XRANGE", std::forward<K>(key), start,
                                        end, f));
      }

      template <class T = streams::default_entry_type, class... Args>
      decltype(auto) xread(Args &&... args) {
        using s =
            sig::signature<sig::flag<flags::_count>, sig::flag<flags::_block>,
                           sig::kw<0>, sig::many<sig::arg<>>>;

        return run(OptMultiStreamReadCommand<T>("XREAD", {"STREAMS"}, s(),
                                                std::forward<Args>(args)...));
      }

      template <class T = streams::default_entry_type, class... Args>
      decltype(auto) xreadgroup(Args &&... args) {
        using s =
            sig::signature<sig::kw<0>, sig::arg<>, sig::arg<>,
                           sig::flag<flags::_count>, sig::flag<flags::_block>,
                           sig::flag<flags::_noack>, sig::kw<1>,
                           sig::many<sig::arg<>>>;

        return run(OptMultiStreamReadCommand<T>("XREADGROUP",
                                                {"GROUP", "STREAMS"}, s(),
                                                std::forward<Args>(args)...));
      }

      template <class T = streams::default_entry_type, class K, class S,
                class E>
      decltype(auto) xrevrange(K &&key, S &&start, S &&end) {
        return run(StreamReadCommand<T>("XREVRANGE", std::forward<K>(key),
                                        std::forward<S>(start),
                                        std::forward<E>(end)));
      }

      template <class T = streams::default_entry_type, class K, class S,
                class E, class... Flag>
      decltype(auto) xrevrange(K &&key, S &&start, S &&end,
                               flag<flags::_count, Flag...> const &f) {
        return run(StreamReadCommand<T>("XREVRANGE", std::forward<K>(key),
                                        std::forward<S>(start),
                                        std::forward<E>(end), f));
      }

      template <class K, class... Flag>
      decltype(auto) xtrim(K &&key,
                           flag<flags::_maxlen_or_minid, Flag...> const &f) {
        return run(IntegerCommand("XTRIM", std::forward<K>(key), f));
      }
    };

  } // namespace impl
} // namespace red1z
#endif
