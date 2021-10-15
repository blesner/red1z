// -*- C++ -*-
#ifndef RED1Z_INTERFACES_H
#define RED1Z_INTERFACES_H

#include "red1z/commands/generic.h"
#include "red1z/commands/hash.h"
#include "red1z/commands/hyperloglog.h"
#include "red1z/commands/list.h"
#include "red1z/commands/set.h"
#include "red1z/commands/sorted_set.h"
#include "red1z/commands/stream.h"
#include "red1z/commands/string.h"

namespace red1z {
  namespace impl {
    template <class Executor>
    struct BasicCommandInterface : GenericCommands<Executor>,
                                   StringCommands<Executor>,
                                   ListCommands<Executor>,
                                   HashCommands<Executor>,
                                   SetCommands<Executor>,
                                   SortedSetCommands<Executor>,
                                   HyperLogLogCommands<Executor>,
                                   StreamCommands<Executor> {};

    template <class Executor, class T>
    class Into : public BasicCommandInterface<Into<Executor, T>> {
      Executor &m_exec;
      T m_dst;

    public:
      Into(Executor &ex, T const &dst) : m_exec(ex), m_dst(dst){};

      template <class Cmd> decltype(auto) _run(Command<Cmd> &&cmd) {
        return m_exec._run_into(std::move(cmd), m_dst);
      }
    };

    template <class Executor>
    struct CommandInterface : BasicCommandInterface<Executor> {
      template <class U> Into<Executor, U *> operator[](U *out) {
        return {*static_cast<Executor *>(this), out};
      }

      template <class OutputIterator>
      Into<Executor, OutputIterator> operator[](OutputIterator out) {
        return {*static_cast<Executor *>(this), out};
      }
    };
  } // namespace impl
} // namespace red1z

#endif
