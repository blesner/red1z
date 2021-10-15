#include <iostream>

#include "red1z/red1z.h"
#include <memory>

template <class T> void print_xread(T const &e) {
  std::cout << "== BEGIN XREAD\n\n";
  if (e) {
    for (auto const &ee : *e) {
      std::cout << "----------------\n";
      std::cout << std::get<0>(ee) << '\n';
      for (auto &&entry : std::get<1>(ee)) {
        std::cout << "  " << std::get<0>(entry) << '\n';
        for (auto &&f : std::get<1>(entry)) {
          std::cout << "    " << f.first << " = " << f.second << '\n';
        }
      }
    }
  } else {
    std::cout << "timeout\n";
  }

  std::cout << "== END XREAD\n\n";
}

int main(int, char **) {
  auto ctx = red1z::Redis::from_url("redis://:password@localhost/0");
  namespace f = red1z::flags;

  auto res = ctx.xadd("stream1", f::maxlen<'='>(1), "*", "valueSSS", 49);
  std::cout << res << '\n';

  std::vector<std::tuple<std::string, int>> data = {{"plop", 64},
                                                    {"answer", 42}};
  std::cout << ctx.xadd("stream", f::maxlen<'='>(1), "*", red1z::unpack(data))
            << '\n';

  std::vector<std::string> ids = {"0", "0"};

  try {
    ctx.xgroup_create("stream", "group55", "0-0");
  } catch (...) {
  }

  {
    auto e = ctx.xreadgroup<std::unordered_map<std::string, int>>(
        "group55", "consumer-123", f::count(20), "stream", ">");
    std::cout << "XREAD GROUP\n";
    print_xread(e);
  }

  {
    auto [n, start, end, info] = ctx.xpending("stream", "group55");
    std::cout << "XPENDING: " << n << ' ' << start << ' ' << end << ": \n";
    for (auto const &[name, pending] : info) {
      std::cout << "  * " << name << " -> " << pending << '\n';
    }
  }

  {
    auto info = ctx.xpending("stream", "group55", "-", "+", 10);
    std::cout << "XPENDING: \n";
    for (auto const &[id, name, elapsed, deliveries] : info) {
      std::cout << "  * " << id << " " << name << " -> " << elapsed << " = "
                << deliveries << '\n';
    }
  }

  auto e = ctx.xread<std::unordered_map<std::string, int>>(
      f::count(20), "stream", "stream1", red1z::unpack(ids));
  std::cout << "\n\nXREAD\n";
  print_xread(e);
}
