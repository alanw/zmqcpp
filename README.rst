ZMQ C++ wrapper (with no added magic)
=====================================

Welcome to the super-thin C++ wrapper for zmq version **1.0.0**.


Features
--------

* Very simple to understand and use.
* Design based on python bindings.
* Resource management (zmq messages, sockets, context, etc) taken care of.
* Messages easily constructed by streaming in types.


Usage
-----

To connect to another zmq socket and send a message::

    #define USE_ZMQ3 // if this is not defined then zmq2 is assumed
    #include "zmqcpp.h"

    using namespace zmqcpp

    Context ctx;
    Socket socket = ctx.socket(xreq);
    socket.setsockopt(linger, 0);
    socket.connect("tcp://127.0.0.1:4050");

    Message msg;
    msg << "first frame" << 123 << "third frame";

    if (!socket.send(msg))
      std::cout << "Send error:" << socket.last_error() << "\n";

To bind a zmq socket and receive a message::

    #define USE_ZMQ3 // if this is not defined then zmq2 is assumed
    #include "zmqcpp.h"

    using namespace zmqcpp

    Context ctx;
    Socket socket = ctx.socket(xrep);
    socket.bind("tpc://*:4050");

    Message msg;
    if (!socket.recv(msg)) {
      std::cout << "Recv error:" << socket.last_error() << "\n";
    }
    else {
      std::string first;
      int32_t second;
      std::string third;

      msg >> first >> second >> third;

    ...

    }

To poll a zmq socket::

    #define USE_ZMQ3 // if this is not defined then zmq2 is assumed
    #include "zmqcpp.h"

    using namespace zmqcpp

    Context ctx;
    Socket socket = ctx.socket(xrep);
    socket.bind("tpc://*:4050");

    Poller poller;
    poller.add(socket, pollin);

    Message msg;

    while (true) {
    poller.poll(1000); // 1 second timeout
      if (poller.has_polled(socket)) {
        socket.recv(msg);
        // do something with msg here
      }
    }

For more examples, including copying messages and constructing messages from
custom data types, then check out the unit tests.


Build Instructions using CMake
------------------------------

Simply run CMake to generate the required Makefile or project and build the
unit test application test_zmqcpp.
