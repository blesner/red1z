// -*- C++ -*-
#ifndef RED1Z_BASIC_PIPELINE_H
#define RED1Z_BASIC_PIPELINE_H

#include "red1z/interfaces.h"

namespace red1z {
  namespace impl {
    template <class T>
    struct Resolver {
      virtual ~Resolver() {}
      virtual T resolve(Reply&&) = 0;
    };

    template <class T, class Cmd>
    struct SimpleResolver : Resolver<T> {
    protected:
      impl::Command<Cmd> m_cmd;
    public:
      SimpleResolver(impl::Command<Cmd>&& cmd) : m_cmd(std::move(cmd)) {}
      T resolve(Reply&& r) override {
        return m_cmd.process(std::move(r));
      }
    };

    template <class T, class Cmd, class Out>
    class IntoResolver : public SimpleResolver<T, Cmd> {
      Out m_out;
      using Base = SimpleResolver<T, Cmd>;
    public:
      IntoResolver(impl::Command<Cmd>&& cmd, Out out) :
        Base(std::move(cmd)), m_out(out)
      {}

      T resolve(Reply&& r) override {
        return this->m_cmd.process_into(std::move(r), m_out);
      }
    };

    template <class T, class Derived>
    class BasicPipeline :
      public CommandInterface<BasicPipeline<T, Derived>>
    {
    protected:
      CommandQueue m_queue;
      std::vector<std::unique_ptr<Resolver<T>>> m_resolvers;
      bool m_resolved = false;

      void _discard() {};
    public:
      BasicPipeline(Context& c) : m_queue(c.start_pipeline()) {}

      ~BasicPipeline() {
        if (!m_resolved) {
          discard();
        }
      }

      template <class Cmd>
      Derived& _run(impl::Command<Cmd>&& cmd) {
        append(std::move(cmd).cmd());
        m_resolvers.push_back(std::make_unique<SimpleResolver<T, Cmd>>(std::move(cmd)));
        return self();
      }

      template <class Cmd, class Out>
      Derived& _run_into(Command<Cmd>&& cmd, Out out) {
        append(std::move(cmd).cmd());
        m_resolvers.push_back(std::make_unique<IntoResolver<T, Cmd, Out>>(std::move(cmd), out));
        return self();
      }

      std::vector<T> execute() {
        mark_resolved();
        int const n = m_resolvers.size();
        std::vector<T> result;
        result.reserve(n);
        auto resolver =
          [i=0, this, &result](Reply&& r) mutable {
            result.push_back(m_resolvers[i++]->resolve(std::move(r)));
          };
        self()._execute(n, resolver);
        return result;
      }

      void discard() {
        mark_resolved();
        self()._discard();
        m_queue.discard();
      }

    private:
      Derived& self() { return *static_cast<Derived*>(this); }

      void check_unresolved() const {
        if (m_resolved) {
          throw Error("pipeline already resolved");
        }
      }

      void mark_resolved() {
        if (std::exchange(m_resolved, true)) {
          throw Error("pipeline already resolved");
        }
      }

      void append(std::string&& cmd) {
        check_unresolved();
        m_queue.append(std::move(cmd));
      }
    };
  }
}

#endif
