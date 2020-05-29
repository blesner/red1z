#include "red1z/red1z.h"

#include <regex>
#include <charconv>

namespace red1z {
  impl::Passtrough commands;

  Redis::Redis(std::string const& hostname, int port, int db,
               std::optional<std::string> pass, std::optional<std::string> user)
    : m_ctx(hostname, port)
  {
    (void) user; //silence warning
    /*
      ignore the username (for REDIS 5)
    if (user) {
      if (!pass) {
        throw Error("username without password");
      }
      m_ctx.run("AUTH", *user, *pass);
    }
    */

    if (pass) {
      m_ctx.run("AUTH", *pass);
    }

    if (db > 0) {
      m_ctx.run("SELECT", std::to_string(db));
    }
  }

  Redis Redis::from_url(std::string_view url) {
    std::regex re("redis://(([^:/ ]+)?:([^@/ ]+)@)?([^@/ :]+)(:([0-9]+))?(/([0-9]{0,2}))?");
    std::cmatch m;
    if (!regex_match(std::begin(url), std::end(url), m, re)) {
      throw Error("unable to parse redis URL ", url);
    }

    std::optional<std::string> username;
    std::optional<std::string> password;
    if (m[1].length()) {
      if (m[2].length()) {
        username = m[2].str();
      }
      password = m[3].str();
    }

    std::string hostname = m[4].str();
    int port = 6379;
    if (m[5].length()) {
      std::from_chars(m[6].first, m[6].second, port);
    }

    int db = 0;
    if (m[8].length() ) {
      std::from_chars(m[8].first, m[8].second, db);
      if (db >= 16) {
        throw Error("invalid DB index ", db);
      }
    }

    return {hostname, port, db, password, username};
  }
}
