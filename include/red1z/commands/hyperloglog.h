// -*- C++ -*-
#ifndef RED1Z_COMMAND_HYPERLOGLOG_H
#define RED1Z_COMMAND_HYPERLOGLOG_H

namespace red1z {
  namespace impl {
    template <class Executor>
    class HyperLogLogCommands {
      template <class Cmd>
      decltype(auto) run(Cmd&& cmd) {
        return static_cast<Executor*>(this)->_run(std::move(cmd));
      }
    public:
      template <class K, class... Es>
      decltype(auto) pfadd(K const& key, Es const&... elements) {
        return run(IntegerCommand("PFADD", key, elements...));
      }

      template <class K, class... Ks>
      decltype(auto) pfcount(K const& key, Ks const&... keys) {
        return run(IntegerCommand("PFCOUNT", key, keys...));
      }

      template <class Kdst, class Ksrc, class... Ksrcs>
      decltype(auto) pfmerge(Kdst const& dst, Ksrc const& src, Ksrcs const&... srcs) {
        return run(StatusCommand("PFMERGE", dst, src, srcs...));
      }
    };
  }
}

#endif
