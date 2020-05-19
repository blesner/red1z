// -*- C++ -*-
#ifndef RED1Z_SOCKET_H
#define RED1Z_SOCKET_H

#include "red1z/error.h"

#include <vector>

#include <sys/socket.h>


namespace red1z {
  namespace impl {
    class SocketFd {
      int m_fd;
    public:
      SocketFd();
      ~SocketFd();

      operator int () const { return m_fd; }
    };

    class Socket {
      SocketFd m_fd;
      static constexpr int N = 256;
      char m_buf[N];
      int m_size = 0;
      int m_pos = 0;
      msghdr m_msg;
      std::vector<iovec> m_iov_queue;
    public:
      Socket(std::string const& host, int port);

      bool wait(int timeout=-1);

      template <class T>
      void read(T& out) {
        read(reinterpret_cast<char*>(&out), sizeof(T));
      }
      void read(char* out, std::int64_t n);

      std::string_view peek() {
        return std::string_view(m_buf + m_pos, m_size - m_pos);
      }

      void discard(int n);

      void write(char const* data, std::int64_t n);
      void write_many(std::vector<std::string> const& data);

      operator int () const { return int(m_fd); }
    private:
      std::int64_t do_read(char* out, std::int64_t n, int flags=0);
    };
  }
}

#endif
