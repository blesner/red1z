#include <iostream>

#include <memory>
#include "red1z/red1z.h"

template <class T>
void print(std::string_view name, std::optional<T> const& o) {
  if (o) {
    std::cout << "GOT " << name << " = " << *o << '\n';
  }
  else {
    std::cout << "NO " << name << '\n';
  }
}

template <class T>
void print(std::string_view name, T const& o) {
  std::cout << "GOT " << name << " = " << o << '\n';
}

template <class L>
void print_list(std::string_view header, L const& l) {
  std::cout << header << ": ";
  for (const auto& i : l) {
    std::cout << i << " ";
  }
  std::cout << '\n';
}

struct OnMsg {
  template <class T>
  void message(std::string channel, T p) const {
    std::cout << "RECV: " << std::string_view(p.data(), p.size()) << " ON: " << channel << '\n';
  }

  template <class T>
  void pmessage(std::string pattern, std::string channel, T p) const {
    std::cout << "RECV: " << std::string_view(p.data(), p.size()) << " ON: " << channel << " (" << pattern << ')' << '\n';
  }

  void info(std::string type, std::string channel, std::int64_t n) {
    std::cout << "INFO: " << type << ", " << channel << ", " << n << '\n';
  }
};

struct MyType {
  int i = 0;
  std::unique_ptr<int> z;
};

namespace red1z {
  // template <>
  // struct io<MyType> : item_io<4> {
  //   static std::string_view view(MyType const& x) {
  //     return io<int>::view(x.i);
  //   }

  //   static MyType read(std::string_view data) {
  //     MyType x;
  //     x.i = io<int>::read(data) + 10;
  //     return x;
  //   }
  // };

  template <>
  struct io<MyType> : item_io<4>, easy_io<MyType, io<MyType>> {
    static void easy_view(IoWriter& w, MyType const& x) {
      w.write(x.i);
    }

    static MyType easy_read(IoReader& r) {
      MyType x;
      r.read(x.i) += 10;
      return x;
    }
  };
}

int main(int, char**) {
  auto ctx = red1z::Redis::from_url("redis://:password@localhost/0");
  namespace f = red1z::flags;
  auto& cmd = red1z::commands;

  std::cout << "SET: " << ctx.set("key1", 123456789) << '\n';
  std::cout << "GET: " << *ctx.get<int>("key1") << '\n';

  {
    std::cout << "=== STATIC TRANSACTION ===\n";
    std::vector<char> plop = {'p', 'l', 'o', 'p'};

    auto [v1, v2, v3] = ctx.transaction(cmd.get<int>("key1"), cmd.set("key2", plop), cmd.get("key2"));
    print("key1", v1);
    print("key2", v2);
    print("key3", v3);
  }

   {
     ctx.set("key3", " 3333 ");
     {
       std::cout << "=== DYNAMIC TRANSACTION ===\n";
       std::string v3;
       auto t = ctx.transaction();
       t
         .get<int>("key1")
         .get("key2")
         [&v3].get("key3");

       // ctx.transaction(); //throws, as exected ;)

       //t.discard();
       auto r = t.execute();

       std::cout << *red1z::opt<int>(r[0]) << ", "
                 << *red1z::opt<std::string>(r[1]) << ", "
                 << (red1z::get<bool>(r[2]) ? v3 : "NO V3 !") << '\n';

       std::cout << "=== TYPED DYNAMIC PIPELINE ===\n";
       auto p = ctx.pipeline<std::optional<std::string>>();
       p.get("key1")
         .get("key2");

       auto r2 = p.execute();

       std::cout << *r2[0] << ", "
                 << *r2[1] << '\n';
     }
     {
       ctx.del("key3");
       std::cout << "=== STATIC PIPELINE ===\n";
       std::string v3;
       auto [v1, v2, has_v3] = ctx.pipeline(cmd.get<int>("key1"), cmd.get("key2"), cmd[&v3].get("key3"));
       std::cout << *v1 << ", " << *v2 << ", " << (has_v3 ? v3 : "NO V3!") << '\n';
     }
   }

  {
    std::cout << "=== MSET ===\n";
    ctx.mset("key1", '$', "key4", "titi");

    std::vector<std::tuple<std::string, std::string>> kv = {{"key1", "$"}, {"key4", "titi"}};
    ctx.mset(red1z::unpack(kv), "key1", 32, red1z::unpack(kv), "", 2);


    std::array<std::string, 3> keys = {"key1", "key3"};

    std::cout << "=== MGET ===\n";
    auto [v1, v2, v3] = ctx.mget("key1", "key3", "key4");
    print("key1", v1);
    print("key3", v2);
    print("key4", v3);

    std::cout << "=== DYNAMIC MGET ===\n";

    for (auto const& v : ctx.mget(red1z::unpack(keys), "key4")) {
      print("key...", v);
    }
  }

  {
    std::cout << "=== ZSETS ===\n";

    ctx.zadd("Z", 1, "one");
    std::vector<std::tuple<double, std::string>> ms = {{42.0, "the answer"}};
    ctx.zadd("Z", red1z::unpack(ms));

    for (auto const& m : ctx.zrange("Z", 0, -1)) {
      std::cout << "  member: " << m << '\n';
    }

    std::vector<std::tuple<std::string, double>> zr;
    ctx[std::back_inserter(zr)].zrange("Z", 0, -1, f::withscores());

    for (auto const& [m, s] : zr) {
      std::cout << "  member: " << std::string_view(m.data(), m.size()) << ", score = " << s << '\n';
    }

    std::cout << "BZPOPMIN: " << std::get<1>(*ctx.bzpopmin("Z", 0)) << '\n';
  }

  {
    std::cout << "=== SCAN ===\n";
    std::cout << " cursor: " << 0 << '\n';
    auto [c, keys] = ctx.scan(0);
    while (c != 0) {
      std::cout << " keys: \n";
      for (auto const& k : keys) {
        std::cout << "  - " << k << '\n';
      }
      std::tie(c, keys) = ctx.scan(c, f::count(3));
      std::cout << " cursor: " << c << '\n';
    }
    for (auto const& k : keys) {
      std::cout << "  - " << k << '\n';
    }
  }

  {
    std::cout << "=== SCAN 2 ===\n";
    std::list<std::string> keys;
    for (auto c = ctx[std::front_inserter(keys)].scan(0); c != 0;
         c = ctx[std::front_inserter(keys)].scan(c, f::count(3)))
      ;
    print_list("KEYS", keys);
  }

  {
    std::cout << "=== SORT ===\n";
    int n;
    ctx[&n].del("list");
    std::cout << "deleted " << n << " keys\n";

    for (int i = 0; i < 10; ++i) {
      //ctx.lpush("list", std::to_string(i), std::to_string(10*i));
      ctx.lpush("list", i, 10*i);
    }

    std::vector<int> items;
    ctx[std::back_inserter(items)].lrange("list", 0, -1);
    print_list("contents", items);

    print_list("sorted", ctx.sort<int>("list", f::order(f::DESC), f::alpha(), f::limit(0, 10)));
    std::cout << "store: "
              << ctx.sort<int>("list", f::order(f::DESC), f::alpha(), f::store("sorted"s)) << '\n';

    print_list("stored", ctx.lrange<int>("sorted", 0, -1));

  }

  ctx.set("tada", 42);
  double four_two = 0;
  ctx[&four_two].get<int>("tada");
  std::cout << "four_two " << four_two << '\n';

  {
    std::tuple<int, short> tup(42, 1);
    ctx.set("tup", tup);
    std::array<short, 3> arr;
    if (ctx[&arr].get("tup")) {
      std::cout << arr[0] << ' ' << arr[1] << ' ' << arr[2] << '\n';
    }

    auto [i, s] = *ctx.get<std::pair<MyType, short>>("tup");
    std::cout << "int: " << i.i << " short: " << s << '\n';
  }

  std::cout << ctx.incrbyfloat("X", 42.123) << '\n';

  auto visitor = OnMsg();
  ctx.subscribe("CHAN");
  ctx.get_message(visitor);
  ctx.psubscribe("CHAN*");
  ctx.get_message(visitor);

  ctx.get_message(visitor);
  ctx.get_message(visitor);
  ctx.unsubscribe("CHAN");
  ctx.get_message(visitor);
  ctx.punsubscribe("CHAN*");
  ctx.get_message(visitor);
  ctx.get_message(visitor, 10000);

}
