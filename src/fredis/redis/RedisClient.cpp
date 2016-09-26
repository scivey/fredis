#include "fredis/redis/RedisClient.h"
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/libevent.h>
#include <glog/logging.h>
#include <folly/ExceptionWrapper.h>
#include "fredis/redis/RedisError.h"
#include "fredis/redis/RedisRequestContext.h"
#include "fredis/folly_util/folly_util.h"

using namespace std;

namespace fredis { namespace redis {

using connect_future_t = typename RedisClient::connect_future_t;

RedisClient::RedisClient(folly::EventBase *base, const string &host, int port)
  : base_(base), host_(host), port_(port) {}

RedisClient::connect_future_t RedisClient::connect(
    folly::EventBase *base, string host, int port) {
  auto result = std::shared_ptr<RedisClient> {
    new RedisClient {base, host, port}
  };
  result->redisContext_ = redisAsyncConnect(result->host_.c_str(), result->port_);
  if (result->redisContext_->err) {
    folly::Try<shared_ptr<RedisClient>> errResult {
      folly::make_exception_wrapper<RedisIOError>(result->redisContext_->errstr)
    };
    return folly::makeFuture(errResult);
  }
  redisLibeventAttach(result->redisContext_, base->getLibeventBase());
  redisAsyncSetConnectCallback(result->redisContext_,
    &RedisClient::hiredisConnectCallback);
  redisAsyncSetDisconnectCallback(result->redisContext_,
    &RedisClient::hiredisDisconnectCallback);

  return result->connectPromise_.getFuture();
}

// RedisClient::connect_future_t RedisClient::connect(
//   folly::EventBase *base, string host, int port) {
//   return folly_util::runInEventLoop<connect_future_t>(base, [base, host, port]() {
//     return connectInternal(base, host, port);
//   });
// }

RedisClient::disconnect_future_t RedisClient::disconnect() {
  CHECK(!!redisContext_);
  redisAsyncDisconnect(redisContext_);
  return disconnectPromise_.getFuture();
}

// RedisClient::disconnect_future_t RedisClient::disconnect() {
//   return folly_util::runInEventLoop<connect_future_t>(base_, [this]() {
//     return disconnectInternal();
//   });
// }

RedisClient::response_future_t RedisClient::get(string key) {
  auto reqCtx = new RedisRequestContext {shared_from_this()};
  redisAsyncCommand(redisContext_,
    &RedisClient::hiredisCommandCallback,
    (void*) reqCtx,
    "GET %s", key.c_str()
  );
  return reqCtx->getFuture();
}

RedisClient::response_future_t RedisClient::set(string key, string val) {
  auto reqCtx = new RedisRequestContext {shared_from_this()};
  redisAsyncCommand(redisContext_,
    &RedisClient::hiredisCommandCallback,
    (void*) reqCtx,
    "SET %s %s", key.c_str(), val.c_str()
  );
  return reqCtx->getFuture();
}

void RedisClient::hiredisConnectCallback(const redisAsyncContext *ac, int status) {
  LOG(INFO) << "hiredisConnectCallback";
}

void RedisClient::hiredisDisconnectCallback(const redisAsyncContext *ac, int status) {
  LOG(INFO) << "hiredisDisconnectCallback";
}

void RedisClient::hiredisCommandCallback(redisAsyncContext *ac, void *ctx, void *data) {
  LOG(INFO) << "RedisClient::hiredisCommandCallback";
  auto reqCtx = (RedisRequestContext*) ctx;
  reqCtx->setValue(RedisResponse{});
}


}} // fredis::redis

