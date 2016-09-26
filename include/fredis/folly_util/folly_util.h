#pragma once
#include <memory>
#include <type_traits>
#include <folly/futures/Future.h>
#include <folly/futures/Promise.h>
#include <folly/io/async/EventBase.h>

namespace fredis { namespace folly_util {

// template<typename TCallable>
// folly::Future<decltype(std::declval<TCallable>()())> runInEventLoop(
//     folly::EventBase *evBase, TCallable &&callable) {
//   using result_t = decltype(callable());
//   using move_wrapper_t = folly::MoveWrapper<TCallable>;
//   auto resultPromise = std::make_shared<folly::Promise<result_t>>();
//   auto wrapped = folly::makeMoveWrapper<TCallable>(std::move(callable));
//   evBase->runInEventBaseThread([resultPromise, wrapped](){
//     move_wrapper_t moved = wrapped;
//     TCallable unwrapped = moved.move();
//     resultPromise->setValue(unwrapped());
//   });

//   // extra layer here is to keep resultPromise alive in a closure.
//   return resultPromise->getFuture().then([resultPromise](result_t result) {
//     return result;
//   });
// }

template<typename TResult, typename TCallable>
folly::Future<TResult> runInEventLoop(
    folly::EventBase *evBase, const TCallable &callable) {
  using result_t = TResult;
  auto resultPromise = std::make_shared<folly::Promise<result_t>>();
  evBase->runInEventBaseThread([resultPromise, callable](){
    result_t result = callable();
    resultPromise->setValue(std::move(result));
  });

  // extra layer here is to keep resultPromise alive in a closure.
  return resultPromise->getFuture().then(
    [resultPromise]
    (result_t result) -> result_t {
      return result;
    }
  );
}

}} // fredis::folly_util