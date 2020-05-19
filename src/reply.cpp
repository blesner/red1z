#include "red1z/reply.h"
#include "red1z/socket.h"

using red1z::Error;

#include <iostream>

static std::string _read_line(red1z::impl::Socket& sock) {
  std::string line;
  std::array<char, 2> c;
  for (;;) {
    auto buf = sock.peek();
    for (std::size_t n = 0; n < buf.size(); ++n) {
      if (buf[n] == '\r') {
        line.reserve(line.size() + n);
        line.append(buf.data(), n);
        sock.discard(n+2);
        return line;
      }
    }

    if (buf.size()) {
      line.append(buf.data(), buf.size());
      sock.discard(buf.size());
    }
    sock.read(c);
    if (c[0] == '\r') {
      break;
    }
    if (c[1] == '\r') {
      line.push_back(c[0]);
      sock.read(c[1]); //read '\n'
      //c[1] MUST be '\n'
      break;
    }
    line.append(c.data(), 2);
  }
  return line;
}

static std::int64_t _read_integer(red1z::impl::Socket& sock) {
  std::int64_t i = 0;
  auto const data = _read_line(sock);
  auto r = std::from_chars(&data[0], &data[data.size()], i);
  if (r.ec != std::errc()) {
    throw Error("unable to parse ", data, " as an integer");
  }
  return i;
}

static std::string read_simple_string(red1z::impl::Socket& sock) {
  return _read_line(sock);
}

static void read_error(red1z::impl::Socket& sock) {
  auto const msg = _read_line(sock);
  throw Error(msg);
}

static std::int64_t read_integer(red1z::impl::Socket& sock) {
  return _read_integer(sock);
}

static red1z::impl::Rep read_bulk_string(red1z::impl::Socket& sock) {
  auto const size = _read_integer(sock);
  if (size == -1) {
    return std::nullopt;
  }
  else if (size < -1) {
    throw Error("negative bulk string size: ", size);
  }

  std::string data;
  data.resize(size);
  sock.read(data.data(), size);
  char delim[2];
  sock.read(delim, 2);
  if (delim[0] != '\r' or delim[1] != '\n') {
    throw Error("bad delimiter");
  }

  return data;
}

static red1z::impl::Rep read_array(red1z::impl::Socket& sock) {
  auto const size = _read_integer(sock);
  if (size == -1) {
    return std::nullopt;
  }
  else if (size < -1) {
    throw Error("negative array size: ", size);
  }

  std::vector<red1z::Reply> elements;
  elements.reserve(size);
  for (std::int64_t i = 0; i < size; ++i) {
    elements.emplace_back(sock);
  }

  return elements;
}

red1z::impl::Rep red1z::impl::read_reply(red1z::impl::Socket& sock) {
  char type;
  sock.read(type);
  switch(type) {
  case '+':
    return read_simple_string(sock);
  case '-':
    read_error(sock); break;
  case ':':
    return read_integer(sock);
  case '$':
    return read_bulk_string(sock);
  case '*':
    return read_array(sock);
  default:
    throw Error("unexpected response type: ", type);
  }
  return std::nullopt;
}
