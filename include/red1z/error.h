// -*- C++ -*-
#ifndef RED1Z_ERROR_H
#define RED1Z_ERROR_H

#include <sstream>
#include <cstring>

namespace red1z {
  struct Error : std::runtime_error {
    using Base = std::runtime_error;
    using Base::Base;
    template <class... Args>
    Error(Args const&... args) :
      //Base(((std::ostringstream() << ... << args)).str())
      Base(build(args...))
    {
    }
  private:
    template <class... Args>
    static std::string build(Args const&... args) {
      std::ostringstream s;
      (s << ... << args);
      return s.str();
    }
  };

  namespace impl {
    inline Error make_error(int errnum) {
      char buf[256];
      if (strerror_r(errnum, buf, 256) != 0) {
        throw Error("unable to format error ", errnum, " : errno = ", errno);
      }
        return Error(buf);
    }

    inline void throw_system_error() {
      throw make_error(errno);
    }

    inline void throw_system_error(int errnum) {
      throw make_error(errnum);
    }
  }

}

#endif //RED1Z_ERROR_H
