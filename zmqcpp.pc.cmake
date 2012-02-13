prefix=@CMAKE_INSTALL_PREFIX@
exec_prefix=@CMAKE_INSTALL_PREFIX@
libdir=${prefix}/@LIB_INSTALL_DIR@
includedir=${prefix}/@INCLUDE_INSTALL_ROOT_DIR@

Name: zmqcpp
Description: ZMQ cpp wrapper
Version: @zmqcpp_VERSION@
Requires:
Libs: -L${libdir} -lzmq3
Cflags: -I${includedir}
