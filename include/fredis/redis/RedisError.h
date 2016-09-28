#pragma once

#include <stdexcept>
#include "fredis/FredisError.h"
#include "fredis/macros.h"

namespace fredis { namespace redis {

#define X FREDIS_DECLARE_EXCEPTION

X(RedisError, FredisError);
X(RedisIOError, RedisError);
X(RedisProtocolError, RedisError);
X(RedisEOFError, RedisError);
X(RedisTypeError, RedisError);
X(AlreadySubscribedError, RedisError);
X(SubscriptionError, RedisError);

#undef X



}} // fredis::redis
