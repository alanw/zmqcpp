//
// Test zmq cpp wrapper.
//
// Copyright (c) 2012 Alan Wright. All rights reserved.
// Distributable under the terms of either the Apache License (Version 2.0)
//

#include <gtest/gtest.h>
#include <zmq.h>

#include "zmqcpp.h"

using namespace zmqcpp;

class zmqcpp_fixture : public testing::Test {
protected:
  void assert_equal(const Message& msg1, const Message& msg2, bool same = false) {
    ASSERT_EQ(msg1.size(), msg2.size());
    for (uint32_t part = 0, last = msg1.size() - 1; part != last; ++part) {
      Message::MessageBuffer buffer1(Message::message_buffer(msg1.at(part)));
      Message::MessageBuffer buffer2(Message::message_buffer(msg2.at(part)));
      ASSERT_EQ(buffer1.size, buffer2.size);
      if (!same) {
        ASSERT_NE(buffer1.data, buffer2.data);
      }
      ASSERT_TRUE(memcmp(buffer1.data, buffer2.data, buffer1.size) == 0);
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

TEST_F(zmqcpp_fixture, test_message_message_string) {
  const char buffer[] = "this is a test message";

  Message input_msg(buffer, sizeof(buffer));
  std::string test_output = Message::message_string(input_msg.front());

  ASSERT_STREQ(test_output.c_str(), "this is a test message");
}

TEST_F(zmqcpp_fixture, test_message_message_size) {
  Message input_msg;
  input_msg << "this is a test message";
  ASSERT_EQ(Message::message_buffer(input_msg.front()).size, 22);
}

TEST_F(zmqcpp_fixture, test_message_message_empty) {
  Message first_msg;
  first_msg << "hello world";

  Message second_msg((void*)0, 0);

  ASSERT_FALSE(Message::message_empty(first_msg.front()));
  ASSERT_TRUE(Message::message_empty(second_msg.front()));
}

TEST_F(zmqcpp_fixture, test_serialize_single_part) {
  Message first_msg;
  first_msg << "hello world";

  Message second_msg;
  second_msg << "hello world";

  assert_equal(first_msg, second_msg);
}

TEST_F(zmqcpp_fixture, test_serialize_multiple_parts) {
  Message first_msg;
  first_msg << "hello world" << (uint8_t)123 << true;

  Message second_msg;
  second_msg << "hello world" << (uint8_t)123 << true;

  assert_equal(first_msg, second_msg);
}

TEST_F(zmqcpp_fixture, test_message_bytes) {
  Message test_msg;
  test_msg << "hello world" << "this is a test";

  ASSERT_EQ(test_msg.message_bytes(), 25);
}

TEST_F(zmqcpp_fixture, test_deserialize_single_part) {
  Message test_msg;
  test_msg << "hello world";

  std::string part;
  test_msg >> part;
  ASSERT_EQ(part, "hello world");
  ASSERT_ANY_THROW(test_msg >> part);
}

TEST_F(zmqcpp_fixture, test_deserialize_empty_message) {
  Message test_msg;

  std::string part;
  ASSERT_ANY_THROW(test_msg >> part);
}

TEST_F(zmqcpp_fixture, test_deserialize_multiple_parts) {
  Message test_msg;
  test_msg << "hello world" << (uint8_t)123 << true;

  std::string part1;
  uint8_t part2;
  bool part3;

  test_msg >> part1 >> part2 >> part3;
  ASSERT_EQ(part1, "hello world");
  ASSERT_EQ(part2, 123);
  ASSERT_EQ(part3, true);
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

  uint8_t part1; int8_t part2;
  bool part3; bool part4;
  uint16_t part5; int16_t part6;
  uint32_t part7; int32_t part8;
  uint64_t part9; int64_t part10;
  std::string part11; std::string part12;
  std::string part13; std::string part14;
  double part15;

  ASSERT_EQ(test_msg.size(), 15);
  test_msg >> part1 >> part2 >> part3 >> part4 >> part5;
  test_msg >> part6 >> part7 >> part8 >> part9 >> part10;
  test_msg >> part11 >> part12 >> part13 >> part14 >> part15;

  ASSERT_EQ(part1, 12);
  ASSERT_EQ(part2, 23);
  ASSERT_EQ(part3, false);
  ASSERT_EQ(part4, true);
  ASSERT_EQ(part5, 3456);
  ASSERT_EQ(part6, 4567);
  ASSERT_EQ(part7, 56789);
  ASSERT_EQ(part8, 67890);
  ASSERT_EQ(part9, 7890123);
  ASSERT_EQ(part10, 8901234);
  ASSERT_EQ(part11, "short");
  ASSERT_EQ(part12, "a somewhat longer string");
  ASSERT_EQ(part13, "a string with\0 a null character");
  ASSERT_EQ(part14, "a string with \xe1\xbc\xa0\xce\xb9 unicode characters");
  ASSERT_EQ(part15, 3.141);
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

  int32_t part1;
  int64_t part2;

  ASSERT_ANY_THROW(test_msg >> part1);
  ASSERT_ANY_THROW(test_msg >> part2);
}

TEST_F(zmqcpp_fixture, test_copy_message) {
  Message test_msg;
  test_msg << "hello world" << (uint8_t)123 << true;

  Message copy_msg(test_msg.begin(), test_msg.end());

  assert_equal(test_msg, copy_msg);
}

TEST_F(zmqcpp_fixture, test_message_assign_single) {
  Message first_msg;
  first_msg << "hello world";

  Message second_msg;
  second_msg.assign(first_msg.front(), false);

  assert_equal(first_msg, second_msg, true);
}

TEST_F(zmqcpp_fixture, test_message_assign_single_copy) {
  Message first_msg;
  first_msg << "hello world";

  Message second_msg;
  second_msg.assign(first_msg.front(), true);

  assert_equal(first_msg, second_msg);
}

TEST_F(zmqcpp_fixture, test_message_assign_range) {
  Message first_msg;
  first_msg << "hello world" << (uint8_t)123 << true;

  Message second_msg;
  second_msg.assign(first_msg.begin(), first_msg.end(), false);

  assert_equal(first_msg, second_msg, true);
}

TEST_F(zmqcpp_fixture, test_message_assign_range_copy) {
  Message first_msg;
  first_msg << "hello world" << (uint8_t)123 << true;

  Message second_msg;
  second_msg.assign(first_msg.begin(), first_msg.end(), true);

  assert_equal(first_msg, second_msg);
}

TEST_F(zmqcpp_fixture, test_message_insert_single_first) {
  Message first_msg;
  first_msg << "hello world";

  Message second_msg;
  second_msg << "insert test";

  first_msg.insert(first_msg.begin(), second_msg.front(), false);

  std::string part1;
  std::string part2;

  ASSERT_EQ(first_msg.size(), 2);
  first_msg >> part1 >> part2;

  ASSERT_EQ(part1, "insert test");
  ASSERT_EQ(part2, "hello world");
}

TEST_F(zmqcpp_fixture, test_message_insert_single_last) {
  Message first_msg;
  first_msg << "hello world";

  Message second_msg;
  second_msg << "insert test";

  first_msg.insert(first_msg.end(), second_msg.front(), false);

  std::string part1;
  std::string part2;

  ASSERT_EQ(first_msg.size(), 2);
  first_msg >> part1 >> part2;

  ASSERT_EQ(part1, "hello world");
  ASSERT_EQ(part2, "insert test");
}

TEST_F(zmqcpp_fixture, test_message_insert_range) {
  Message first_msg;
  first_msg << "hello world";
  first_msg << "second part";

  Message second_msg;
  second_msg << "insert test 1";
  second_msg << "insert test 2";

  first_msg.insert(first_msg.begin() + 1, second_msg.begin(), second_msg.end(), false);

  std::string part1;
  std::string part2;
  std::string part3;
  std::string part4;

  ASSERT_EQ(first_msg.size(), 4);
  first_msg >> part1 >> part2 >> part3 >> part4;

  ASSERT_EQ(part1, "hello world");
  ASSERT_EQ(part2, "insert test 1");
  ASSERT_EQ(part3, "insert test 2");
  ASSERT_EQ(part4, "second part");
}

TEST_F(zmqcpp_fixture, test_socket_single_part) {
  Context ctx;

  Socket bind_sock(ctx, pair);
  ASSERT_TRUE(bind_sock.bind("tcp://*:4050"));

  Socket connect_sock(ctx, pair);
  ASSERT_TRUE(connect_sock.connect("tcp://localhost:4050"));

  Message send_msg;
  send_msg << "hello world";

  ASSERT_TRUE(connect_sock.send(send_msg));

  Message recv_msg;
  bind_sock.recv(recv_msg);

  assert_equal(send_msg, recv_msg);
}

TEST_F(zmqcpp_fixture, test_socket_multiple_parts) {
  Context ctx;

  Socket bind_sock(ctx, rep);
  ASSERT_TRUE(bind_sock.bind("tcp://*:4050"));

  Socket connect_sock(ctx, req);
  ASSERT_TRUE(connect_sock.connect("tcp://localhost:4050"));

  Message send_msg;
  send_msg << "hello world" << (uint8_t)123;

  Message validate_msg(send_msg.begin(), send_msg.end());

  ASSERT_TRUE(connect_sock.send(send_msg));

  Message recv_msg;
  bind_sock.recv(recv_msg);

  assert_equal(validate_msg, recv_msg);
}

TEST_F(zmqcpp_fixture, test_socket_setsockopt_int32) {
  Context ctx;

  Socket test_sock(ctx, push);
  test_sock.setsockopt(zmqcpp::linger, 123);

  int32_t test_linger = 0;
  test_sock.getsockopt(zmqcpp::linger, test_linger);
  ASSERT_EQ(test_linger, 123);
}

TEST_F(zmqcpp_fixture, test_socket_setsockopt_int64) {
  Context ctx;

  Socket test_sock(ctx, push);
  test_sock.setsockopt(affinity, (int64_t)123456789);

  int64_t test_affinity = 0;
  test_sock.getsockopt(affinity, test_affinity);
  ASSERT_EQ(test_affinity, (int64_t)123456789);
}

TEST_F(zmqcpp_fixture, test_socket_setsockopt_string) {
  Context ctx;

  Socket test_sock(ctx, sub);
  ASSERT_TRUE(test_sock.setsockopt(subscribe, "hello world"));
}

TEST_F(zmqcpp_fixture, test_socket_setsockopt_invalid_type) {
  Context ctx;

  Socket test_sock(ctx, sub);
  ASSERT_ANY_THROW(test_sock.setsockopt(subscribe, 123));
  ASSERT_ANY_THROW(test_sock.setsockopt(affinity, 123));
  ASSERT_ANY_THROW(test_sock.setsockopt(zmqcpp::linger, "throw"));
}

TEST_F(zmqcpp_fixture, test_socket_getsockopt_invalid_type) {
  Context ctx;

  Socket test_sock(ctx, sub);
  int32_t test_affinity = 0;
  ASSERT_ANY_THROW(test_sock.getsockopt(affinity, test_affinity));
  int64_t test_rcvbuf = 0;
  ASSERT_ANY_THROW(test_sock.getsockopt(zmqcpp::linger, test_rcvbuf));
  std::string test_linger;
  ASSERT_ANY_THROW(test_sock.getsockopt(zmqcpp::linger, test_linger));
}

TEST_F(zmqcpp_fixture, test_socket_send_noblock) {
  Context ctx;

  Socket test_sock(ctx, push);
  test_sock.setsockopt(zmqcpp::linger, 0);
  #if (ZMQ_VERSION_MAJOR >= 3)
  test_sock.setsockopt(sndhwm, 10);
  #else
  test_sock.setsockopt(hwm, (int64_t)10);
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

  Socket test_sock(ctx, pull);
  test_sock.setsockopt(zmqcpp::linger, 0);
  test_sock.bind("tcp://*:4050");

  Poller test_poller;
  test_poller.add(test_sock, pollin);

  ASSERT_FALSE(test_poller.poll(10));
}

TEST_F(zmqcpp_fixture, test_poller_single_socket) {
  Context ctx;

  Socket bind_sock(ctx, rep);
  ASSERT_TRUE(bind_sock.bind("tcp://*:4050"));

  Socket connect_sock(ctx, req);
  ASSERT_TRUE(connect_sock.connect("tcp://localhost:4050"));

  Message send_msg;
  send_msg << "hello world";

  Message validate_msg(send_msg.begin(), send_msg.end());
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

  Socket bind_sock1(ctx, rep);
  bind_sock1.bind("tcp://*:4050");

  Socket bind_sock2(ctx, rep);
  bind_sock2.bind("tcp://*:4051");

  Socket connect_sock(ctx, req);
  ASSERT_TRUE(connect_sock.connect("tcp://localhost:4050"));

  Message send_msg;
  send_msg << "hello world";

  Message validate_msg(send_msg.begin(), send_msg.end());
  ASSERT_TRUE(connect_sock.send(send_msg));

  Poller test_poller;
  test_poller.add(bind_sock1, pollin);
  test_poller.add(bind_sock2, pollin);

  Sleep(100);

  ASSERT_TRUE(test_poller.poll());
  ASSERT_TRUE(test_poller.has_polled(bind_sock1));
  ASSERT_FALSE(test_poller.has_polled(bind_sock2));

  Message recv_msg;
  bind_sock1.recv(recv_msg, false);

  assert_equal(validate_msg, recv_msg);
}

TEST_F(zmqcpp_fixture, test_poller_multiple_sockets_multiple_poll) {
  Context ctx;

  Socket bind_sock1(ctx, rep);
  bind_sock1.bind("tcp://*:4050");

  Socket bind_sock2(ctx, rep);
  bind_sock2.bind("tcp://*:4051");

  Socket connect_sock1(ctx, req);
  connect_sock1.connect("tcp://localhost:4050");

  Socket connect_sock2(ctx, req);
  connect_sock2.connect("tcp://localhost:4051");

  Message send_msg1;
  send_msg1 << "hello world";

  Message send_msg2;
  send_msg2 << "hello again";

  Message validate_msg1(send_msg1.begin(), send_msg1.end());
  connect_sock1.send(send_msg1);

  Message validate_msg2(send_msg2.begin(), send_msg2.end());
  connect_sock2.send(send_msg2);

  Poller test_poller;
  test_poller.add(bind_sock1, pollin);
  test_poller.add(bind_sock2, pollin);

  Sleep(100);

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

  Socket bind_sock1(ctx, rep);
  bind_sock1.bind("tcp://*:4050");

  Socket bind_sock2(ctx, rep);
  bind_sock2.bind("tcp://*:4051");

  Socket connect_sock1(ctx, req);
  connect_sock1.connect("tcp://localhost:4050");

  Socket connect_sock2(ctx, req);
  connect_sock2.connect("tcp://localhost:4051");

  Message send_msg1;
  send_msg1 << "hello world";

  Message send_msg2;
  send_msg2 << "hello again";
  connect_sock2.send(send_msg2);

  Message validate_msg1(send_msg1.begin(), send_msg1.end());
  connect_sock1.send(send_msg1);

  Poller test_poller;
  test_poller.add(bind_sock1, pollin);
  test_poller.add(bind_sock2, pollin);
  test_poller.remove(bind_sock2);

  Sleep(100);

  ASSERT_TRUE(test_poller.poll());
  ASSERT_TRUE(test_poller.has_polled(bind_sock1));
  ASSERT_FALSE(test_poller.has_polled(bind_sock2));

  Message recv_msg1;
  bind_sock1.recv(recv_msg1, false);

  assert_equal(validate_msg1, recv_msg1);
}

namespace {

bool callback_called = false;

void test_poll_callback(const void* param, const Socket& socket) {
  callback_called = true;

  Message recv_msg;
  socket.recv(recv_msg, false);

  std::string output;
  recv_msg >> output;

  ASSERT_EQ(output, "hello world");
}

TEST_F(zmqcpp_fixture, test_poller_callback) {
  Context ctx;

  Socket bind_sock(ctx, rep);
  ASSERT_TRUE(bind_sock.bind("tcp://*:4050"));

  Socket connect_sock(ctx, req);
  ASSERT_TRUE(connect_sock.connect("tcp://localhost:4050"));

  Message send_msg;
  send_msg << "hello world";

  Message validate_msg(send_msg.begin(), send_msg.end());
  ASSERT_TRUE(connect_sock.send(send_msg));

  Poller test_poller;
  test_poller.add(bind_sock, pollin, test_poll_callback);

  ASSERT_TRUE(test_poller.poll());
  ASSERT_TRUE(test_poller.has_polled(bind_sock));
  ASSERT_TRUE(callback_called);
}

}
