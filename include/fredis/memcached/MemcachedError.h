#pragma once

#include "fredis/macros.h"
#include "fredis/FredisError.h"



namespace fredis { namespace memcached {

FREDIS_DECLARE_EXCEPTION(MemcachedError, fredis::FredisError);
FREDIS_DECLARE_EXCEPTION(ConfigurationError, MemcachedError);
FREDIS_DECLARE_EXCEPTION(ConnectionError, MemcachedError);
FREDIS_DECLARE_EXCEPTION(AlreadyConnected, ConnectionError);
FREDIS_DECLARE_EXCEPTION(ProtocolError, MemcachedError);


}} // fredis::memcached
