## fredis
Futures-based c++ wrappers for libhiredis and libmemcached's async transport modes.

This integrates with Folly's futures, which in turn are built on top of libevent.  Folly is a dependency.  Hiredis is a dependency.  Libmemcached is a dependency.  You and I are expendable.

You can see the basic idea in one of the integration tests [here](src/fredis/integration_tests/redis_integration.cpp).

This is still a work in progress - the async memcached driver is not yet fully functional.

