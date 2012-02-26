ZMQ C++ wrapper (with no added magic)
=====================================

Welcome to the super-thin C++ wrapper for zmq version **1.1.0**.


Features
--------

* Very simple to understand and use.
* Design based on python bindings.
* Resource management (zmq messages, sockets, context, etc) taken care of.
* Messages easily constructed by streaming in types.


Usage
-----

To connect to another zmq socket and send a message::

    #include "zmqcpp.h"

    using namespace zmqcpp;

    Context ctx;
    Socket socket(ctx, xreq);
    socket.setsockopt(linger, 0);
    socket.connect("tcp://127.0.0.1:4050");

    Message msg;
    msg << "first part" << 123 << "third part";

    if (!socket.send(msg))
      std::cout << "Send error:" << socket.last_error() << "\n";

To bind a zmq socket and receive a message::

    #include "zmqcpp.h"

    using namespace zmqcpp;

    Context ctx;
    Socket socket(ctx, xrep);
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

    #include "zmqcpp.h"

    using namespace zmqcpp;

    Context ctx;
    Socket socket(ctx, xrep);
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

To receive a callback when a zmq socket is polled::

    #include "zmqcpp.h"

    using namespace zmqcpp;

    void socket_callback(const Socket& socket) {
      Message msg;
      socket.recv(msg);
      // doc something with msg here
    }

    ...

    Context ctx;
    Socket socket(ctx, xrep);
    socket.bind("tpc://*:4050");

    Poller poller;
    poller.add(socket, pollin, socket_callback);

    while (true) {
      poller.poll(1000); // 1 second timeout
      // callback function called here if polled
    }


For more examples, including copying messages and constructing messages from
custom data types, then check out the unit tests.


Build Instructions using CMake
------------------------------

Simply run CMake to generate the required Makefile or project and build the
unit test application test_zmqcpp.
