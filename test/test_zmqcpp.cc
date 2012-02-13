//
// Test zmq cpp wrapper.
//
// Copyright (c) 2012 Alan Wright. All rights reserved.
// Distributable under the terms of either the Apache License (Version 2.0)
//

#include <gtest/gtest.h>

#define USE_ZMQ3
#include "zmqcpp.h"

using namespace zmqcpp;

class zmqcpp_fixture : public testing::Test {
protected:
  void assert_equal(const Message& msg1, const Message& msg2) {
    ASSERT_EQ(msg1.size(), msg2.size());
    for (Message::size_type frame = 0, last = msg1.size() - 1; frame != last; ++ frame) {
      ASSERT_EQ(zmq_msg_size(const_cast<zmq_msg_t*>(&msg1[frame])), zmq_msg_size(const_cast<zmq_msg_t*>(&msg2[frame])));
      ASSERT_TRUE(memcmp(zmq_msg_data(const_cast<zmq_msg_t*>(&msg1[frame])), zmq_msg_data(const_cast<zmq_msg_t*>(&msg2[frame])), zmq_msg_size(const_cast<zmq_msg_t*>(&msg1[frame]))) == 0);
    }
  }
};

TEST_F(zmqcpp_fixture, test_message_buffer) {
  const char buffer[] = "this is a test message";

  Message first_msg(buffer, sizeof(buffer));

  Message second_msg;
  second_msg << buffer;

  assert_equal(first_msg, second_msg);
}

TEST_F(zmqcpp_fixture, test_message_buffer_no_copy) {
  const char input_buffer[] = "this is a test message";

  Message test_msg;
  test_msg.push(input_buffer, sizeof(input_buffer));

  void* output_buffer;
  size_t output_size;

  test_msg.pop(&output_buffer, &output_size);

  ASSERT_EQ((void*)input_buffer, (void*)output_buffer);
}

TEST_F(zmqcpp_fixture, test_serialize_single_frame) {
  Message first_msg;
  first_msg << "hello world";

  Message second_msg;
  second_msg << "hello world";

  assert_equal(first_msg, second_msg);
}

TEST_F(zmqcpp_fixture, test_serialize_multiple_frames) {
  Message first_msg;
  first_msg << "hello world" << (uint8_t)123 << true;

  Message second_msg;
  second_msg << "hello world" << (uint8_t)123 << true;

  assert_equal(first_msg, second_msg);
}

TEST_F(zmqcpp_fixture, test_deserialize_single_frame) {
  Message test_msg;
  test_msg << "hello world";

  std::string frame;
  test_msg >> frame;
  ASSERT_EQ(frame, "hello world");
  ASSERT_ANY_THROW(test_msg >> frame);
}

TEST_F(zmqcpp_fixture, test_deserialize_emoty_message) {
  Message test_msg;

  std::string frame;
  ASSERT_ANY_THROW(test_msg >> frame);
}

TEST_F(zmqcpp_fixture, test_deserialize_multiple_frames) {
  Message test_msg;
  test_msg << "hello world" << (uint8_t)123 << true;

  std::string frame1;
  uint8_t frame2;
  bool frame3;

  test_msg >> frame1 >> frame2 >> frame3;
  ASSERT_EQ(frame1, "hello world");
  ASSERT_EQ(frame2, 123);
  ASSERT_EQ(frame3, true);
}

TEST_F(zmqcpp_fixture, test_deserialize_all_types) {
  Message test_msg;
  test_msg << (uint8_t)12 << (int8_t)23 << false << true;
  test_msg << (uint16_t)3456 << (int16_t)4567;
  test_msg << (uint32_t)56789 << (int32_t)67890;
  test_msg << (uint64_t)7890123 << (int64_t)8901234;
  test_msg << "short" << "a somewhat longer string";
  test_msg << "a string with\0 a null character";
  test_msg << "a string with \xe1\xbc\xa0\xce\xb9 unicode characters";
  test_msg << (double)3.141;

  uint8_t frame1; int8_t frame2;
  bool frame3; bool frame4;
  uint16_t frame5; int16_t frame6;
  uint32_t frame7; int32_t frame8;
  uint64_t frame9; int64_t frame10;
  std::string frame11; std::string frame12;
  std::string frame13; std::string frame14;
  double frame15;

  ASSERT_EQ(test_msg.size(), 15);
  test_msg >> frame1 >> frame2 >> frame3 >> frame4 >> frame5;
  test_msg >> frame6 >> frame7 >> frame8 >> frame9 >> frame10;
  test_msg >> frame11 >> frame12 >> frame13 >> frame14 >> frame15;

  ASSERT_EQ(frame1, 12);
  ASSERT_EQ(frame2, 23);
  ASSERT_EQ(frame3, false);
  ASSERT_EQ(frame4, true);
  ASSERT_EQ(frame5, 3456);
  ASSERT_EQ(frame6, 4567);
  ASSERT_EQ(frame7, 56789);
  ASSERT_EQ(frame8, 67890);
  ASSERT_EQ(frame9, 7890123);
  ASSERT_EQ(frame10, 8901234);
  ASSERT_EQ(frame11, "short");
  ASSERT_EQ(frame12, "a somewhat longer string");
  ASSERT_EQ(frame13, "a string with\0 a null character");
  ASSERT_EQ(frame14, "a string with \xe1\xbc\xa0\xce\xb9 unicode characters");
  ASSERT_EQ(frame15, 3.141);
}

namespace {

struct CustomType {
  int32_t first;
  std::string second;
};

Message& operator << (Message& message, const CustomType& value) {
  return message << value.first << value.second;
}

Message& operator >> (Message& message, CustomType& value) {
  return message >> value.first >> value.second;
}

TEST_F(zmqcpp_fixture, test_serialize_and_deserialize_custom_type) {
  CustomType input_custom;
  input_custom.first = 123;
  input_custom.second = "custom type";

  Message test_msg;
  test_msg << input_custom;

  CustomType output_custom;
  test_msg >> output_custom;

  ASSERT_EQ(output_custom.first, 123);
  ASSERT_EQ(output_custom.second, "custom type");
}
}

TEST_F(zmqcpp_fixture, test_deserialize_invalid) {
  Message test_msg;
  test_msg << true << (uint32_t)123;

  int32_t frame1;
  int64_t frame2;

  ASSERT_ANY_THROW(test_msg >> frame1);
  ASSERT_ANY_THROW(test_msg >> frame2);
}

TEST_F(zmqcpp_fixture, test_copy_message) {
  Message test_msg;
  test_msg << "hello world" << (uint8_t)123 << true;

  Message copy_msg;
  test_msg.copy(copy_msg);

  assert_equal(test_msg, copy_msg);
}

TEST_F(zmqcpp_fixture, test_socket_single_frame) {
  Context ctx;

  Socket bind_sock = ctx.socket(rep);
  ASSERT_TRUE(bind_sock.bind("tcp://*:4050"));

  Socket connect_sock = ctx.socket(req);
  ASSERT_TRUE(connect_sock.connect("tcp://localhost:4050"));

  Message send_msg;
  send_msg << "hello world";

  ASSERT_TRUE(connect_sock.send(send_msg));

  Message recv_msg;
  bind_sock.recv(recv_msg);

  assert_equal(send_msg, recv_msg);
}

TEST_F(zmqcpp_fixture, test_socket_multiple_frames) {
  Context ctx;

  Socket bind_sock = ctx.socket(rep);
  ASSERT_TRUE(bind_sock.bind("tcp://*:4050"));

  Socket connect_sock = ctx.socket(req);
  ASSERT_TRUE(connect_sock.connect("tcp://localhost:4050"));

  Message send_msg;
  send_msg << "hello world" << (uint8_t)123;

  Message validate_msg;
  send_msg.copy(validate_msg);

  ASSERT_TRUE(connect_sock.send(send_msg));

  Message recv_msg;
  bind_sock.recv(recv_msg);

  assert_equal(validate_msg, recv_msg);
}

TEST_F(zmqcpp_fixture, test_socket_setsockopt_int32) {
  Context ctx;

  Socket test_sock = ctx.socket(push);
  test_sock.setsockopt(linger, 123);

  int32_t test_linger = 0;
  test_sock.getsockopt(linger, test_linger);
  ASSERT_EQ(test_linger, 123);
}

TEST_F(zmqcpp_fixture, test_socket_setsockopt_int64) {
  Context ctx;

  Socket test_sock = ctx.socket(push);
  test_sock.setsockopt(affinity, (int64_t)123456789);

  int64_t test_affinity = 0;
  test_sock.getsockopt(affinity, test_affinity);
  ASSERT_EQ(test_affinity, (int64_t)123456789);
}

TEST_F(zmqcpp_fixture, test_socket_setsockopt_string) {
  Context ctx;

  Socket test_sock = ctx.socket(sub);
  ASSERT_TRUE(test_sock.setsockopt(subscribe, "hello world"));
}

TEST_F(zmqcpp_fixture, test_socket_setsockopt_invalid_type) {
  Context ctx;

  Socket test_sock = ctx.socket(sub);
  ASSERT_ANY_THROW(test_sock.setsockopt(subscribe, 123));
  ASSERT_ANY_THROW(test_sock.setsockopt(affinity, 123));
  ASSERT_ANY_THROW(test_sock.setsockopt(linger, "throw"));
}

TEST_F(zmqcpp_fixture, test_socket_getsockopt_invalid_type) {
  Context ctx;

  Socket test_sock = ctx.socket(sub);
  int32_t test_affinity = 0;
  ASSERT_ANY_THROW(test_sock.getsockopt(affinity, test_affinity));
  int64_t test_rcvbuf = 0;
  ASSERT_ANY_THROW(test_sock.getsockopt(rcvbuf, test_rcvbuf));
  std::string test_linger;
  ASSERT_ANY_THROW(test_sock.getsockopt(linger, test_linger));
}

TEST_F(zmqcpp_fixture, test_socket_send_noblock) {
  Context ctx;

  Socket test_sock = ctx.socket(push);
  test_sock.setsockopt(linger, 0);
  #ifdef USE_ZMQ3
  test_sock.setsockopt(sndhwm, 10);
  #else
  test_sock.setsockopt(hwm, 10);
  #endif
  test_sock.connect("tcp://localhost:4050");

  bool send_success = true;

  int32_t test_events = 0;
  ASSERT_TRUE(test_sock.getsockopt(events, test_events));
  ASSERT_TRUE(test_events & ZMQ_POLLOUT);

  // trigger socket exceptional state
  for (int32_t i = 0; i < 11; ++i) {
    Message send_msg;
    send_msg << "hello world";
    send_success = test_sock.send(send_msg, false);
  }

  ASSERT_TRUE(test_sock.getsockopt(events, test_events));
  ASSERT_FALSE(test_events & ZMQ_POLLOUT);
  ASSERT_FALSE(send_success);
}

TEST_F(zmqcpp_fixture, test_poller_no_sockets) {
  Poller test_poller;
  ASSERT_FALSE(test_poller.poll(0));
}

TEST_F(zmqcpp_fixture, test_poller_timeout) {
  Context ctx;

  Socket test_sock = ctx.socket(pull);
  test_sock.setsockopt(linger, 0);
  test_sock.bind("tcp://*:4050");

  Poller test_poller;
  test_poller.add(test_sock, pollin);

  ASSERT_FALSE(test_poller.poll(10));
}

TEST_F(zmqcpp_fixture, test_poller_single_socket) {
  Context ctx;

  Socket bind_sock = ctx.socket(rep);
  ASSERT_TRUE(bind_sock.bind("tcp://*:4050"));

  Socket connect_sock = ctx.socket(req);
  ASSERT_TRUE(connect_sock.connect("tcp://localhost:4050"));

  Message send_msg;
  send_msg << "hello world";

  Message validate_msg;
  send_msg.copy(validate_msg);
  ASSERT_TRUE(connect_sock.send(send_msg));

  Poller test_poller;
  test_poller.add(bind_sock, pollin);

  ASSERT_TRUE(test_poller.poll());
  ASSERT_TRUE(test_poller.has_polled(bind_sock));

  Message recv_msg;
  bind_sock.recv(recv_msg, false);

  assert_equal(validate_msg, recv_msg);
}

TEST_F(zmqcpp_fixture, test_poller_multiple_sockets_single_poll) {
  Context ctx;

  Socket bind_sock1 = ctx.socket(rep);
  bind_sock1.bind("tcp://*:4050");

  Socket bind_sock2 = ctx.socket(rep);
  bind_sock2.bind("tcp://*:4051");

  Socket connect_sock = ctx.socket(req);
  ASSERT_TRUE(connect_sock.connect("tcp://localhost:4050"));

  Message send_msg;
  send_msg << "hello world";

  Message validate_msg;
  send_msg.copy(validate_msg);
  ASSERT_TRUE(connect_sock.send(send_msg));

  Poller test_poller;
  test_poller.add(bind_sock1, pollin);
  test_poller.add(bind_sock2, pollin);

  ASSERT_TRUE(test_poller.poll());
  ASSERT_TRUE(test_poller.has_polled(bind_sock1));
  ASSERT_FALSE(test_poller.has_polled(bind_sock2));

  Message recv_msg;
  bind_sock1.recv(recv_msg, false);

  assert_equal(validate_msg, recv_msg);
}

TEST_F(zmqcpp_fixture, test_poller_multiple_sockets_multiple_poll) {
  Context ctx;

  Socket bind_sock1 = ctx.socket(rep);
  bind_sock1.bind("tcp://*:4050");

  Socket bind_sock2 = ctx.socket(rep);
  bind_sock2.bind("tcp://*:4051");

  Socket connect_sock1 = ctx.socket(req);
  connect_sock1.connect("tcp://localhost:4050");

  Socket connect_sock2 = ctx.socket(req);
  connect_sock2.connect("tcp://localhost:4051");

  Message send_msg1;
  send_msg1 << "hello world";

  Message send_msg2;
  send_msg2 << "hello again";

  Message validate_msg1;
  send_msg1.copy(validate_msg1);
  connect_sock1.send(send_msg1);

  Message validate_msg2;
  send_msg2.copy(validate_msg2);
  connect_sock2.send(send_msg2);

  Poller test_poller;
  test_poller.add(bind_sock1, pollin);
  test_poller.add(bind_sock2, pollin);

  ASSERT_TRUE(test_poller.poll());
  ASSERT_TRUE(test_poller.has_polled(bind_sock1));
  ASSERT_TRUE(test_poller.has_polled(bind_sock2));

  Message recv_msg1;
  bind_sock1.recv(recv_msg1, false);

  Message recv_msg2;
  bind_sock2.recv(recv_msg2, false);

  assert_equal(validate_msg1, recv_msg1);
  assert_equal(validate_msg2, recv_msg2);
}

TEST_F(zmqcpp_fixture, test_poller_remove_socket) {
  Context ctx;

  Socket bind_sock1 = ctx.socket(rep);
  bind_sock1.bind("tcp://*:4050");

  Socket bind_sock2 = ctx.socket(rep);
  bind_sock2.bind("tcp://*:4051");

  Socket connect_sock1 = ctx.socket(req);
  connect_sock1.connect("tcp://localhost:4050");

  Socket connect_sock2 = ctx.socket(req);
  connect_sock2.connect("tcp://localhost:4051");

  Message send_msg1;
  send_msg1 << "hello world";

  Message send_msg2;
  send_msg2 << "hello again";
  connect_sock2.send(send_msg2);

  Message validate_msg1;
  send_msg1.copy(validate_msg1);
  connect_sock1.send(send_msg1);

  Poller test_poller;
  test_poller.add(bind_sock1, pollin);
  test_poller.add(bind_sock2, pollin);
  test_poller.remove(bind_sock2);

  ASSERT_TRUE(test_poller.poll());
  ASSERT_TRUE(test_poller.has_polled(bind_sock1));
  ASSERT_FALSE(test_poller.has_polled(bind_sock2));

  Message recv_msg1;
  bind_sock1.recv(recv_msg1, false);

  assert_equal(validate_msg1, recv_msg1);
}
