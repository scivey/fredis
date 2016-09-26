#pragma once
#include <thread>
#include <memory>
#include <atomic>
#include <functional>
#include <glog/logging.h>
#include <folly/io/async/EventBase.h>
#include <folly/MoveWrapper.h>
#include "fredis/FredisError.h"
#include "fredis/macros.h"

namespace fredis { namespace folly_util {

template<typename TEventBase>
class EBThreadBase {
 public:
  FREDIS_DECLARE_EXCEPTION(EBThreadError, FredisError);
  FREDIS_DECLARE_EXCEPTION(AlreadyStarted, EBThreadError);
  FREDIS_DECLARE_EXCEPTION(NotRunning, EBThreadError);
  using void_func_t = std::function<void ()>;

 protected:
  std::unique_ptr<std::thread> thread_ {nullptr};
  std::unique_ptr<TEventBase> base_ {nullptr};
  std::atomic<bool> running_ {false};
  EBThreadBase(){}
 public:

  static std::shared_ptr<EBThreadBase> createShared() {
    std::shared_ptr<EBThreadBase> instance {new EBThreadBase};
    instance->base_.reset(new TEventBase);
    return instance;
  }
  bool isRunning(std::memory_order memOrder = std::memory_order_seq_cst) const {
    return running_.load(memOrder);
  }
  void start() {
    if (isRunning()) {
      FREDIS_THROW0(AlreadyStarted);
    }
    DCHECK(!thread_);
    bool expected = false;
    bool desired = true;
    if (!running_.compare_exchange_strong(expected, desired)) {
      FREDIS_THROW0(AlreadyStarted);
    }
    thread_.reset(new std::thread([this]() {
      base_->loopForever();
    }));
  }
  void stop() {
    if (!isRunning()) {
      FREDIS_THROW0(NotRunning);
    }
    DCHECK(!!thread_);
    bool expected = true;
    bool desired = false;
    if (!running_.compare_exchange_strong(expected, desired)) {
      FREDIS_THROW0(NotRunning);
    }
    base_->terminateLoopSoon();
  }
  void join() {
    CHECK(!!thread_);
    thread_->join();
  }
  TEventBase* getBase() {
    return base_.get();
  }

  template<typename TCallable>
  void runInEventBaseThread(TCallable &&callable) {
    DCHECK(isRunning());
    using wrapper_t = folly::MoveWrapper<TCallable>;
    wrapper_t wrapper { std::move(callable) };
    base_->runInEventBaseThread([wrapper]() {
      wrapper_t moved = wrapper;
      TCallable unwrapped = moved.move();
      unwrapped();
    });
  }

  template<typename TCallable>
  void runInEventBaseThread(const TCallable &callable) {
    DCHECK(isRunning());
    base_->runInEventBaseThread([callable]() {
      callable();
    });
  }
};

using EBThread = EBThreadBase<folly::EventBase>;

}} // score::folly_util

