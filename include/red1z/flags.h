// -*- C++ -*-
#ifndef RED1Z_FLAGS_H
#define RED1Z_FLAGS_H

#include <chrono>
#include <charconv>

namespace red1z {
  namespace impl {
    template <class Tag, class... Args>
    class flag {
      std::tuple<char const*, Args...> m_flag;
    public:
      flag() {
        std::get<0>(m_flag) = nullptr;
      }

      flag(char const* flag, Args const&... args) : m_flag(flag, args...) {}
      static constexpr int num_args = sizeof...(Args) + 1;
      void encode(Encoder& e) const {
        if (std::get<0>(m_flag)) {
          encode(e, std::make_index_sequence<num_args>());
        }
      }

      int size() const {
        if (std::get<0>(m_flag)) {
          return 1 + sizeof...(Args);
        }
        return 0;
      }
    private:
      template <std::size_t... I>
      void encode(Encoder& e, std::index_sequence<I...>) const {
        int _[] = {e.encode(std::get<I>(m_flag))...};
        (void)_;
      }
    };

    template <class From, class To>  using type_replace = To;
  }

  namespace flags {
    struct _ex_px {};
    struct _nx_xx {};
    struct _ch {};
    struct _keepttl {};
    struct _incr {};
    struct _replace {};
    struct _absttl {};
    struct _idletime {};
    struct _freq {};
    struct _weights {};
    struct _aggregate {};
    struct _withscores {};
    struct _linsert {};
    struct _match {};
    struct _type {};
    struct _count{};

    struct _by {};
    struct _limit {};
    struct _get{};
    struct _order {};
    struct _alpha {};
    struct _store {};

    struct _refcount {};
    struct _encoding {};

    struct _getbf{};
    struct _setbf{};
    struct _incrby{};
    struct _overflow{};

    inline impl::flag<_ex_px, std::string> expire(std::chrono::seconds s) {
      return {"EX", std::to_string(s.count())};
    }

    template <std::intmax_t N, class Rep>
    impl::flag<_ex_px, std::string> expire(std::chrono::duration<Rep, std::ratio<1, N>> sub_s) {
      return {"PX", std::to_string(std::chrono::round<std::chrono::milliseconds>(sub_s).count())};
    }

    template <std::intmax_t N, class Rep>
    impl::flag<_ex_px, std::string> expire(std::chrono::duration<Rep, std::ratio<N>> sup_s) {
      return {"EX", std::to_string(std::chrono::round<std::chrono::seconds>(sup_s).count())};
    }

    inline impl::flag<_ex_px, std::string> ex(std::int64_t s) { return {"EX", std::to_string(s)}; }
    inline impl::flag<_ex_px, std::string> px(std::int64_t ms) { return {"PX", std::to_string(ms)}; }

    enum Exclusive {ALWAYS, IF_NOT_EXISTS, IF_EXISTS, NX, XX};

    inline impl::flag<_nx_xx> cond(Exclusive x) {
      using F = impl::flag<_nx_xx>;
      switch (x) {
      case ALWAYS: return F();
      case IF_NOT_EXISTS :
      case NX:
        return F("NX");
      case IF_EXISTS:
      case XX:
        return F("XX");
      }

      return F();
    }

    inline impl::flag<_nx_xx> nx() { return {"NX"}; }
    inline impl::flag<_nx_xx> xx() { return {"XX"}; }


    template <class Tag>
    impl::flag<Tag> _toggle_flag(char const* f, bool toggle) {
      if (toggle) {
        return {f};
      }
      return {};
    }

    inline auto keepttl(bool toggle=true) {
      return _toggle_flag<_keepttl>("KEEPTTL", toggle);
    }

    //inline impl::flag<_ch> ch() { return {"CH"}; }
    inline auto ch(bool toggle=true) {
      return _toggle_flag<_ch>("CH", toggle);
    }


    inline impl::flag<_incr> incr() { return {"INCR"}; }
    // inline auto incr(bool toggle=true) {
    //   return _toggle_flag<_incr>("INCR", toggle);
    // }

    //inline impl::flag<_replace> replace() { return {"REPLACE"}; }
    inline auto replace(bool toggle=true) {
      return _toggle_flag<_replace>("REPLACE", toggle);
    }

    //inline impl::flag<_absttl> absttl() { return {"ABSTTL"}; }
    inline auto absttl(bool toggle=true) {
      return _toggle_flag<_absttl>("ABSTTL", toggle);
    }

    inline impl::flag<_idletime, std::string>
    idletime(std::int64_t s) { return {"IDLETIME", std::to_string(s)}; }

    inline impl::flag<_freq, std::string> freq(std::int64_t f) { return {"FREQ", std::to_string(f)}; }

    template <class... Ws>
    impl::flag<_weights, impl::type_replace<Ws, std::string>...>
    weights(double w0, Ws const&... ws) {
      return {"WEIGHTS", std::to_string(double(ws))...};
    }

    inline impl::flag<_aggregate, std::string> aggregate_sum() { return {"AGGREGATE", "SUM"}; }
    inline impl::flag<_aggregate, std::string> aggregate_min() { return {"AGGREGATE", "MIN"}; }
    inline impl::flag<_aggregate, std::string> aggregate_max() { return {"AGGREGATE", "MAX"}; }

    enum Aggregate { SUM, MIN, MAX };
    inline impl::flag<_aggregate, std::string> aggregate(Aggregate agg) {
      switch(agg) {
      case SUM:
        return {"AGGREGATE", "SUM"};
      case MIN:
        return {"AGGREGATE", "MIN"};
      case MAX:
        return {"AGGREGATE", "MAX"};
      }
    }

    inline impl::flag<_withscores> withscores() { return {"WITHSCORES"}; }
    // inline auto absttl(bool toggle=true) {
    //   return _toggle_flag<_absttl>("ABSTTL", toggle);
    // }

    enum Linsert { BEFORE, AFTER };
    inline impl::flag<_linsert> linsert(Linsert f) {
      switch (f) {
      case BEFORE:
        return {"BEFORE"};
      case AFTER:
        return {"AFTER"};
      }
    }

    inline impl::flag<_match, std::string>
    match(std::string const& pattern) { return {"MATCH", pattern}; }

    inline impl::flag<_type, std::string>
    type(std::string const& name) { return {"TYPE", name}; }

    inline impl::flag<_count, std::string>
    count(std::int64_t n) { return {"COUNT", std::to_string(n)}; }


    enum Order { ASC, DESC };
    inline impl::flag<_order> order(Order o) {
      switch (o) {
      case ASC:
        return {"ASC"};
      case DESC:
        return {"DESC"};
      }
      __builtin_unreachable();
    }

    inline auto alpha(bool toggle=true) {
      return _toggle_flag<_alpha>("ALPHA", toggle);
    }

    inline impl::flag<_by, std::string>
    by(std::string const& pattern) { return {"BY", pattern}; }

    inline impl::flag<_get, std::string>
    get(std::string const& pattern) { return {"GET", pattern}; }


    enum Sign {SIGNED, UNSIGNED};

    namespace fimpl {
      inline std::string make_type(char t, int size) {
        static constexpr int N = 16;
        char buf[N];
        buf[0] = t;
        auto r = std::to_chars(buf + 1, buf + N, size);
        if (r.ec == std::errc()) {
          throw Error("unable to convert ", size, " to string");
        }
        return {buf, r.ptr};
      }
      inline std::string make_type(Sign s, int size) {
        if (s == SIGNED) {
          return make_type('i', size);
        }
        return make_type('u', size);
      }

      inline std::string make_offset(std::int64_t offset, bool typew) {
        return (typew ? "#" : "") + std::to_string(offset);
      }
    }

    inline impl::flag<_getbf, std::string, std::string>
    get(Sign s, int size, std::int64_t offset, bool typew = false) {
      return {"GET", fimpl::make_type(s, size), fimpl::make_offset(offset, typew)};
    }

    inline impl::flag<_setbf, std::string, std::string>
    set(Sign s, int size, std::int64_t offset, bool typew = false) {
      return {"SET", fimpl::make_type(s, size), fimpl::make_offset(offset, typew)};
    }

    inline impl::flag<_incrby, std::string, std::string, std::string>
    incrby(Sign s, int size, std::int64_t offset, std::int64_t increment, bool typew = false) {
      return {"INCRBY", fimpl::make_type(s, size),
              fimpl::make_offset(offset, typew), std::to_string(increment)};
    }

    enum Overflow { WRAP, SAT, FAIL };
    inline impl::flag<_overflow, std::string>
    overflow(Overflow of) {
      switch (of) {
      case WRAP: return {"OVERFLOW", "WRAP"};
      case SAT:  return {"OVERFLOW", "SAT"};
      case FAIL: return {"OVERFLOW", "FAIL"};
      }
    }

    inline impl::flag<_limit, std::string, std::string>
    limit(std::int64_t offset, std::int64_t count) {
      return {"LIMIT", std::to_string(offset), std::to_string(count)};
    }

    template <class K>
    impl::flag<_store, K> store(K const& dest) { return {"STORE", dest}; }

    template <class... Flags> struct _any_of {};
  }

  namespace impl {
    ///// FLAG TESTING

    template <class Tag, class... Args>
    struct has_flag_helper;

    template <class Tag>
    struct has_flag_helper<Tag> : std::false_type {};

    template <class Tag, class... Flags, class... Args>
    struct has_flag_helper<Tag, flag<Tag, Flags...>, Args...> : std::true_type {};

    template <class Tag, class Arg, class... Args>
    struct has_flag_helper<Tag, Arg, Args...> : has_flag_helper<Tag, Args...> {};

    template <class Tag, class... Args>
    inline constexpr bool has_flag_v = has_flag_helper<Tag, Args...>::value;


    ////// FLAGS RANGE

    template <int I, bool started, int F, int L, class... Args>
    struct flags_range_helper;

    template <int I, bool started, int F, int L, class Arg, class... Args>
    struct flags_range_helper<I, started, F, L, Arg, Args...> :
      flags_range_helper<I+1, started, F, L, Args...>
    {};

    template <int I, bool started, int F, int L>
    struct flags_range_helper<I, started, F, L> {
      static constexpr int first = F;
      static constexpr int last = L;
      static constexpr int size = L - F;
      static constexpr bool empty = L == F;
    };

    template <int I, int F, int L, class Tag, class... Flags, class... Args>
    struct flags_range_helper<I, false, F, L, flag<Tag, Flags...>, Args...>
      : flags_range_helper<I+1, true, I, I+1, Args...>
    {};

    template <int I, int F, int L, class Tag, class... Flags, class... Args>
    struct flags_range_helper<I, true, F, L, flag<Tag, Flags...>, Args...>
      : flags_range_helper<I+1, true, F, I+1, Args...>
    {
      static_assert(L == I, "all flags must be passsed together");
    };

    template <class... Args>
    using flags_range = flags_range_helper<0, false, 0, 0, Args...>;


    //// FLAG CHECKING

    template <class Tag, class Kept, class Allowed>
    struct remove_helper;

    template <class Tag, class Kept>
    struct remove_helper<Tag, Kept, flags::_any_of<> > {
      using type = Kept;
    };

    template <class Tag, int N=10000> struct many {
      static_assert(N > 0);
    };

    // template <class Tag, class... Kept, class... Choices, class... Allowed>
    // struct remove_helper <Tag,
    //                       flags::_any_of<Kept...>,
    //                       flags::_any_of<flags::_one_of<Choices...>, Allowed...> >  {
    //   using type =
    //     std::conditional_t<std::disjunction_v<std::is_same<Tag, Choices>...>,
    //                        flags::_any_of<Kept..., Allowed...>,
    //                        typename remove_helper<Tag,
    //                                               flags::_any_of<Kept..., flags::_one_of<Choices...>>,
    //                                               flags::_any_of<Allowed...>
    //                                               >::type
    //                        >;
    // };

    template <class Tag, class... Kept, class Choice, class... Allowed>
    struct remove_helper <Tag,
                          flags::_any_of<Kept...>,
                          flags::_any_of<Choice, Allowed...> >  {
      using type =
        std::conditional_t<std::is_same_v<Tag, Choice>,
                           flags::_any_of<Kept..., Allowed...>,
                           typename remove_helper<Tag,
                                                  flags::_any_of<Kept..., Choice>,
                                                  flags::_any_of<Allowed...>
                                                  >::type
                           >
        ;
    };
    template <class Tag, class... Kept, class Choice, class... Allowed>
    struct remove_helper <Tag,
                          flags::_any_of<Kept...>,
                          flags::_any_of<many<Choice, 1>, Allowed...> >

      : remove_helper <Tag, flags::_any_of<Kept...>, flags::_any_of<Choice, Allowed...> >
    {};

    template <class Tag, int N, class... Kept, class Choice, class... Allowed>
    struct remove_helper <Tag,
                          flags::_any_of<Kept...>,
                          flags::_any_of<many<Choice, N>, Allowed...> >  {
      using type =
        std::conditional_t<std::is_same_v<Tag, Choice>,
                           flags::_any_of<Kept..., many<Choice, N-1>, Allowed...>,
                           typename remove_helper<Tag,
                                                  flags::_any_of<Kept..., many<Choice, N>>,
                                                  flags::_any_of<Allowed...>
                                                  >::type
                           >
        ;
    };


    template <class Tag, class Allowed>
    using remove = typename remove_helper<Tag, flags::_any_of<>, Allowed>::type;


    template <class Allowed, class... Args>
    struct is_allowed;

    template <class Allowed>
    struct is_allowed<Allowed> : std::true_type {};

    template <class Allowed, class Arg, class... Args>
    struct is_allowed<Allowed, Arg, Args...> : is_allowed<Allowed, Args...> {};

    template <class Allowed, class Tag, class... Flag, class... Args>
    struct is_allowed<Allowed, impl::flag<Tag, Flag...>, Args...> :
      std::conditional_t<std::is_same_v<remove<Tag, Allowed>,  Allowed>,
                         std::false_type,
                         is_allowed<remove<Tag, Allowed>, Args...> >
    {};

    template <class Tag, class... Flag, class... Args>
    struct is_allowed<flags::_any_of<>, impl::flag<Tag, Flag...>, Args... > : std::false_type {};

    template <class Allowed, class... Args>
    static constexpr bool is_allowed_v = is_allowed<Allowed, Args...>::value;

#define CHECK_FLAGS(msg, args, ...)                                     \
    static_assert(impl::is_allowed_v<flags::_any_of<__VA_ARGS__>, args>, msg);


  }
}

#endif
