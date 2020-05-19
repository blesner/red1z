// -*- C++ -*-
#ifndef RED1Z_CONTEXT_H
#define RED1Z_CONTEXT_H

#include "red1z/command.h"
#include "red1z/socket.h"

namespace red1z {
  namespace impl {
    class Context;

    class CommandQueue {
      Context* m_ctx;
    public:
      CommandQueue(Context* ctx) : m_ctx(ctx) {}

      inline int append(std::string&& cmd);
      inline void discard(int count);
      inline void discard();
      inline Reply get_reply();
      inline ~CommandQueue();
    };

    class Context {
      Socket m_sock;
      int m_in_flight = 0;
      std::vector<std::string> m_queue;
      friend class CommandQueue;
    public:
      Context(std::string const& host, int port, std::string_view password) :
        m_sock(host, port)
      {
        if (password.size()) {
          run("AUTH", password);
        }
      }

      int in_flight() const {
        return m_in_flight;
      }

      bool ready() const { return m_in_flight == 0; }

      Reply get_reply() {
        if (m_in_flight == 0) {
          throw Error("cannot get reply: no requests in flight");
        }
        --m_in_flight;
        send();
        return {m_sock};
      }

      Reply get_message() {
        return {Reply(m_sock)};
      }

      std::optional<Reply> get_message(int timeout) {
        if (m_sock.wait(timeout)) {
          return {Reply(m_sock)};
        }
        return std::nullopt;
      }

      Reply execute(std::string&& cmd) {
        if (not ready()) {
          throw Error("cannot execute command: requests are pending");
        }
        append(std::move(cmd));
        return get_reply();
      };

      template <class... Args>
      Reply run(Args const&... cmd) {
        return execute(encode_command(cmd...));
      }

      template <class... Args>
      void pubsub_run(Args const&... cmd) {
        auto c = encode_command(cmd...);
        m_sock.write(c.data(), c.size());
      }

      CommandQueue start_pipeline() {
        if (not ready()) {
          throw Error("cannot start pipeline: ", m_in_flight, " requests are pending");
        }
        return {this};
      }

    private:
      int append(std::string&& cmd) {
        m_queue.emplace_back(std::move(cmd));
        return ++m_in_flight;
      }

      void discard_reply() {
        get_reply();
      }

      void send() {
        m_sock.write_many(m_queue);
        m_queue.clear();
      }
    };


    int CommandQueue::append(std::string&& cmd) {
      return m_ctx->append(std::move(cmd));
    }

    void CommandQueue::discard(int count) {
      if (int n = m_ctx->in_flight(); n < count) {
        throw Error("cannot discard ", count, " replies: there are only ", n, " requests in flight");
      }
      for (int i = 0; i < count; ++i) {
        m_ctx->discard_reply();
      }
    }
    void CommandQueue::discard() {
      discard(m_ctx->in_flight());
    }

    Reply CommandQueue::get_reply() {
      return m_ctx->get_reply();
    }

    CommandQueue::~CommandQueue() {
      discard();
    }
  }
}

#endif
