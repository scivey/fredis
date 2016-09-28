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
#include <folly/FBString.h>
#include "fredis/redis/RedisRequestContext.h"
#include "fredis/redis/RedisSubscription.h"

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
  using response_t = typename RedisRequestContext::response_t;
  using response_future_t =
    typename RedisRequestContext::response_future_t;

  using cmd_str_t = folly::fbstring;
  using arg_str_t = cmd_str_t;
  using cmd_str_ref = const cmd_str_t&;
  using arg_str_ref = const arg_str_t&;
  using redis_signed_t = int64_t;
  using arg_str_list = folly::fbvector<arg_str_t>;
  using mset_list = folly::fbvector<std::pair<arg_str_t, arg_str_t>>;
  using subscription_t = RedisSubscription;
  using subscription_try_t = folly::Try<std::shared_ptr<subscription_t>>;
  using subscription_handler_ptr_t = subscription_t::handler_ptr_t;

 protected:
  folly::EventBase *base_ {nullptr};
  folly::fbstring host_;
  int port_ {0};
  struct redisAsyncContext *redisContext_ {nullptr};
  std::weak_ptr<subscription_t> currentSubscription_;
  connect_promise_t connectPromise_;
  disconnect_promise_t disconnectPromise_;

  // not really for public use.
  RedisClient(folly::EventBase *base,
    const folly::fbstring& host, int port);

  RedisClient(const RedisClient &other) = delete;
  RedisClient& operator=(const RedisClient &other) = delete;

  response_future_t command0(cmd_str_ref cmd);
  response_future_t command1(cmd_str_ref cmd, arg_str_ref arg);
  response_future_t command2(cmd_str_ref cmd, arg_str_ref arg1,
      arg_str_ref arg2);
  response_future_t command2(cmd_str_ref cmd, arg_str_ref arg1,
      redis_signed_t arg2);

 public:

  RedisClient(RedisClient &&other);
  RedisClient& operator=(RedisClient &&other);

  static std::shared_ptr<RedisClient> createShared(folly::EventBase *base,
    const folly::fbstring &host, int port);

  connect_future_t connect();
  disconnect_future_t disconnect();

  response_future_t get(arg_str_ref);
  response_future_t set(arg_str_ref, arg_str_ref);
  response_future_t set(arg_str_ref, redis_signed_t);


  template<typename TCollection>
  response_future_t mset(const TCollection &args) {
    std::ostringstream oss;
    oss << "MSET";
    for (const auto &keyVal: args) {
      oss << " " << keyVal.first << " " << keyVal.second;
    }
    return command0(oss.str());
  }

  using mset_init_list = std::initializer_list<std::pair<arg_str_t, arg_str_t>>;
  response_future_t mset(mset_init_list&& msetList);


  template<typename TCollection>
  response_future_t mget(const TCollection &args) {
    std::ostringstream oss;
    oss << "MGET";
    for (const auto &key: args) {
      oss << " " << key;
    }
    return command0(oss.str());
  }

  using mget_init_list = std::initializer_list<arg_str_t>;
  response_future_t mget(mget_init_list&& mgetList);
  response_future_t exists(arg_str_ref);
  response_future_t del(arg_str_ref);
  response_future_t expire(arg_str_ref, redis_signed_t);
  response_future_t setnx(arg_str_ref, arg_str_ref);
  response_future_t setnx(arg_str_ref, redis_signed_t);
  response_future_t getset(arg_str_ref, arg_str_ref);

  response_future_t keys(arg_str_ref pattern);

  response_future_t strlen(arg_str_ref);

  response_future_t decr(arg_str_ref);
  response_future_t decrby(arg_str_ref, redis_signed_t);
  response_future_t incr(arg_str_ref key);
  response_future_t incrby(arg_str_ref key, redis_signed_t);

  response_future_t llen(arg_str_ref key);

  subscription_try_t subscribe(subscription_handler_ptr_t, arg_str_ref);

 protected:
  // event handler methods called from the static handlers (because C)
  void handleConnected(int status);
  void handleCommandResponse(RedisRequestContext *ctx, response_t&& data);
  void handleDisconnected(int status);
  void handleSubscriptionEvent(response_t&& data);

 public:
  // these are public because hiredis needs access to them.
  // they aren't really "public" public
  static void hiredisConnectCallback(const redisAsyncContext*, int status);
  static void hiredisCommandCallback(redisAsyncContext*, void *reply, void *pdata);
  static void hiredisDisconnectCallback(const redisAsyncContext*, int status);
  static void hiredisSubscriptionCallback(redisAsyncContext*, void *reply, void *pdata);
  ~RedisClient();
};

namespace detail {
RedisClient* getClientFromContext(const redisAsyncContext* ctx);
}


}} // fredis::redis


