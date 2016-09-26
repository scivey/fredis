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


RedisClient::RedisClient(RedisClient &&other)
  : base_(other.base_),
    host_(other.host_),
    port_(other.port_),
    redisContext_(other.redisContext_),
    connectPromise_(std::move(other.connectPromise_)),
    disconnectPromise_(std::move(other.disconnectPromise_)) {
  other.redisContext_ = nullptr;
}

RedisClient& RedisClient::operator=(RedisClient &&other) {
  std::swap(base_, other.base_);
  std::swap(host_, other.host_);
  std::swap(port_, other.port_);
  std::swap(redisContext_, other.redisContext_);
  std::swap(connectPromise_, other.connectPromise_);
  std::swap(disconnectPromise_, other.disconnectPromise_);
  return *this;
}

std::shared_ptr<RedisClient> RedisClient::createShared(folly::EventBase *base,
      const string &host, int port) {
  return std::make_shared<RedisClient>(RedisClient(base, host, port));
}

RedisClient::connect_future_t RedisClient::connect() {
  CHECK(!redisContext_);
  redisContext_ = redisAsyncConnect(host_.c_str(), port_);
  if (redisContext_->err) {
    folly::Try<shared_ptr<RedisClient>> errResult {
      folly::make_exception_wrapper<RedisIOError>(redisContext_->errstr)
    };
    return folly::makeFuture(errResult);
  }
  hiredis_adapter::fredisLibeventAttach(
    this, redisContext_, base_->getLibeventBase()
  );
  redisAsyncSetConnectCallback(redisContext_,
    &RedisClient::hiredisConnectCallback);
  redisAsyncSetDisconnectCallback(redisContext_,
    &RedisClient::hiredisDisconnectCallback);
  return connectPromise_.getFuture();
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

void RedisClient::hiredisCommandCallback(redisAsyncContext *ac, void *reply, void *pdata) {
  LOG(INFO) << "RedisClient::hiredisCommandCallback";
  auto clientPtr = detail::getClientFromContext(ac);
  auto reqCtx = (RedisRequestContext*) pdata;
  auto bareReply = (redisReply*) reply;
  clientPtr->handleCommandResponse(reqCtx, RedisDynamicResponse {bareReply});
}

void RedisClient::handleConnected(int status) {
  CHECK(status == REDIS_OK);
  auto selfPtr = shared_from_this();
  connectPromise_.setValue(folly::Try<decltype(selfPtr)> {selfPtr});
}

void RedisClient::handleCommandResponse(RedisRequestContext *ctx, RedisDynamicResponse &&response) {
  ctx->setValue(std::forward<RedisDynamicResponse>(response));
}

void RedisClient::handleDisconnected(int status) {
  CHECK(status == REDIS_OK);
  disconnectPromise_.setValue(folly::Try<folly::Unit> {folly::Unit {}});
}

RedisClient::~RedisClient() {
  if (redisContext_) {
    delete redisContext_;
    redisContext_ = nullptr;
  }
}

namespace detail {
RedisClient* getClientFromContext(const redisAsyncContext *ctx) {
  auto evData = (hiredis_adapter::fredisLibeventEvents*) ctx->ev.data;
  return evData->client;
}
}

}} // fredis::redis

