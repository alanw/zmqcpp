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
//   #define USE_ZMQ3 // if this is not defined then zmq2 is assumed
//   #include "zmqcpp.h"
//
//   using namespace zmqcpp
//
//   Context ctx;
//   Socket socket = ctx.socket(xreq);
//   socket.setsockopt(linger, 0);
//   socket.connect("tcp://127.0.0.1:4050");
//
//   Message msg;
//   msg << "first frame" << 123 << "third frame";
//
//   if (!socket.send(msg))
//     std::cout << "Send error:" << socket.last_error() << "\n";
//
// To bind a zmq socket and receive a message:
//
//   #define USE_ZMQ3 // if this is not defined then zmq2 is assumed
//   #include "zmqcpp.h"
//
//   using namespace zmqcpp
//
//   Context ctx;
//   Socket socket = ctx.socket(xrep);
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
//   #define USE_ZMQ3 // if this is not defined then zmq2 is assumed
//   #include "zmqcpp.h"
//
//   using namespace zmqcpp
//
//   Context ctx;
//   Socket socket = ctx.socket(xrep);
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
// For more examples, including copying messages and constructing messages from
// custom data types, then check out the unit tests.
//

#ifndef _ZMQCPP_H_
#define _ZMQCPP_H_

#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <algorithm>
#include <stdexcept>
#include <stdint.h>

#ifdef USE_ZMQ3
#include <zmq3/zmq.h>
#else
#include <zmq.h>
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
  dealer = ZMQ_DEALER
};

enum SocketOption {
  #ifdef USE_ZMQ3
  maxmsgsize = ZMQ_MAXMSGSIZE,
  sndhwm = ZMQ_SNDHWM,
  rcvhwm = ZMQ_RCVHWM,
  multicast_hops = ZMQ_MULTICAST_HOPS,
  rcvtimeo = ZMQ_RCVTIMEO,
  sndtimeo = ZMQ_SNDTIMEO,
  ipv4only = ZMQ_IPV4ONLY,
  #else
  hwm = ZMQ_HWM,
  swap = ZMQ_SWAP,
  mcastloop = ZMQ_MCAST_LOOP,
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
  reconnect_ivl_max = ZMQ_RECONNECT_IVL_MAX
};

enum PollOption {
  pollin = ZMQ_POLLIN,
  pollout = ZMQ_POLLOUT,
  pollerror = ZMQ_POLLERR
};


class Message : public std::deque<zmq_msg_t> {
public:
  Message() {
    // default
  }

  Message(const zmq_msg_t& msg) {
    push(msg);
  }

  Message(const void* data, uint32_t size, zmq_free_fn* free_fn = 0) {
    push(data, size, free_fn);
  }

  template <typename Iter>
  Message(Iter first, Iter last) {
    for (; first != last; ++first)
      push(*first);
  }

  ~Message() {
    for (iterator frame = begin(), last = end(); frame != last; ++frame)
      zmq_msg_close(&(*frame));
  }

  void copy(Message& copy_msg) const {
    for (const_iterator frame = begin(), last = end(); frame != last; ++frame) {
      zmq_msg_t copy_frame;
      zmq_msg_init(&copy_frame);
      zmq_msg_copy(&copy_frame, const_cast<zmq_msg_t*>(&(*frame)));
      copy_msg.push(copy_frame);
    }
  }

  void push(const zmq_msg_t& msg) {
    push_back(msg);
  }

  void push(const void* data, size_t size, zmq_free_fn* free_fn = 0) {
    zmq_msg_t msg;
    zmq_msg_init_data(&msg, const_cast<void*>(data), size, free_fn, 0);
    push(msg);
  }

  void pop(zmq_msg_t& msg) {
    if (empty())
      throw std::runtime_error("message: no more frames.");
    msg = front();
    pop_front();
  }

  void pop(void** data, size_t* size) {
    if (empty())
      throw std::runtime_error("message: no more frames.");
    zmq_msg_t& msg = front();
    *data = zmq_msg_data(&msg);
    *size = zmq_msg_size(&msg);
    pop_front();
  }
};

Message& operator << (Message& message, uint8_t value) {
  zmq_msg_t msg;
  zmq_msg_init_size(&msg, sizeof(uint8_t));
  *reinterpret_cast<uint8_t*>(zmq_msg_data(&msg)) = value;
  message.push(msg);
  return message;
}

Message& operator << (Message& message, int8_t value) {
  return message << static_cast<uint8_t>(value);
}

Message& operator << (Message& message, bool value) {
  return message << static_cast<uint8_t>(value ? 1 : 0);
}

Message& operator << (Message& message, uint16_t value) {
  zmq_msg_t msg;
  zmq_msg_init_size(&msg, sizeof(uint16_t));
  *reinterpret_cast<uint16_t*>(zmq_msg_data(&msg)) = value;
  message.push(msg);
  return message;
}

Message& operator << (Message& message, int16_t value) {
  return message << static_cast<uint16_t>(value);
}

Message& operator << (Message& message, uint32_t value) {
  zmq_msg_t msg;
  zmq_msg_init_size(&msg, sizeof(uint32_t));
  *reinterpret_cast<uint32_t*>(zmq_msg_data(&msg)) = value;
  message.push(msg);
  return message;
}

Message& operator << (Message& message, int32_t value) {
  return message << static_cast<uint32_t>(value);
}

Message& operator << (Message& message, uint64_t value) {
  zmq_msg_t msg;
  zmq_msg_init_size(&msg, sizeof(uint64_t));
  *reinterpret_cast<uint64_t*>(zmq_msg_data(&msg)) = value;
  message.push(msg);
  return message;
}

Message& operator << (Message& message, int64_t value) {
  return message << static_cast<uint64_t>(value);
}

Message& operator << (Message& message, const std::string& value) {
  zmq_msg_t msg;
  zmq_msg_init_size(&msg, value.length());
  std::copy(value.begin(), value.end(), reinterpret_cast<char*>(zmq_msg_data(&msg)));
  message.push(msg);
  return message;
}

template <typename Type>
Message& operator << (Message& message, const Type& value) {
  std::ostringstream stream;
  stream << value;
  return message << stream.str();
}

Message& operator >> (Message& message, uint8_t& value) {
  zmq_msg_t msg;
  message.pop(msg);
  if (zmq_msg_size(&msg) != sizeof(uint8_t))
    throw std::runtime_error("message: type mismatch.");
  value = *reinterpret_cast<uint8_t*>(zmq_msg_data(&msg));
  return message;
}

Message& operator >> (Message& message, int8_t& value) {
  return message >> reinterpret_cast<uint8_t&>(value);
}

Message& operator >> (Message& message, bool& value) {
  uint8_t msg_value;
  message >> reinterpret_cast<uint8_t&>(msg_value);
  value = msg_value != 0;
  return message;
}

Message& operator >> (Message& message, uint16_t& value) {
  zmq_msg_t msg;
  message.pop(msg);
  if (zmq_msg_size(&msg) != sizeof(uint16_t))
    throw std::runtime_error("message: type mismatch.");
  value = *reinterpret_cast<uint16_t*>(zmq_msg_data(&msg));
  return message;
}

Message& operator >> (Message& message, int16_t& value) {
  return message >> reinterpret_cast<uint16_t&>(value);
}

Message& operator >> (Message& message, uint32_t& value) {
  zmq_msg_t msg;
  message.pop(msg);
  if (zmq_msg_size(&msg) != sizeof(uint32_t))
    throw std::runtime_error("message: type mismatch.");
  value = *reinterpret_cast<uint32_t*>(zmq_msg_data(&msg));
  return message;
}

Message& operator >> (Message& message, int32_t& value) {
  return message >> reinterpret_cast<uint32_t&>(value);
}

Message& operator >> (Message& message, uint64_t& value) {
  zmq_msg_t msg;
  message.pop(msg);
  if (zmq_msg_size(&msg) != sizeof(uint64_t))
    throw std::runtime_error("message: type mismatch.");
  value = *reinterpret_cast<uint64_t*>(zmq_msg_data(&msg));
  return message;
}

Message& operator >> (Message& message, int64_t& value) {
  return message >> reinterpret_cast<uint64_t&>(value);
}

Message& operator >> (Message& message, std::string& value) {
  zmq_msg_t msg;
  message.pop(msg);
  value.assign(reinterpret_cast<char*>(zmq_msg_data(&msg)), zmq_msg_size(&msg));
  return message;
}

template <typename Type>
Message& operator >> (Message& message, Type& value) {
  zmq_msg_t msg;
  message.pop(msg);
  std::istringstream stream(std::string(reinterpret_cast<char*>(zmq_msg_data(&msg)), zmq_msg_size(&msg)));
  stream >> value;
  return message;
}


class Socket {
private:
  Socket(void* ctx, SocketType type) {
    socket = zmq_socket(ctx, type);
  }

  bool send(const zmq_msg_t& msg, bool block = true, bool more = false) const {
    uint32_t flags = 0;
    if (!block)
      flags |= DONTWAIT;
    if (more)
      flags |= SNDMORE;
    #ifdef USE_ZMQ3
    return zmq_sendmsg(socket, const_cast<zmq_msg_t*>(&msg), flags) >= 0;
    #else
    return zmq_send(socket, const_cast<zmq_msg_t*>(&msg), flags) >= 0;
    #endif
  }

  bool recv(const zmq_msg_t& msg, bool block = true) const {
    #ifdef USE_ZMQ3
    return zmq_recvmsg(socket, const_cast<zmq_msg_t*>(&msg), block ? 0 : DONTWAIT) >= 0;
    #else
    return zmq_recv(socket, const_cast<zmq_msg_t*>(&msg), block ? 0 : DONTWAIT) >= 0;
    #endif
  }

  bool option_int32(SocketOption option) const {
    #ifdef USE_ZMQ3
    return option != maxmsgsize && option != affinity && option != subscribe && option != unsubscribe && option != identity;
    #else
    return option != affinity && option != subscribe && option != unsubscribe && option != identity;
    #endif
  }

  bool option_int64(SocketOption option) const {
    #ifdef USE_ZMQ3
    return option == maxmsgsize || option == affinity;
    #else
    return option == affinity;
    #endif
  }

  bool option_string(SocketOption option) const {
    return option == subscribe || option == unsubscribe || option == identity;
  }

public:
  ~Socket() {
    close();
  }

  void close() {
    zmq_close(socket);
    socket = 0;
  }

  std::string last_error() const {
    return zmq_strerror(zmq_errno());
  }

  bool setsockopt(SocketOption option, int32_t value) const {
    if (!option_int32(option))
      throw std::runtime_error("socket option: invalid option for data type.");
    return zmq_setsockopt(socket, static_cast<int32_t>(option), &value, sizeof(int32_t)) == 0;
  }

  bool setsockopt(SocketOption option, int64_t value) const {
    if (!option_int64(option))
      throw std::runtime_error("socket option: invalid option for data type.");
    return zmq_setsockopt(socket, static_cast<int32_t>(option), &value, sizeof(int64_t)) == 0;
  }

  bool setsockopt(SocketOption option, const std::string& value) const {
    if (!option_string(option))
      throw std::runtime_error("socket option: invalid option for data type.");
    return zmq_setsockopt(socket, static_cast<int>(option), value.c_str(), value.length()) == 0;
  }

  bool getsockopt(SocketOption option, int32_t& value) const {
    if (!option_int32(option))
      throw std::runtime_error("socket option: invalid option for data type.");
    size_t value_size = sizeof(int32_t);
    return zmq_getsockopt(socket, static_cast<int32_t>(option), &value, &value_size) == 0;
  }

  bool getsockopt(SocketOption option, int64_t& value) const {
    if (!option_int64(option))
      throw std::runtime_error("socket option: invalid option for data type.");
    size_t value_size = sizeof(int64_t);
    return zmq_getsockopt(socket, static_cast<int32_t>(option), &value, &value_size) == 0;
  }

  bool getsockopt(SocketOption option, std::string& value) const {
    if (!option_int64(option))
      throw std::runtime_error("socket option: invalid option for data type.");
    const uint32_t buffer_size = 256;
    size_t value_size = buffer_size;
    char buffer[buffer_size];
    if (zmq_getsockopt(socket, static_cast<int32_t>(option), buffer, &value_size) < 0)
      return false;
    value.assign(buffer, value_size);
    return true;
  }

  bool bind(const std::string& addr) const {
    return zmq_bind(socket, addr.c_str()) == 0;
  }

  bool connect(const std::string& addr) const {
    return zmq_connect(socket, addr.c_str()) == 0;
  }

  bool send(const Message& msg, bool block = true) const {
    if (msg.empty())
      return true;
    for (Message::const_iterator frame = msg.begin(), last = msg.end() - 1; frame != last; ++frame) {
      if (!send(*frame, block, true)) {
        return false;
      }
    }
    return send(msg.back(), block, false);
  }

  bool recv(Message& msg, bool block = true) const {
    msg.clear();
    for (int32_t more = 1L; more != 0L; getsockopt(rcvmore, more)) {
      zmq_msg_t frame;
      zmq_msg_init(&frame);
      if (!recv(frame, block)) {
        return false;
      }
      msg.push(frame);
    }
    return true;
  }

private:
  void* socket;

  #ifdef USE_ZMQ3
  static const uint32_t DONTWAIT = ZMQ_DONTWAIT;
  static const uint32_t SNDMORE = ZMQ_SNDMORE;
  #else
  static const uint32_t DONTWAIT = ZMQ_NOBLOCK;
  static const uint32_t SNDMORE = ZMQ_SNDMORE;
  #endif

  friend class Context;
  friend class Poller;
};


class Poller {
public:
  void add(const Socket& socket, PollOption option) {
    zmq_pollitem_t item = {socket.socket, 0, option, 0};
    items.insert(std::upper_bound(items.begin(), items.end(), item, less_than_socket()), item);
  }

  void remove(const Socket& socket) {
    zmq_pollitem_t item = {socket.socket, 0, 0, 0};
    PollItems::iterator poll_item = std::lower_bound(items.begin(), items.end(), item, less_than_socket());
    if (poll_item == items.end() || socket.socket < poll_item->socket)
      return;
    items.erase(poll_item);
  }

  bool poll() {
    return zmq_poll(items.data(), items.size(), -1) > 0;
  }

  bool poll(uint32_t timeout) {
    return zmq_poll(items.data(), items.size(), timeout) > 0;
  }

  bool has_polled(const Socket& socket) const {
    zmq_pollitem_t item = {socket.socket, 0, 0, 0};
    PollItems::const_iterator poll_item = std::lower_bound(items.begin(), items.end(), item, less_than_socket());
    if (poll_item == items.end() || socket.socket < poll_item->socket)
      return false;
    return poll_item->revents & poll_item->events;
  }

private:
  struct less_than_socket {
    inline bool operator() (const zmq_pollitem_t& item1, const zmq_pollitem_t& item2) {
      return ( item1.socket < item2.socket );
    }
  };
  typedef std::vector<zmq_pollitem_t> PollItems;
  PollItems items;
};


class Context {
public:
  Context(bool auto_term = true, uint32_t threads = 1) {
    this->auto_term = auto_term;
    context = zmq_init(threads);
  }

  ~Context() {
    if (auto_term) {
      term();
    }
  }

  void term() {
    zmq_term(context);
    context = 0;
  }

  Socket socket(SocketType type) const {
    return Socket(context, type);
  }

private:
  void* context;
  bool auto_term;
};

} // zmq

#endif /* _ZMQCPP_H_ */
