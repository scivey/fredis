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
using folly::fbstring;

namespace fredis { namespace redis {

using connect_future_t = typename RedisClient::connect_future_t;
using arg_str_ref = typename RedisClient::arg_str_ref;
using cmd_str_ref = typename RedisClient::cmd_str_ref;
using redis_signed_t = typename RedisClient::redis_signed_t;
using arg_str_list = typename RedisClient::arg_str_list;
using mset_init_list = typename RedisClient::mset_init_list;

RedisClient::RedisClient(folly::EventBase *base, const fbstring &host, int port)
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
      const fbstring &host, int port) {
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

RedisClient::response_future_t RedisClient::command0(cmd_str_ref cmd) {
  auto reqCtx = new RedisRequestContext {shared_from_this()};
  redisAsyncCommand(redisContext_,
    &RedisClient::hiredisCommandCallback,
    (void*) reqCtx,
    cmd.c_str()
  );
  return reqCtx->getFuture();
}

RedisClient::response_future_t RedisClient::command1(cmd_str_ref cmd,
    arg_str_ref arg) {
  auto reqCtx = new RedisRequestContext {shared_from_this()};
  redisAsyncCommand(redisContext_,
    &RedisClient::hiredisCommandCallback,
    (void*) reqCtx,
    cmd.c_str(), arg.c_str()
  );
  return reqCtx->getFuture();
}

RedisClient::response_future_t RedisClient::command2(cmd_str_ref cmd,
    arg_str_ref arg1, arg_str_ref arg2) {
  auto reqCtx = new RedisRequestContext {shared_from_this()};
  redisAsyncCommand(redisContext_,
    &RedisClient::hiredisCommandCallback,
    (void*) reqCtx,
    cmd.c_str(), arg1.c_str(), arg2.c_str()
  );
  return reqCtx->getFuture();
}

RedisClient::response_future_t RedisClient::command2(cmd_str_ref cmd,
    arg_str_ref arg1, redis_signed_t arg2) {
  auto reqCtx = new RedisRequestContext {shared_from_this()};
  redisAsyncCommand(redisContext_,
    &RedisClient::hiredisCommandCallback,
    (void*) reqCtx,
    cmd.c_str(), arg1.c_str(), arg2
  );
  return reqCtx->getFuture();
}


RedisClient::response_future_t RedisClient::get(arg_str_ref key) {
  return command1("GET %s", key);
}

RedisClient::response_future_t RedisClient::del(arg_str_ref key) {
  return command1("DEL %s", key);
}

RedisClient::response_future_t RedisClient::exists(arg_str_ref key) {
  return command1("EXISTS %s", key);
}

RedisClient::response_future_t RedisClient::expire(arg_str_ref key,
    redis_signed_t ttlSecs) {
  return command2("EXPIRE %s %i", key, ttlSecs);
}

RedisClient::response_future_t RedisClient::set(arg_str_ref key, arg_str_ref val) {
  return command2("SET %s %s", key, val);
}

RedisClient::response_future_t RedisClient::set(arg_str_ref key, redis_signed_t val) {
  return command2("SET %s %i", key, val);
}

RedisClient::response_future_t RedisClient::mset(mset_init_list&& msetList) {
  folly::fbvector<std::pair<arg_str_t, arg_str_t>> toMset{
    std::forward<mset_init_list>(msetList)
  };
  return mset(toMset);
}

RedisClient::response_future_t RedisClient::mget(mget_init_list&& mgetList) {
  folly::fbvector<arg_str_t> toMget{
    std::forward<mget_init_list>(mgetList)
  };
  return mget(toMget);
}

RedisClient::response_future_t RedisClient::setnx(arg_str_ref key,
    arg_str_ref val) {
  return command2("SETNX %s %s", key, val);
}

RedisClient::response_future_t RedisClient::setnx(arg_str_ref key,
    redis_signed_t val) {
  return command2("SETNX %s %i", key, val);
}

RedisClient::response_future_t RedisClient::getset(arg_str_ref key,
    arg_str_ref val) {
  return command2("GETSET %s %s", key, val);
}

RedisClient::response_future_t RedisClient::incr(arg_str_ref key) {
  return command1("INCR %s", key);
}

RedisClient::response_future_t RedisClient::incrby(arg_str_ref key,
    redis_signed_t amount) {
  return command2("INCRBY %s %i", key, amount);
}

RedisClient::response_future_t RedisClient::decr(arg_str_ref key) {
  return command1("DECR %s", key);
}

RedisClient::response_future_t RedisClient::decrby(arg_str_ref key,
    redis_signed_t amount) {
  return command2("DECRBY %s %i", key, amount);
}

RedisClient::response_future_t RedisClient::llen(arg_str_ref key) {
  return command1("LLEN %s", key);
}

RedisClient::response_future_t RedisClient::strlen(arg_str_ref key) {
  return command1("STRLEN %s", key);
}

void RedisClient::hiredisConnectCallback(const redisAsyncContext *ac, int status) {
  auto clientPtr = detail::getClientFromContext(ac);
  clientPtr->handleConnected(status);
}

void RedisClient::hiredisDisconnectCallback(const redisAsyncContext *ac, int status) {
  auto clientPtr = detail::getClientFromContext(ac);
  clientPtr->handleDisconnected(status);
}

void RedisClient::hiredisCommandCallback(redisAsyncContext *ac, void *reply, void *pdata) {
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

