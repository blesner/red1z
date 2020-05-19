// -*- C++ -*-
#ifndef RED1Z_REPLY_H
#define RED1Z_REPLY_H

#include "red1z/io.h"

#include <memory>
#include <tuple>
#include <vector>
#include <string>
#include <optional>
#include <variant>

namespace red1z {
  class Reply;

  namespace impl {
    class Socket;
    using Rep = std::variant<std::nullopt_t, std::int64_t, std::string, std::vector<Reply>>;
    Rep read_reply(red1z::impl::Socket& sock);
  }

  class Reply {
    impl::Rep m_impl;
  public:
    Reply(impl::Socket& sock) : m_impl(read_reply(sock)) {}

    Reply(Reply const&) = delete;
    Reply(Reply&&) = default;
    Reply& operator=(Reply const&) = delete;
    Reply& operator=(Reply&&) = default;

    explicit operator bool () const {
      return m_impl.index() != 0;
    }

    std::vector<Reply> array() && {
      if (auto p = std::get_if<std::vector<Reply>>(&m_impl)) {
         return std::move(*p);
      }
      throw Error("cannot access array data");
    }

    std::string string() && {
      if (auto p = std::get_if<std::string>(&m_impl)) {
        return std::move(*p);
      }
      throw Error("cannot access string data");
    }

    std::int64_t integer() const {
      if (auto p = std::get_if<std::int64_t>(&m_impl)) {
        return *p;
      }
      throw Error("cannot access integer data");
    }

    double floating_point() const {
      if (auto p = std::get_if<std::string>(&m_impl)) {
        return atof(p->c_str());
      }
      throw Error("cannot access floating point data");
    }

    template <class T>
    T get() && {
      return io<T>::read(std::move(*this).string());
    }

    bool ok() const {
      if (!*this) {
        return false;
      }
      if (auto s = std::get_if<std::string>(&m_impl); s and *s == "OK") {
        return true;
      }
      throw Error("Unexpected reply type");
    }

    std::int64_t array_size() const {
      if (auto p = std::get_if<std::vector<Reply>>(&m_impl)) {
        return p->size();
      }
      return 0;
    }
  };

}

#endif
