#include "fredis/redis/RedisClient.h"
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <glog/logging.h>
#include <folly/ExceptionWrapper.h>
#include "fredis/redis/RedisError.h"
#include "fredis/redis/RedisRequestContext.h"
#include "fredis/folly_util/folly_util.h"
#include "fredis/redis/hiredis_adapter/hiredis_adapter.h"

using namespace std;

namespace fredis { namespace redis {

using connect_future_t = typename RedisClient::connect_future_t;

RedisClient::RedisClient(folly::EventBase *base, const string &host, int port)
  : base_(base), host_(host), port_(port) {}

RedisClient::connect_future_t RedisClient::connect(
    folly::EventBase *base, string host, int port) {
  auto result = std::make_shared<RedisClient>(base, host, port);
  result->redisContext_ = redisAsyncConnect(result->host_.c_str(), result->port_);
  if (result->redisContext_->err) {
    folly::Try<shared_ptr<RedisClient>> errResult {
      folly::make_exception_wrapper<RedisIOError>(result->redisContext_->errstr)
    };
    return folly::makeFuture(errResult);
  }
  hiredis_adapter::fredisLibeventAttach(
    result.get(), result->redisContext_, base->getLibeventBase()
  );
  redisAsyncSetConnectCallback(result->redisContext_,
    &RedisClient::hiredisConnectCallback);
  redisAsyncSetDisconnectCallback(result->redisContext_,
    &RedisClient::hiredisDisconnectCallback);

  return result->connectPromise_.getFuture();
}

RedisClient::disconnect_future_t RedisClient::disconnect() {
  CHECK(!!redisContext_);
  redisAsyncDisconnect(redisContext_);
  return disconnectPromise_.getFuture();
}

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
  auto clientPtr = detail::getClientFromContext(ac);
  clientPtr->handleConnected(status);
}

void RedisClient::hiredisDisconnectCallback(const redisAsyncContext *ac, int status) {
  LOG(INFO) << "hiredisDisconnectCallback";
  auto clientPtr = detail::getClientFromContext(ac);
  clientPtr->handleDisconnected(status);
}

void RedisClient::hiredisCommandCallback(redisAsyncContext *ac, void *ctx, void *data) {
  LOG(INFO) << "RedisClient::hiredisCommandCallback";
  auto clientPtr = detail::getClientFromContext(ac);
  auto reqCtx = (RedisRequestContext*) ctx;
  clientPtr->handleCommand(reqCtx, data);
}

void RedisClient::handleConnected(int status) {
  CHECK(status == REDIS_OK);
  auto selfPtr = shared_from_this();
  connectPromise_.setValue(folly::Try<decltype(selfPtr)> {selfPtr});
}

void RedisClient::handleCommand(RedisRequestContext *ctx, void *data) {
  ctx->setValue(RedisResponse{});
}

void RedisClient::handleDisconnected(int status) {
  CHECK(status == REDIS_OK);
  disconnectPromise_.setValue(folly::Try<folly::Unit> {folly::Unit {}});
}

namespace detail {
RedisClient* getClientFromContext(const redisAsyncContext *ctx) {
  auto evData = (hiredis_adapter::fredisLibeventEvents*) ctx->ev.data;
  return evData->client;
}
}

}} // fredis::redis

