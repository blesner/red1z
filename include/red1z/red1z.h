// -*- C++ -*-

#ifndef RED1Z_RED1Z_H
#define RED1Z_RED1Z_H

#include "red1z/context.h"
#include "red1z/interfaces.h"
#include "red1z/transaction.h"
#include "red1z/pipeline.h"

#include <any>

namespace red1z {
  namespace impl {
    class Passtrough :
      public CommandInterface<Passtrough>
    {
      template <class T, class Derived>
      class IntoCommand : public Command<IntoCommand<T, Derived> >{
        T m_dst;
        using Base = Command<IntoCommand<T, Derived>>;
      public:
        IntoCommand(Command<Derived>&& cmd, T dst):
          Base(typename Base::raw(), std::move(cmd.cmd())),
          m_dst(dst)
        {}

        decltype(auto) process(Reply&& e) const {
          return Derived::process_into(std::move(e), m_dst);
        }
      };
    public:
      template <class Cmd>
      Cmd _run(Cmd&& cmd) const { return std::move(cmd); }

      template <class T, class Cmd>
      auto _run_into(Command<Cmd>&& cmd, T out) const {
        return IntoCommand<T, Cmd>(std::move(cmd), out);
      }
    };
  }

  inline static impl::Passtrough commands;

  template <class T=std::string>
  class Message {
    std::string m_channel;
    T m_payload;
    std::optional<std::string> m_pattern = std::nullopt;
  public:
    Message(std::string ch, T payload) :
      m_channel(std::move(ch)),
      m_payload(std::move(payload))
    {
    }

    Message(std::string ch, T payload, std::string ptn) :
      m_channel(std::move(ch)),
      m_payload(std::move(payload)),
      m_pattern(std::move(ptn))
    {
    }

    std::string const& channel() const { return m_channel; }
    std::string& channel() { return m_channel; }
    T const& payload() const { return m_payload; }
    T& payload() { return m_payload; }
    bool has_pattern() const { return m_pattern.has_value(); }
    std::string const& pattern() const { return *m_pattern; }
    std::string& pattern() { return *m_pattern; }
  };

  //  std::variant<std::nullopt_t, Message, PMessage, InfoMessage>;

  class Redis :
    public impl::CommandInterface<Redis>
  {
    impl::Context m_ctx;
  public:
    Redis(std::string const& ip, std::string const& password, int port = 6379) :
      m_ctx(ip, port, password)
    {
    }

    template <class Cmd>
    auto _run(impl::Command<Cmd>&& cmd) {
      return process(cmd.derived(), m_ctx.execute(std::move(cmd).cmd()));
    }

    template <class Cmd, class Out>
    auto _run_into(impl::Command<Cmd>&& cmd, Out dst) {
      return cmd.process_into(m_ctx.execute(std::move(cmd).cmd()), dst);
    }

    template <class... Commands>
    auto transaction(Commands&&... commands) {
      auto p = m_ctx.start_pipeline();
      p.append("*1\r\n$5\r\nMULTI\r\n");
      int _[] = {p.append(std::move(commands).cmd())...};
      (void)_;
      p.append("*1\r\n$4\r\nEXEC\r\n");
      p.discard(1 + sizeof...(Commands));
      return process_transaction_reply
        (p.get_reply(), impl::args_indices_v<Commands...>, commands...);
    }

    template <class T=std::any>
    Transaction<T> transaction() {
      return {m_ctx};
    }

    template <class... Commands>
    auto pipeline(Commands&&... commands) {
      auto p = m_ctx.start_pipeline();
      int _[] = {p.append(std::move(commands).cmd())...};
      (void)_;
      //Too good to be true, argument evaluation order messes things up...
      // return std::make_tuple(Commands::process(p.get_reply())...);
      Reply replies[] = {((void)commands, p.get_reply())...}; //this order is defined ;)
      return process_pipeline_replies(replies, impl::args_indices_v<Commands...>, commands...);
    }

    template <class T=std::any>
    Pipeline<T> pipeline() {
      return {m_ctx};
    }

    template <class... Cs>
    void subscribe(Cs const&... channels) {
      m_ctx.pubsub_run("SUBSCRIBE", channels...);
      //      ++m_subs;
    }

    template <class... Ps>
    void psubscribe(Ps const&... patterns) {
      m_ctx.pubsub_run("PSUBSCRIBE", patterns...);
      //++m_subs;
    }

    template <class... Cs>
    void unsubscribe(Cs const&... channels) {
      m_ctx.pubsub_run("UNSUBSCRIBE", channels...);
      //--m_subs;
    }

    template <class... Ps>
    void punsubscribe(Ps const&... patterns) {
      m_ctx.pubsub_run("PUNSUBSCRIBE", patterns...);
      //--m_subs;
    }


    template <class T=std::string, class Visitor>
    bool get_message(Visitor&& v, int timeout=-1) {
      std::vector<Reply> r;
      if (timeout >= 0) {
        auto msg = m_ctx.get_message(timeout);
        if (!msg) {
          return false;
        }
        r = std::move(*msg).array();
      }
      else {
        r = m_ctx.get_message().array();
      }

      auto type = std::move(r[0]).string();
      if (type == "message") {
        std::forward<Visitor>(v).message(std::move(r[1]).string(), std::move(r[2]).get<T>());
      }
      else if (type == "pmessage") {
        std::forward<Visitor>(v).pmessage(std::move(r[1]).string(),
                                          std::move(r[2]).string(),
                                          std::move(r[3]).get<T>());
      }
      else {
        //m_ack_subs = r[2].integer();
        std::forward<Visitor>(v).info(std::move(type), std::move(r[1]).string(), r[2].integer());//m_ack_subs);
      }
      return true;
    }

    template <class T=std::string>
    Message<T> get_message() {
      for (;;) {
        if (auto msg = process_message<T>(m_ctx.get_message())) {
          return *msg;
        }
      }
    }

    template <class T=std::string>
    std::optional<Message<T>> get_message(int timeout) {
      using namespace std::chrono;
      int elapsed = 0;
      auto start = steady_clock::now();
      while (auto msg = m_ctx.get_message(std::max(timeout - elapsed, 0))) {
        if (auto m = process_message<T>(std::move(*msg))) {
          return m;
        }
        elapsed = duration_cast<milliseconds>(start - steady_clock::now()).count();
      }
      return std::nullopt;
    }

  private:
    template <class... Commands, std::size_t... I>
    static auto
    process_pipeline_replies(Reply* rep, std::index_sequence<I...>,  Commands const&... cmds) {
      return std::make_tuple(process(cmds, std::move(rep[I]))...);
    }


    template <class... Commands, std::size_t... I>
    auto process_transaction_reply(Reply&& r, std::index_sequence<I...>, Commands const&... cmds) {
      if (r.array_size() != sizeof...(Commands)) {
        throw Error("unexpected reply size for EXEC");
      }
      auto elements = std::move(r).array();
      return std::make_tuple(process(cmds, std::move(elements[I]))...);
    }

    template <class Cmd>
    static auto process(impl::Command<Cmd> const& cmd, Reply&& r) {
      return cmd.process(std::move(r));
    }

    template <class T>
    static std::optional<Message<T>> process_message(Reply&& r) {
      auto reply = std::move(r).array();
      auto type = std::move(reply[0]).string();
      if (type == "message") {
        return Message<T>{std::move(reply[1]).string(), std::move(reply[2]).get<T>()};
      }
      if (type == "pmessage") {
        return Message<T>(std::move(reply[2]).string(),
                          std::move(reply[3]).get<T>(),
                          std::move(reply[2]).string());
      }
      else {
        auto ch = std::move(reply[1]).string();
        auto n = reply[2].integer();
        std::cout << "META MESSAGE: " << type << ", " << ch<< " ("<< n<< ')' <<'\n';
      }
      return std::nullopt;
    }
  };

  template <class Container>
  auto unpack(Container const& c) {
    return impl::ArgPack(std::begin(c), std::end(c));
  }

  // template <class T>
  // auto unpack(std::initializer_list<T> ilist) {
  //   return impl::ArgPack(std::begin(ilist), std::end(ilist));
  // }

  template <class It1, class It2>
  impl::ArgPack<It1, It2> unpack(It1 b, It2 e) {
    return {b, e};
  }

  template <class T>
  std::optional<T> opt(std::any const& v) {
    return std::any_cast<std::optional<T>>(v);
  }

  template <class T>
  T get(std::any const& v) {
    return std::any_cast<T>(v);
  }
}

#endif
