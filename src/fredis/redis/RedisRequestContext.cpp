#include "fredis/redis/RedisRequestContext.h"

using namespace std;

namespace fredis { namespace redis {

RedisRequestContext::RedisRequestContext(std::shared_ptr<RedisClient> client)
  : client_(client){}

RedisRequestContext::response_future_t RedisRequestContext::getFuture() {
  return donePromise_.getFuture();
}

}} // fredis::redis
