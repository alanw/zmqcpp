//
// Very simple, very thin cpp wrapper for zmq.
// ... magic not included.
//
// Copyright (c) 2012 Alan Wright. All rights reserved.
// Distributable under the terms of either the Apache License (Version 2.0)
//
//
// To understand the code, it's best if you read bottom -> up. The design is
// influenced by the zmq python bindings.
//
// To connect to another zmq socket and send a message:
//
//   #include "zmqcpp.h"
//
//   using namespace zmqcpp;
//
//   Context ctx;
//   Socket socket(ctx, xreq);
//   socket.setsockopt(linger, 0);
//   socket.connect("tcp://127.0.0.1:4050");
//
//   Message msg;
//   msg << "first part" << 123 << "third part";
//
//   if (!socket.send(msg))
//     std::cout << "Send error:" << socket.last_error() << "\n";
//
// To bind a zmq socket and receive a message:
//
//   #include "zmqcpp.h"
//
//   using namespace zmqcpp;
//
//   Context ctx;
//   Socket socket(ctx, xrep);
//   socket.bind("tpc://*:4050");
//
//   Message msg;
//   if (!socket.recv(msg)) {
//     std::cout << "Recv error:" << socket.last_error() << "\n";
//   }
//   else {
//     std::string first;
//     int32_t second;
//     std::string third;
//
//     msg >> first >> second >> third;
//
//     ...
//
//   }
//
// To poll a zmq socket:
//
//   #include "zmqcpp.h"
//
//   using namespace zmqcpp;
//
//   Context ctx;
//   Socket socket(ctx, xrep);
//   socket.bind("tpc://*:4050");
//
//   Poller poller;
//   poller.add(socket, pollin);
//
//   Message msg;
//
//   while (true) {
//     poller.poll(1000); // 1 second timeout
//     if (poller.has_polled(socket)) {
//       socket.recv(msg);
//       // do something with msg here
//     }
//   }
//
// To receive a callback when a zmq socket is polled:
//
//   #include "zmqcpp.h"
//
//   using namespace zmqcpp;
//
//   void socket_callback(const Socket& socket) {
//     Message msg;
//     socket.recv(msg);
//     // doc something with msg here
//   }
//
//   ...
//
//   Context ctx;
//   Socket socket(ctx, xrep);
//   socket.bind("tpc://*:4050");
//
//   Poller poller;
//   poller.add(socket, pollin, socket_callback);
//
//   while (true) {
//     poller.poll(1000); // 1 second timeout
//     // callback function called here if polled
//   }
//
// For more examples, including copying messages and constructing messages from
// custom data types, then check out the unit tests.
//

#ifndef _ZMQCPP_H_
#define _ZMQCPP_H_

#include <iostream>
#include <sstream>
#include <cstring>
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <algorithm>
#include <stdexcept>

#ifndef _MSC_VER
#include <stdint.h>
#else
typedef __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;

#pragma warning( push )
#pragma warning( disable: 4996 4800 )
#endif

namespace zmqcpp {

enum SocketType {
  pair = ZMQ_PAIR,
  pub = ZMQ_PUB,
  sub = ZMQ_SUB,
  pull = ZMQ_PULL,
  push = ZMQ_PUSH,
  req = ZMQ_REQ,
  rep = ZMQ_REP,
  xpub = ZMQ_XPUB,
  xsub = ZMQ_XSUB,
  router = ZMQ_ROUTER,
  dealer = ZMQ_DEALER,
  stream = ZMQ_STREAM
};

enum SocketOption {
  #if (ZMQ_VERSION_MAJOR >= 3)
  maxmsgsize = ZMQ_MAXMSGSIZE,
  sndhwm = ZMQ_SNDHWM,
  rcvhwm = ZMQ_RCVHWM,
  multicast_hops = ZMQ_MULTICAST_HOPS,
  rcvtimeo = ZMQ_RCVTIMEO,
  sndtimeo = ZMQ_SNDTIMEO,
  ipv6 = ZMQ_IPV6,
  #else
  hwm = ZMQ_HWM,
  swap = ZMQ_SWAP,
  mcast_loop = ZMQ_MCAST_LOOP,
  recovery_ivl_msec = ZMQ_RECOVERY_IVL_MSEC,
  #endif
  affinity = ZMQ_AFFINITY,
  identity = ZMQ_IDENTITY,
  subscribe = ZMQ_SUBSCRIBE,
  unsubscribe = ZMQ_UNSUBSCRIBE,
  rate = ZMQ_RATE,
  sndbuf = ZMQ_SNDBUF,
  rcvbuf = ZMQ_RCVBUF,
  rcvmore = ZMQ_RCVMORE,
  fd = ZMQ_FD,
  events = ZMQ_EVENTS,
  type = ZMQ_TYPE,
  linger = ZMQ_LINGER,
  backlog = ZMQ_BACKLOG,
  recovery_ivl = ZMQ_RECOVERY_IVL,
  reconnect_ivl = ZMQ_RECONNECT_IVL,
  reconnect_ivl_max = ZMQ_RECONNECT_IVL_MAX,
  immediate = ZMQ_IMMEDIATE,
  probe_router = ZMQ_PROBE_ROUTER,
  req_correlate = ZMQ_REQ_CORRELATE,
  req_relaxed = ZMQ_REQ_RELAXED,
  conflate = ZMQ_CONFLATE,
  last_endpoint = ZMQ_LAST_ENDPOINT,
};

enum PollOption {
  pollin = ZMQ_POLLIN,
  pollout = ZMQ_POLLOUT,
  pollerror = ZMQ_POLLERR
};


class Message {
public:
  typedef std::deque<zmq_msg_t> Parts;
  typedef Parts::iterator iterator;
  typedef Parts::const_iterator const_iterator;

  struct MessageBuffer
  {
    MessageBuffer(void* data = 0, uint32_t size = 0) : data(data), size(size) {}
    bool empty() const {
      return size == 0;
    }
    void* data;
    uint32_t size;
  };

  Message() : current(parts.begin()) {
    // default
  }

  Message(const zmq_msg_t& msg) {
    push(msg);
  }

  Message(const void* data, uint32_t size) {
    push(data, size);
  }

  template <typename Iter>
  Message(Iter first, Iter last) {
    assign(first, last);
  }

  Message(const Message& msg) {
    assign(msg);
  }

  ~Message() {
    clear();
  }

  void push(const zmq_msg_t& msg, bool copy = false) {
    if (!copy) {
      parts.push_back(msg);
    }
    else {
      zmq_msg_t copy_msg;
      zmq_msg_init(&copy_msg);
      zmq_msg_copy(&copy_msg, const_cast<zmq_msg_t*>(&msg));
      parts.push_back(copy_msg);
    }
    current = parts.begin();
  }

  void push(const void* data, size_t size) {
    zmq_msg_t msg;
    zmq_msg_init_size(&msg, size);
    if (data) {
      std::memcpy(message_buffer(msg).data, data, size);
    }
    else {
      std::memset(message_buffer(msg).data, 0, size);
    }
    push(msg);
  }

  const zmq_msg_t& pop() {
    if (current == parts.end()) {
      throw std::runtime_error("message: no more parts.");
    }
    return *current++;
  }

  MessageBuffer pop_buffer() {
    return message_buffer(pop());
  }

  uint32_t message_bytes() const {
    uint32_t total_size = 0;
    for (const_iterator part = parts.begin(), last = parts.end(); part != last; ++part) {
      total_size += message_buffer(*part).size;
    }
    return total_size;
  }

  void clear() {
    for (const_iterator part = parts.begin(), last = parts.end(); part != last; ++part) {
      zmq_msg_close(const_cast<zmq_msg_t*>(&(*part)));
    }
    parts.clear();
  }

  bool empty() const {
    return parts.empty();
  }

  uint32_t size() const {
    return parts.size();
  }

  const_iterator begin() const {
    return parts.begin();
  }

  const_iterator end() const {
    return parts.end();
  }

  iterator begin() {
    return parts.begin();
  }

  iterator end() {
    return parts.end();
  }

  const_iterator get_current() const {
      return current;
  }

  void reset_current() {
      current = parts.begin();
  }

  const zmq_msg_t& at(uint32_t i) const {
    return parts.at(i);
  }

  const zmq_msg_t& front() const {
    return parts.front();
  }

  const zmq_msg_t& back() const {
    return parts.back();
  }

  void assign(const Message& msg, bool copy = true) {
    clear();
    if (msg.size() > 0) {
        assign(msg.begin(), msg.end());
    }
  }

  template <typename Iter>
  void assign(Iter first, Iter last, bool copy = true) {
    clear();
    for (; first != last; ++first) {
      push(*first, copy);
    }
  }

  void assign(const zmq_msg_t& msg, bool copy = true) {
    clear();
    push(msg, copy);
  }

  template <typename Iter>
  iterator insert(iterator position, Iter first, Iter last, bool copy = true) {
    for (; first != last; ++first) {
      position = insert(position, *first, copy) + 1;
    }
    return position;
  }

  iterator insert(iterator position, const zmq_msg_t& msg, bool copy = true) {
    if (!copy) {
      position = parts.insert(position, msg);
    }
    else {
      zmq_msg_t copy_msg;
      zmq_msg_init(&copy_msg);
      zmq_msg_copy(&copy_msg, const_cast<zmq_msg_t*>(&msg));
      position = parts.insert(position, copy_msg);
    }
    current = parts.begin();
    return position;
  }

  Message& operator = (const Message& msg) {
    assign(msg.begin(), msg.end());
    return *this;
  }

  Message& operator << (uint8_t value) {
    zmq_msg_t msg;
    zmq_msg_init_size(&msg, sizeof(uint8_t));
    *reinterpret_cast<uint8_t*>(message_buffer(msg).data) = value;
    push(msg);
    return *this;
  }

  Message& operator << (int8_t value) {
    return *this << static_cast<uint8_t>(value);
  }

  Message& operator << (bool value) {
    return *this << static_cast<uint8_t>(value ? 1 : 0);
  }

  Message& operator << (uint16_t value) {
    zmq_msg_t msg;
    zmq_msg_init_size(&msg, sizeof(uint16_t));
    *reinterpret_cast<uint16_t*>(message_buffer(msg).data) = value;
    push(msg);
    return *this;
  }

  Message& operator << (int16_t value) {
    return *this << static_cast<uint16_t>(value);
  }

  Message& operator << (uint32_t value) {
    zmq_msg_t msg;
    zmq_msg_init_size(&msg, sizeof(uint32_t));
    *reinterpret_cast<uint32_t*>(message_buffer(msg).data) = value;
    push(msg);
    return *this;
  }

  Message& operator << (int32_t value) {
    return *this << static_cast<uint32_t>(value);
  }

  Message& operator << (uint64_t value) {
    zmq_msg_t msg;
    zmq_msg_init_size(&msg, sizeof(uint64_t));
    *reinterpret_cast<uint64_t*>(message_buffer(msg).data) = value;
    push(msg);
    return *this;
  }

  Message& operator << (int64_t value) {
    return *this << static_cast<uint64_t>(value);
  }

  Message& operator << (const std::string& value) {
    zmq_msg_t msg;
    zmq_msg_init_size(&msg, value.length());
    std::copy(value.begin(), value.end(), reinterpret_cast<char*>(message_buffer(msg).data));
    push(msg);
    return *this;
  }

  template <typename Type>
  Message& operator << (const Type& value) {
    std::ostringstream stream;
    stream << value;
    return *this << stream.str();
  }

  Message& operator >> (uint8_t& value) {
    MessageBuffer buffer(pop_buffer());
    if (buffer.size != sizeof(uint8_t)) {
      throw std::runtime_error("message: type mismatch.");
    }
    value = *reinterpret_cast<uint8_t*>(buffer.data);
    return *this;
  }

  Message& operator >> (int8_t& value) {
    return *this >> reinterpret_cast<uint8_t&>(value);
  }

  Message& operator >> (bool& value) {
    uint8_t msg_value;
    *this >> reinterpret_cast<uint8_t&>(msg_value);
    value = msg_value != 0;
    return *this;
  }

  Message& operator >> (uint16_t& value) {
    MessageBuffer buffer(pop_buffer());
    if (buffer.size != sizeof(uint16_t)) {
      throw std::runtime_error("message: type mismatch.");
    }
    value = *reinterpret_cast<uint16_t*>(buffer.data);
    return *this;
  }

  Message& operator >> (int16_t& value) {
    return *this >> reinterpret_cast<uint16_t&>(value);
  }

  Message& operator >> (uint32_t& value) {
    MessageBuffer buffer(pop_buffer());
    if (buffer.size != sizeof(uint32_t)) {
      throw std::runtime_error("message: type mismatch.");
    }
    value = *reinterpret_cast<uint32_t*>(buffer.data);
    return *this;
  }

  Message& operator >> (int32_t& value) {
    return *this >> reinterpret_cast<uint32_t&>(value);
  }

  Message& operator >> (uint64_t& value) {
    MessageBuffer buffer(pop_buffer());
    if (buffer.size != sizeof(uint64_t)) {
      throw std::runtime_error("message: type mismatch.");
    }
    value = *reinterpret_cast<uint64_t*>(buffer.data);
    return *this;
  }

  Message& operator >> (int64_t& value) {
    return *this >> reinterpret_cast<uint64_t&>(value);
  }

  Message& operator >> (std::string& value) {
    MessageBuffer buffer(pop_buffer());
    value.assign(reinterpret_cast<char*>(buffer.data), buffer.size);
    return *this;
  }

  template <typename Type>
  Message& operator >> (Type& value) {
    MessageBuffer buffer(pop_buffer());
    std::istringstream stream(std::string(reinterpret_cast<char*>(buffer.data), buffer.size));
    stream >> value;
    return *this;
  }

  static std::string message_string(const zmq_msg_t& msg) {
    MessageBuffer buffer(message_buffer(msg));
    return std::string(reinterpret_cast<char*>(buffer.data), buffer.size);
  }

  static MessageBuffer message_buffer(const zmq_msg_t& msg) {
    return MessageBuffer(zmq_msg_data(const_cast<zmq_msg_t*>(&msg)), zmq_msg_size(const_cast<zmq_msg_t*>(&msg)));
  }

  static bool message_empty(const zmq_msg_t& msg) {
    return zmq_msg_size(const_cast<zmq_msg_t*>(&msg)) == 0;
  }

private:
  Parts parts;
  const_iterator current;
};


class Context {
public:
  Context(uint32_t threads = 1, bool auto_term = true) {
    init(threads, auto_term);
  }

  ~Context() {
    if (auto_term) {
      term();
    }
  }

  void init(uint32_t threads = 1, bool auto_term = true) {
    int32_t major, minor, patch;
    zmq_version(&major, &minor, &patch);
    if (major != ZMQ_VERSION_MAJOR) {
      throw std::runtime_error("zmq: library mismatch.");
    }
    this->auto_term = auto_term;
    context = zmq_init(threads);
  }

  void term() {
    zmq_term(context);
    context = 0;
  }

private:
  Context& operator = (const Context&); // non copyable

  void* context;
  bool auto_term;

  friend class Socket;
};


class Socket {
private:
  bool send(const zmq_msg_t& msg, bool block = true, bool more = false) const {
    uint32_t flags = 0;
    if (!block) {
      flags |= DONTWAIT;
    }
    if (more) {
      flags |= SNDMORE;
    }
    #if (ZMQ_VERSION_MAJOR >= 3)
    return zmq_sendmsg(socket, const_cast<zmq_msg_t*>(&msg), flags) >= 0;
    #else
    return zmq_send(socket, const_cast<zmq_msg_t*>(&msg), flags) >= 0;
    #endif
  }

  bool recv(const zmq_msg_t& msg, bool block = true) const {
    #if (ZMQ_VERSION_MAJOR >= 3)
    return zmq_recvmsg(socket, const_cast<zmq_msg_t*>(&msg), block ? 0 : DONTWAIT) >= 0;
    #else
    return zmq_recv(socket, const_cast<zmq_msg_t*>(&msg), block ? 0 : DONTWAIT) >= 0;
    #endif
  }

  bool option_int32(SocketOption option) const {
    #if (ZMQ_VERSION_MAJOR >= 3)
    return option == sndhwm || option == rcvhwm || option == rate ||
           option == recovery_ivl || option == sndbuf || option == rcvbuf ||
           option == linger || option == reconnect_ivl ||
           option == reconnect_ivl_max || option == backlog ||
           option == multicast_hops || option == rcvtimeo ||
           option == sndtimeo || option == ipv6 || option == type ||
           option == rcvmore || option == fd || option == events ||
           option == immediate || option == probe_router ||
           option == req_correlate || option == req_relaxed ||
           option == conflate;
    #else
    return option == linger || option == reconnect_ivl ||
           option == reconnect_ivl_max || option == backlog ||
           option == type || option == fd || option == events;
    #endif
  }

  bool option_int64(SocketOption option) const {
    #if (ZMQ_VERSION_MAJOR >= 3)
    return option == affinity || option == maxmsgsize;
    #else
    return option == rcvmore || option == hwm || option == swap ||
           option == affinity || option == rate || option == recovery_ivl ||
           option == recovery_ivl_msec || option == mcast_loop ||
           option == sndbuf || option == rcvbuf;
    #endif
  }

  bool option_string(SocketOption option) const {
    return option == subscribe || option == unsubscribe || option == identity || option == last_endpoint;
  }

public:
  explicit Socket(void* socket = 0) : socket(socket), own_socket(false) {
  }

  Socket(const Context& ctx, SocketType type) {
    open(ctx, type);
  }

  ~Socket() {
    close();
  }

  bool open(const Context& ctx, SocketType type) {
    own_socket = true;
    socket = zmq_socket(ctx.context, type);
    return socket != 0;
  }

  void close() {
    if (own_socket && socket != 0) {
      zmq_close(socket);
      socket = 0;
    }
  }

  int last_error() const {
    return zmq_errno();
  }

  std::string last_error_str() const {
    return zmq_strerror(last_error());
  }

  bool setsockopt(SocketOption option, const void* value, size_t value_size) const {
    return zmq_setsockopt(socket, static_cast<int32_t>(option), value, value_size) == 0;
  }

  bool setsockopt(SocketOption option, int32_t value) const {
    if (!option_int32(option)) {
      throw std::runtime_error("socket option: invalid option for data type.");
    }
    return zmq_setsockopt(socket, static_cast<int32_t>(option), &value, sizeof(int32_t)) == 0;
  }

  bool setsockopt(SocketOption option, int64_t value) const {
    if (!option_int64(option)) {
      throw std::runtime_error("socket option: invalid option for data type.");
    }
    return zmq_setsockopt(socket, static_cast<int32_t>(option), &value, sizeof(int64_t)) == 0;
  }

  bool setsockopt(SocketOption option, const std::string& value) const {
    if (!option_string(option)) {
      throw std::runtime_error("socket option: invalid option for data type.");
    }
    return zmq_setsockopt(socket, static_cast<int>(option), value.c_str(), value.length()) == 0;
  }

  bool getsockopt(SocketOption option, void* value, size_t& value_size) const {
    return zmq_getsockopt(socket, static_cast<int32_t>(option), value, &value_size) == 0;
  }

  bool getsockopt(SocketOption option, int32_t& value) const {
    if (!option_int32(option)) {
      throw std::runtime_error("socket option: invalid option for data type.");
    }
    size_t value_size = sizeof(int32_t);
    return zmq_getsockopt(socket, static_cast<int32_t>(option), &value, &value_size) == 0;
  }

  bool getsockopt(SocketOption option, int64_t& value) const {
    if (!option_int64(option)) {
      throw std::runtime_error("socket option: invalid option for data type.");
    }
    size_t value_size = sizeof(int64_t);
    return zmq_getsockopt(socket, static_cast<int32_t>(option), &value, &value_size) == 0;
  }

  bool getsockopt(SocketOption option, std::string& value) const {
    if (!option_string(option)) {
      throw std::runtime_error("socket option: invalid option for data type.");
    }
    const uint32_t buffer_size = 256;
    size_t value_size = buffer_size;
    char buffer[buffer_size];
    if (zmq_getsockopt(socket, static_cast<int32_t>(option), buffer, &value_size) < 0) {
      return false;
    }
    value.assign(buffer, value_size);
    return true;
  }

  bool bind(const std::string& addr) const {
    return zmq_bind(socket, addr.c_str()) == 0;
  }

  bool connect(const std::string& addr) const {
    return zmq_connect(socket, addr.c_str()) == 0;
  }

  bool disconnect(const std::string& addr) const {
    return zmq_disconnect(socket, addr.c_str()) == 0;
  }

  bool send(const Message& msg, bool block = true) const {
    if (msg.empty()) {
      return true;
    }
    for (Message::const_iterator part = msg.begin(), last = msg.end() - 1; part != last; ++part) {
      if (!send(*part, block, true)) {
        return false;
      }
    }
    return send(msg.back(), block, false);
  }

  bool recv(Message& msg, bool block = true) const {
    msg.clear();
    #if (ZMQ_VERSION_MAJOR >= 3)
    for (int32_t more = 1; more != 0; getsockopt(rcvmore, more)) {
    #else
    for (int64_t more = 1; more != 0; getsockopt(rcvmore, more)) {
    #endif
      zmq_msg_t part;
      zmq_msg_init(&part);
      if (!recv(part, block)) {
        return false;
      }
      msg.push(part);
    }
    return true;
  }

private:
  Socket& operator = (const Socket&); // non copyable

  void* socket;
  bool own_socket;

  #if (ZMQ_VERSION_MAJOR >= 3)
  static const uint32_t DONTWAIT = ZMQ_DONTWAIT;
  static const uint32_t SNDMORE = ZMQ_SNDMORE;
  #else
  static const uint32_t DONTWAIT = ZMQ_NOBLOCK;
  static const uint32_t SNDMORE = ZMQ_SNDMORE;
  #endif

  friend class Poller;
};


class Poller {
public:
  typedef void (*poll_callback)(const void*, const Socket&);

  void add(const Socket& socket, PollOption option, poll_callback callback = 0, const void* param = 0) {
    if (callback) {
      callbacks.insert(std::make_pair(socket.socket, std::make_pair(callback, param)));
    }
    zmq_pollitem_t item = {socket.socket, 0, (short)option, 0};
    items.insert(std::upper_bound(items.begin(), items.end(), item, less_than_socket()), item);
  }

  void remove(const Socket& socket) {
    zmq_pollitem_t item = {socket.socket, 0, 0, 0};
    PollItems::iterator poll_item = std::lower_bound(items.begin(), items.end(), item, less_than_socket());
    if (poll_item == items.end() || socket.socket < poll_item->socket)
      return;
    items.erase(poll_item);
    callbacks.erase(socket.socket);
  }

#define VECTOR_DATA(v) (v.size() > 0 ? &v[0] : 0)

  bool poll() {
    bool poll_socket = zmq_poll(VECTOR_DATA(items), items.size(), -1) > 0;
    return poll_socket ? dispatch() : false;
  }

  bool poll(uint32_t timeout) {
    bool poll_socket = zmq_poll(VECTOR_DATA(items), items.size(), timeout) > 0;
    return poll_socket ? dispatch() : false;
  }

#undef VECTOR_DATA

  bool dispatch() {
    for (CallbackItems::const_iterator socket = callbacks.begin(), last = callbacks.end(); socket != last; ++socket) {
      if (has_polled(Socket(socket->first)) && socket->second.first) {
        socket->second.first(socket->second.second, Socket(socket->first));
      }
    }
    return true;
  }

  bool has_polled(const Socket& socket) const {
    zmq_pollitem_t item = {socket.socket, 0, 0, 0};
    PollItems::const_iterator poll_item = std::lower_bound(items.begin(), items.end(), item, less_than_socket());
    if (poll_item == items.end() || socket.socket < poll_item->socket) {
      return false;
    }
    return poll_item->revents & poll_item->events;
  }

private:
  typedef std::pair<poll_callback, const void*> CallbackParam;
  typedef std::map<void*, CallbackParam> CallbackItems;
  typedef std::vector<zmq_pollitem_t> PollItems;

  PollItems items;
  CallbackItems callbacks;

  struct less_than_socket {
    inline bool operator() (const zmq_pollitem_t& item1, const zmq_pollitem_t& item2) {
      return (item1.socket < item2.socket);
    }
  };
};


} // zmqcpp

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif // _ZMQCPP_H_
