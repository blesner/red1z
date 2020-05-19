#include "red1z/socket.h"

#include <netdb.h>
#include <unistd.h>
#include <poll.h>

namespace red1z {
  namespace impl {

    SocketFd::SocketFd() :
      m_fd(socket(AF_INET, SOCK_STREAM, 0))
    {
      if (m_fd == -1) {
        impl::throw_system_error();
      }
    }

    SocketFd::~SocketFd() {
      close(m_fd);
    }

    Socket::Socket(std::string const& host, int port) {
      memset(&m_msg, 0, sizeof(msghdr));

      char buf[1024];
      hostent srv, *psrv = nullptr;
      int errnum;
      if (gethostbyname_r(host.c_str(), &srv, buf, 1024, &psrv, &errnum) != 0 || !psrv) {
        throw_system_error(errnum);
      }

      sockaddr_in addr;
      memset(&addr, 0, sizeof(sockaddr_in));
      addr.sin_family = AF_INET;
      addr.sin_port = htons(port);
      memcpy(&addr.sin_addr.s_addr, srv.h_addr_list[0], srv.h_length);

      if (connect(m_fd, (const sockaddr*)&addr, sizeof(sockaddr_in)) != 0) {
        throw_system_error();
      }
    }

    bool Socket::wait(int timeout) {
      if (m_size - m_pos > 0) {
        //there are unconsumed data in the buffer !
        return true;
      }

      pollfd pfd;
      pfd.fd = m_fd;
      pfd.events = POLLIN;
      pfd.revents = 0;

      int r = poll(&pfd, 1, timeout);
      while (r == -1 and errno == EINTR) {
        r = poll(&pfd, 1, timeout);
      }
      if (r == -1) {
        throw_system_error();
      }
      return pfd.revents & POLLIN;
    }

    void Socket::discard(int n) {
      auto const avail = m_size - m_pos;
      if (n <= avail) {
        m_pos += n;
      } else {
        //read whatever must be discarded into buffer
        for (int remain = n - avail; remain > 0; ) {
          int m = std::min(N, remain);
          do_read(m_buf, m);
          remain -= m;
        }
        m_pos = m_size; //mark buffer as empty
      }
    }

    void Socket::read(char* out, std::int64_t n) {
      auto const avail = m_size - m_pos;
      if (n <= avail) {
        //everything is buffered, just copy
        memcpy(out, m_buf + m_pos, n);
        m_pos += n;
        return;
      }
      if (avail > 0) {
        //partially in the buffer, copy and empty buffer
        memcpy(out, m_buf + m_pos, avail);
        m_size = 0;
        m_pos = 0;
        //adjust request
        out += avail;
        n -= avail;
      }
      if (n > N) {
        //more than a full buffer to read, skip buffering altogether
        std::int64_t total = 0;
        while (total < n) {
          total += do_read(out + total, n - total);
        }
        return;
      }
      //fill up buffer
      m_size = do_read(m_buf, N);
      //copy from buffer
      ::memcpy(out, m_buf, n);
      m_pos = n;
    }

    std::int64_t Socket::do_read(char* out, std::int64_t n, int flags) {
      auto r = recv(m_fd, out, n, flags);
      while (r == -1 && errno == EINTR) {
        r = recv(m_fd, out, n, flags);
      };

      if (r == -1) {
        throw_system_error();
      }
      return r;
    }

    void Socket::write(char const* data, std::int64_t n) {
      auto r = send(m_fd, data, n, 0);
      while (r == -1 and errno == EINTR) {
        r = send(m_fd, data, n, 0);
      }
      if (r == -1) {
        throw_system_error();
      }
    }

    void Socket::write_many(std::vector<std::string> const& data) {
      if (data.empty()) {
        return;
      }
      m_iov_queue.clear();
      m_iov_queue.reserve(data.size());

      std::int64_t n = 0;
      for (auto const& cmd : data) {
        iovec iov;
        iov.iov_base = (void*) cmd.data();
        iov.iov_len = cmd.size();
        n += iov.iov_len;

        m_iov_queue.push_back(iov);
      }
      m_msg.msg_iov = m_iov_queue.data();
      m_msg.msg_iovlen = m_iov_queue.size();
      auto ret = sendmsg(m_fd, &m_msg, 0);
      while (ret == -1 and errno == EINTR) {
        ret = sendmsg(m_fd, &m_msg, 0);
      }

      while (ret >= 0 and ret < n) {
        for (auto& io : m_iov_queue) {
          if (static_cast<std::size_t>(ret) >= io.iov_len) {
            //full block sent : remove from m_msg
            ret -= io.iov_len;
            ++m_msg.msg_iov;
            --m_msg.msg_iovlen;
            n -= io.iov_len;
          }
          else {
            //partialy or not sent block : adjust and keep remaining blocks
            auto const sent = n - ret;
            io.iov_len -= sent;
            io.iov_base = reinterpret_cast<char*>(io.iov_base) + sent;
            break;
          }
        }
        ret = sendmsg(m_fd, &m_msg, 0);
        while (ret == -1 and errno == EINTR) {
          ret = sendmsg(m_fd, &m_msg, 0);
        }
      }

      if (ret == -1) {
        throw_system_error();
      }
    }
  }
}
