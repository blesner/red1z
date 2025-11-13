// -*- C++ -*-
#ifndef RED1Z_IO_H
#define RED1Z_IO_H

#include "error.h"

#include <array>
#include <cstdint>
#include <deque>
#include <forward_list>
#include <list>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include <charconv>
#include <cstring>

namespace red1z {
  namespace impl {

    template <class T>
    constexpr bool is_trivial_v =
        std::is_trivially_copyable_v<T> and std::is_standard_layout_v<T>;

    template <class It, class C>
    constexpr bool iterator_of_v =
        std::is_same_v<typename C::iterator, It> ||
        std::is_same_v<typename C::const_iterator, It>;

    template <class It> struct is_contiguous {
      using T = typename std::iterator_traits<It>::value_type;
      static constexpr bool value =
          iterator_of_v<It, std::vector<T>> ||
          iterator_of_v<It, std::array<T, 1>> ||
          iterator_of_v<It, std::basic_string<T>> ||
          iterator_of_v<It, std::basic_string_view<T>>;
    };
    template <class It>
    constexpr bool is_contiguous_v = is_contiguous<It>::value;

    template <class It>
    constexpr bool is_contiguous_of_trivial_v =
        is_contiguous_v<It> and is_trivial_v<typename It::value_type>;

    template <class It> struct ContiguousRange : std::tuple<It, It> {};

    template <class It> struct Range : std::tuple<It, It> {};
  } // namespace impl

  template <class T> std::string_view make_view(T const *ptr, int count) {
    return {reinterpret_cast<char const *>(ptr), count * sizeof(T)};
  }

  template <class T> std::string_view range(T const *ptr, int count) {
    return make_view(ptr, count);
  }

  template <class It>
  impl::ContiguousRange<It>
  range(It begin, It end,
        std::enable_if_t<impl::is_contiguous_of_trivial_v<It>> * = nullptr) {
    return std::make_tuple(begin, end);
  }

  template <class It>
  impl::Range<It> range(
      It begin, It end,
      std::enable_if_t<not impl::is_contiguous_of_trivial_v<It>> * = nullptr) {
    return std::make_tuple(begin, end);
  }

  namespace impl {
    struct item_io_base {};
    struct trivial_io_base {};
  } // namespace impl

  template <int N> struct item_io : impl::item_io_base {
    static constexpr int item_size = N;
  };

  template <class T>
  struct trivial_io : item_io<sizeof(T)>, impl::trivial_io_base {
    static_assert(impl::is_trivial_v<T>,
                  "your user-defined type T cannot be serialized, please "
                  "specialize red1z::io<T>");
    static std::string_view view(T const &v) {
      return make_view(&v, 1);
    }

    static T read(std::string_view data) {
      if (data.size() != sizeof(T)) {
        throw Error("requested type size mismatch, got: ", data.size(),
                    " expected: ", sizeof(T));
      }
      return *reinterpret_cast<T const *>(data.data());
    }
  };

  template <class T> struct io : trivial_io<T> {};

  template <class T>
  struct has_item_io : std::is_base_of<impl::item_io_base, io<T>> {};
  template <class T> constexpr bool has_item_io_v = has_item_io<T>::value;
  template <class T>
  struct has_trivial_io : std::is_base_of<impl::trivial_io_base, io<T>> {};
  template <class T> constexpr bool has_trivial_io_v = has_trivial_io<T>::value;

  namespace impl {
    template <class T> auto view(T const &value) {
      auto const data = io<T>::view(value);
      if constexpr (has_item_io<T>::value) {
        if (data.size() != io<T>::item_size) {
          throw Error("your declared red1z::item_io serialized size (=",
                      io<T>::item_size,
                      ") does not match the size returned by io<T>::view (=",
                      data.size(), ")");
        }
      }
      return data;
    }

    class Encoder {
      std::string m_cmd;

    public:
      explicit Encoder(int size) {
        m_cmd.reserve(1024);
        m_cmd.push_back('*');
        append(size);
        append_delimiter();
      }

      template <class T> int encode(T const &val) {
        auto const data = impl::view(val);
        m_cmd.push_back('$');
        append(data.size());
        append_delimiter();
        m_cmd.append(data);
        append_delimiter();
        return 0;
      }

      std::string const &value() const {
        return m_cmd;
      }

    private:
      void append(std::int64_t x) {
        m_cmd.append(std::to_string(x));
      }

      void append_delimiter() {
        m_cmd.append("\r\n", 2);
      }
    };
  } // namespace impl

  template <class T> struct io<std::basic_string_view<T>> {
    static_assert(std::is_trivially_copyable_v<T>);
    static std::string_view view(std::basic_string_view<T> v) {
      return make_view(v.data(), v.size());
    }

    // can't read into string_view
  };

  template <std::size_t N> struct io<char[N]> {
    static std::string_view view(char const (&str)[N]) {
      return {str};
    }

    // can't read into char literal
  };

  template <> struct io<char const *> {
    static std::string_view view(char const *str) {
      return {str};
    }

    // can't read into char literal
  };

  template <class It> struct io<impl::ContiguousRange<It>> {
    static std::string_view view(impl::ContiguousRange<It> const &r) {
      auto [b, e] = r;
      return make_view(std::addressof(*b), std::distance(b, e));
    }

    // can't read into range
  };

  template <class It> struct io<impl::Range<It>> {
    static std::string view(impl::Range<It> const &r) {
      std::string out;
      for (auto [first, last] = r; first != last; ++first) {
        auto const data = impl::view(*first);
        out.append(data.data(), data.size());
      }
      return out;
    }

    // can't read into range
  };

  template <class C, class Derived> struct contiguous_container_io {
    using T = typename C::value_type;
    static_assert(has_item_io_v<T>,
                  "your user-defined type T has no item_io, therefore it "
                  "cannot be used in a standard container. Please have "
                  "red1z::io<T> inherit from red1z::item_io<N> where N is the "
                  "byte-size of your serialized object.");

    static auto view(C const &c) {
      return view(c, has_trivial_io<T>());
    }
    static C read(std::string_view v) {
      return read(v, has_trivial_io<T>());
    }

  private:
    template <class C2> struct is_array : std::false_type {};
    template <std::size_t N>
    struct is_array<std::array<T, N>> : std::true_type {};
    static constexpr const int N = io<T>::item_size;
    static_assert((not has_trivial_io_v<T>) or N == sizeof(T));

    static std::string view(C const &c, std::false_type /* trivial_io */) {
      std::string out;
      out.reserve(c.size() * io<T>::item_size);
      for (auto const &item : c) {
        auto const data = impl::view(item);
        out.append(data.data(), data.size());
      }
      return out;
    }

    static std::string_view view(C const &c, std::true_type /* trivial_io */) {
      return make_view(c.data(), c.size());
    }

    static C read(std::string_view v, std::false_type /* trivial_io */) {
      int const nbytes = v.size();
      if (nbytes % N) {
        throw Error("unexpected data size");
      }
      int const n = nbytes / N;

      return Derived::template read_generic<N>(n, v);
    }

    static C read(std::string_view v, std::true_type /* trivial_io */) {
      auto const nbytes = v.size();
      if (nbytes % N) {
        throw Error("unexpected data size");
      }
      auto const n = nbytes / N;
      auto ptr = reinterpret_cast<T const *>(v.data());

      return Derived::read_trivial(ptr, n);
    }
  };

  namespace impl {
    template <class C>
    struct vector_string_io : contiguous_container_io<C, vector_string_io<C>> {
      using T = typename C::value_type;
      template <int N> static C read_generic(int n, std::string_view v) {
        C out;
        out.reserve(n);
        for (int i = 0; i < n; ++i) {
          out.push_back(io<T>::read({&v[i * N], N}));
        }
        return out;
      }

      static C read_trivial(T const *ptr, int n) {
        return {ptr, ptr + n};
      }
    };
  } // namespace impl

  template <class T>
  struct io<std::vector<T>> : impl::vector_string_io<std::vector<T>> {};
  template <class T>
  struct io<std::basic_string<T>>
      : impl::vector_string_io<std::basic_string<T>> {};

  template <> struct io<std::string> {
    static std::string const &read(std::string const &s) {
      return s;
    }

    static std::string read(std::string &&s) {
      return std::move(s);
    }

    // static std::string const& read(std::string_view v) {
    //   return {v.data(), v.size()};
    // }

    static std::string_view view(std::string const &s) {
      return s;
    }
  };

  template <class T, std::size_t N>
  struct io<std::array<T, N>>
      : contiguous_container_io<std::array<T, N>, io<std::array<T, N>>>,
        item_io<N * io<T>::item_size> {
    template <int N2>
    static std::array<T, N> read_generic(int n, std::string_view v) {
      std::array<T, N> out;
      for (int i = 0; i < n; ++i) {
        out[i] = io<T>::read({&v[i * N2], N2});
      }
      return out;
    }

    static std::array<T, N> read_trivial(T const *ptr, int n) {
      std::array<T, N> out;
      if ((int)out.size() != n) {
        throw Error("invalid number of elements in output array");
      }
      std::copy(ptr, ptr + n, out.begin());
      return out;
    }
  };

  template <class C, class Derived> struct container_io {
    using T = typename C::value_type;
    static_assert(has_item_io_v<T>,
                  "your user-defined type T has no item_io, therefore it "
                  "cannot be used in a standard container. Please have "
                  "red1z::io<T> inherit from red1z::item_io<N> where N is the "
                  "byte-size of your serialized object.");

    static C read(std::string_view v) {
      return read(v, has_trivial_io<T>());
    }

    static std::string view(C const &c) {
      std::string out;
      out.reserve(io<T>::item_size() * c.size());
      for (auto const &item : c) {
        auto const data = impl::view(item);
        out.append(data.data(), data.size());
      }
      return out;
    }

  private:
    static constexpr const int N = io<T>::item_size;
    static_assert((not has_trivial_io_v<T>) or N == sizeof(T));

    static C read(std::string_view v, std::false_type /* trivial_io */) {
      auto const n = item_count(v);
      C out;
      for (int i = 0; i < n; ++i) {
        Derived::append(out, io<T>::read({&v[i * N], N}));
      }
      return out;
    }

    static C read(std::string_view v, std::true_type /* trivial_io */) {
      auto const n = item_count(v);
      auto ptr = reinterpret_cast<T const *>(v.data());
      return {ptr, ptr + n};
    }

    static int item_count(std::string_view v) {
      auto const nbytes = v.size();
      if (nbytes % N) {
        throw Error("unexpected data size");
      }
      return nbytes / N;
    }
  };

  template <class T>
  struct io<std::list<T>> : container_io<std::list<T>, io<std::list<T>>> {
    static void append(std::list<T> &out, T &&value) {
      out.push_back(std::move(value));
    }
  };

  template <class T>
  struct io<std::forward_list<T>>
      : container_io<std::forward_list<T>, io<std::forward_list<T>>> {
    static void append(std::forward_list<T> &out, T &&value) {
      out.push_back(std::move(value));
    }
  };

  template <class T>
  struct io<std::deque<T>> : container_io<std::deque<T>, io<std::deque<T>>> {
    static void append(std::list<T> &out, T &&value) {
      out.push_back(std::move(value));
    }
  };

  template <class T>
  struct io<std::set<T>> : container_io<std::set<T>, io<std::set<T>>> {
    static void append(std::list<T> &out, T &&value) {
      out.insert(std::move(value));
    }
  };

  template <class T>
  struct io<std::unordered_set<T>>
      : container_io<std::unordered_set<T>, io<std::unordered_set<T>>> {
    static void append(std::list<T> &out, T &&value) {
      out.insert(std::move(value));
    }
  };

  template <class T> struct io<std::tuple<T>> {
    static auto view(std::tuple<T> const &t) {
      return io<T>::view(std::get<0>(t));
    }

    static std::tuple<T> read(std::string_view data) {
      return {io<T>::read(data)};
    }
  };

  template <class... Ts> struct io<std::tuple<Ts...>> {
    static_assert((... && has_item_io_v<Ts>));
    using Tuple = std::tuple<Ts...>;

    static std::string view(Tuple const &t) {
      return view(t, std::make_index_sequence<N>());
    }

    static Tuple read(std::string_view data) {
      return read(data, std::make_index_sequence<N>());
    }

  private:
    template <std::size_t... I>
    static std::string view(Tuple const &t, std::index_sequence<I...>) {
      std::string out;
      out.reserve((... + io<Ts>::item_size));
      int _[] = {view<I>(out, t)...};
      (void)_;
      return out;
    }

    template <int I> static int view(std::string &out, Tuple const &t) {
      auto data = impl::view(std::get<I>(t));
      out.append(data.data(), data.size());
      return 0;
    }

    static constexpr int N = sizeof...(Ts);
    static constexpr std::array<int, N> sizes = {io<Ts>::item_size...};

    template <std::size_t... I>
    static constexpr int make_offset(std::index_sequence<I...>) {
      return (0 + ... + sizes[I]);
    }

    template <int I>
    static constexpr int offset = make_offset(std::make_index_sequence<I>());

    template <std::size_t... I>
    static Tuple read(std::string_view data, std::index_sequence<I...>) {
      return {
          io<Ts>::read(std::string_view(data.data() + offset<I>, sizes[I]))...};
    }
  };

  template <class T1, class T2> struct io<std::pair<T1, T2>> {
    static_assert(has_item_io_v<T1> and has_item_io_v<T2>);
    using Pair = std::pair<T1, T2>;
    static std::string view(Pair const &p) {
      auto const d1 = io<T1>::view(p.first);
      auto const d2 = io<T2>::view(p.second);
      std::string out;
      out.reserve(d1.size() + d2.size());
      out.append(d1.data(), d1.size());
      out.append(d2.data(), d2.size());
      return out;
    }

    static Pair read(std::string_view data) {
      auto const ptr = data.data();
      static constexpr int N1 = io<T1>::item_size;
      static constexpr int N2 = io<T2>::item_size;
      if (N1 + N2 != data.size()) {
        throw Error("unexpected data size for reading");
      }
      return {io<T1>::read({ptr, N1}), io<T2>::read({ptr + N1, N2})};
    }
  };

  // template <class T = std::string> class StreamEntry {
  //   std::string m_id;
  //   std::vector<std::tuple<std::string, T>> m_fields;
  // };

  // template <class T> struct io<StreamEntry<T>> {
  //   static StreamEntry<T> read(std::string_view data) {

  //   }
  // };

  class IoWriter {
    std::string &m_data;

  public:
    IoWriter(std::string &data) : m_data(data){};

    template <class T> void write(T const &x) {
      auto d = impl::view(x);
      m_data.append(d.data(), d.size());
    }

    template <class T> void write(T const *ptr, int n) {
      auto d = impl::view(range(ptr, n));
      m_data.append(d.data(), d.size());
    }
  };

  class IoReader {
    char const *m_ptr;
    int m_bytes;

  public:
    IoReader(std::string_view const &data)
        : m_ptr(data.data()), m_bytes(data.size()) {}

    template <class T>
    T &read(T &out, std::enable_if_t<has_item_io_v<T>> * = nullptr) {
      static constexpr int N = io<T>::item_size;
      if (N > m_bytes) {
        throw Error("requested read too large");
      }
      out = io<T>::read({m_ptr, N});
      m_ptr += N;
      m_bytes -= N;
      return out;
    }

    template <class T>
    T &read(T &out, std::enable_if_t<not has_item_io_v<T>> * = nullptr) {
      return out = io<T>::easy_read(*this);
    }

    template <class T> T read() {
      T out;
      read(out);
      return out;
    }

    template <class T>
    void read(T *ptr, int n,
              std::enable_if_t<not impl::is_trivial_v<T>> * = nullptr) {
      for (int i = 0; i < n; ++i) {
        read(*ptr++);
      }
    }

    template <class T>
    void read(T *ptr, int n,
              std::enable_if_t<impl::is_trivial_v<T>> * = nullptr) {
      auto const size = n * sizeof(T);
      if (size > m_bytes) {
        throw Error("requested read too large");
      }
      std::copy(m_ptr, m_ptr + size, reinterpret_cast<char *>(ptr));
      m_ptr += size;
      m_bytes -= size;
    }
  };

  template <class T, class Impl> struct easy_io {
    static std::string view(T const &x) {
      std::string data;
      IoWriter w(data);
      Impl::easy_view(w, x);
      return data;
    }

    static T read(std::string_view data) {
      IoReader r(data);
      return Impl::easy_read(r);
    }
  };

} // namespace red1z

#endif
