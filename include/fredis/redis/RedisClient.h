#pragma once
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <functional>
#include <folly/io/async/EventBase.h>
#include <folly/futures/Future.h>
#include <folly/futures/Unit.h>
#include <folly/futures/Try.h>
#include "fredis/redis/RedisRequestContext.h"

struct redisAsyncContext;

namespace fredis { namespace redis {

class RedisClient: public std::enable_shared_from_this<RedisClient> {
 public:
  using connect_promise_t = folly::Promise<
    folly::Try<std::shared_ptr<RedisClient>>
  >;
  using connect_future_t = decltype(
    std::declval<connect_promise_t>().getFuture()
  );
  using disconnect_promise_t = folly::Promise<
    folly::Try<folly::Unit>
  >;
  using disconnect_future_t = decltype(
    std::declval<disconnect_promise_t>().getFuture()
  );
  using response_future_t =
    typename RedisRequestContext::response_future_t;

 protected:
  folly::EventBase *base_ {nullptr};
  std::string host_;
  int port_ {0};
  struct redisAsyncContext *redisContext_ {nullptr};
  connect_promise_t connectPromise_;
  disconnect_promise_t disconnectPromise_;

 public:

  // not really for public use.
  RedisClient(folly::EventBase *base,
    const std::string& host, int port);

  static connect_future_t connect(
    folly::EventBase *evBase, std::string host, int port
  );
  disconnect_future_t disconnect();
  response_future_t get(std::string key);
  response_future_t set(std::string key, std::string val);

 protected:
  // event handler methods called from the static handlers (because C)
  void handleConnected(int status);
  void handleCommand(RedisRequestContext *ctx, void *data);
  void handleDisconnected(int status);

 public:
  // these are public because hiredis needs access to them.
  // they aren't really "public" public
  static void hiredisConnectCallback(const redisAsyncContext*, int status);
  static void hiredisCommandCallback(redisAsyncContext*, void *ctx, void *pdata);
  static void hiredisDisconnectCallback(const redisAsyncContext*, int status);

};

namespace detail {
RedisClient* getClientFromContext(const redisAsyncContext* ctx);
}


}} // fredis::redis


