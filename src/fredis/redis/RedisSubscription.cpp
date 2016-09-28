#include "fredis/redis/RedisSubscription.h"

using namespace std;
using folly::make_exception_wrapper;
using folly::Unit;
using folly::Try;

namespace fredis { namespace redis {

using handler_ptr_t = typename RedisSubscription::handler_ptr_t;

RedisSubscription::RedisSubscription(shared_ptr<RedisClient> client,
    handler_ptr_t handler)
  : client_(client), handler_(std::forward<handler_ptr_t>(handler)) {}

RedisSubscription::RedisSubscription(RedisSubscription &&other)
    : client_(std::move(other.client_)), handler_(std::move(other.handler_)) {
  stopping_.store(other.stopping_.load());
  updateHandlerParent();
}

RedisSubscription& RedisSubscription::operator=(RedisSubscription &&other) {
  std::swap(client_, other.client_);
  std::swap(handler_, other.handler_);
  bool selfStopping = stopping_.load();
  stopping_.store(other.stopping_.load());
  other.stopping_.store(selfStopping);
  updateHandlerParent();
  other.updateHandlerParent();
  return *this;
}

bool RedisSubscription::isAlive() const {
  return !!handler_ && !stopping_.load(std::memory_order_acquire);
}

Try<Unit> RedisSubscription::stop() {
  if (!!handler_) {
    return Try<Unit> {
      make_exception_wrapper<RedisSubscription::NotActive>("NotActive")
    };
  }
  bool failed = false;
  for (;;) {
    if (stopping_.load(std::memory_order_acquire)) {
      failed = true;
      break;
    }
    bool expected = false;
    bool desired = true;
    if (stopping_.compare_exchange_strong(expected, desired)) {
      break;
    }
  }
  if (!!handler_) {
    return Try<Unit> {
      make_exception_wrapper<RedisSubscription::NotActive>("NotActive")
    };
  }
  if (failed) {
    return Try<Unit> {
      make_exception_wrapper<RedisSubscription::AlreadyStopping>("AlreadyStopping")
    };
  }
  return Try<Unit>{Unit{}};
}

void RedisSubscription::updateHandlerParent() {
  if (handler_) {
    handler_->setParent(this);
  }
}

void RedisSubscription::EventHandler::setParent(RedisSubscription *parent) {
  parent_ = parent;
}

RedisSubscription* RedisSubscription::EventHandler::getParent() const {
  return parent_;
}

void RedisSubscription::dispatchMessage(RedisDynamicResponse&& message) {
  DCHECK(!!handler_);
  if (!stopping_.load(std::memory_order_relaxed)) {
    handler_->onMessage(std::forward<RedisDynamicResponse>(message));
  }
}

void RedisSubscription::EventHandler::stop() {
  auto parent = getParent();
  DCHECK(!!parent);
  auto result = parent->stop();
  if (result.hasException()) {
    LOG(INFO) << "unexpected exception on RedisSubscription::EventHandler::stop()"
              << ": '" << result.exception().what() << "'";
  }
}

shared_ptr<RedisSubscription> RedisSubscription::createShared(
    shared_ptr<RedisClient> client, handler_ptr_t handler) {
  return std::shared_ptr<RedisSubscription> {
    new RedisSubscription {client, std::forward<handler_ptr_t>(handler) }
  };
}

}} // fredis::redis


