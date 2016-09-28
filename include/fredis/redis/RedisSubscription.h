#pragma once
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <functional>
#include <atomic>
#include <folly/io/async/EventBase.h>
#include <folly/futures/Future.h>
#include <folly/futures/Unit.h>
#include <folly/futures/Try.h>
#include <folly/FBString.h>
#include "fredis/redis/RedisDynamicResponse.h"
#include "fredis/redis/RedisError.h"
#include "fredis/macros.h"

namespace fredis { namespace redis {

class RedisClient;

class RedisSubscription: public std::enable_shared_from_this<RedisSubscription> {
 public:

  #define X FREDIS_DECLARE_EXCEPTION
    X(SubscriptionError, RedisError)
    X(NotActive, SubscriptionError)
    X(AlreadyStopping, SubscriptionError)
  #undef X

  class EventHandler {
   protected:
    RedisSubscription *parent_ {nullptr};
    void setParent(RedisSubscription *parent);
    friend class RedisSubscription;
   public:
    RedisSubscription* getParent() const;
    virtual void stop();
    virtual void onStarted() = 0;
    virtual void onMessage(RedisDynamicResponse&& message) = 0;
    virtual void onStopped() = 0;
    virtual ~EventHandler() = default;
  };
  friend class RedisClient;

  using handler_ptr_t = std::unique_ptr<EventHandler>;
 protected:
  std::shared_ptr<RedisClient> client_;
  handler_ptr_t handler_;
  std::atomic<bool> stopping_ {false};
  void updateHandlerParent();
  RedisSubscription(
    std::shared_ptr<RedisClient> client,
    handler_ptr_t handler
  );
  void dispatchMessage(RedisDynamicResponse&& message);
  static std::shared_ptr<RedisSubscription> createShared(
    std::shared_ptr<RedisClient>, handler_ptr_t
  );
 public:
  folly::Try<folly::Unit> stop();
  RedisSubscription(RedisSubscription &&other);
  RedisSubscription& operator=(RedisSubscription &&other);
  bool isAlive() const;
};

}} // fredis::redis


