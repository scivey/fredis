#include <gtest/gtest.h>
#include <atomic>
#include <folly/io/async/EventBase.h>
#include <folly/futures/Future.h>
#include <folly/Optional.h>
#include <folly/ExceptionWrapper.h>
#include <folly/Baton.h>
#include <folly/Conv.h>

#include "fredis/folly_util/EBThread.h"
#include "fredis/redis/RedisClient.h"
#include "fredis/redis/RedisDynamicResponse.h"

using namespace fredis::redis;
using namespace std;
using fredis::folly_util::EBThread;
using ResponseType = RedisDynamicResponse::ResponseType;


struct TestContext {
  TestContext(){
    ebt = EBThread::createShared();
  }
  std::shared_ptr<EBThread> ebt {nullptr};
  std::string redisHost {"127.0.0.1"};
  int redisPort {6379};
  folly::Baton<std::atomic> baton;
  std::shared_ptr<RedisClient> clientRef {nullptr};

  using try_connect_t = folly::Try<shared_ptr<RedisClient>>;
  using connect_cb_t = std::function<void (try_connect_t)>;

  void start(connect_cb_t cb) {
    ebt->ensureStarted();
    ebt->runInEventBaseThread([this, cb]() {
      auto client = RedisClient::createShared(ebt->getBase(), redisHost, redisPort);
      clientRef = client;
      client->connect().then([client, this, cb](try_connect_t result) {
        cb(result);
      });
    });
  }

  void post() {
    baton.post();
  }

  void wait() {
    baton.wait();
  }

  void stop() {
    ebt->stop();
  }

  ~TestContext() {
    if (ebt) {
      if (ebt->isRunning()) {
        stop();
      }
      ebt->join();
    }
  }
};

#define EXPECT_INT_RESPONSE(responseOpt, n) \
  do { \
    EXPECT_TRUE(responseOpt.value().isType(ResponseType::INTEGER)); \
    EXPECT_EQ(n, responseOpt.value().getInt().value()); \
  } while (0)

#define EXPECT_STRING_RESPONSE(responseOpt, aStr) \
  do { \
    EXPECT_TRUE(responseOpt.value().isType(ResponseType::STRING)); \
    EXPECT_EQ(std::string {aStr}, responseOpt.value().getString().value()); \
  } while (0)


#define EXPECT_STATUS(responseOpt) \
  do { \
    EXPECT_TRUE(responseOpt.value().isType(ResponseType::STATUS)); \
  } while (0);


using response_t = RedisDynamicResponse;
using try_response_t = folly::Try<response_t>;

TEST(TestRedisIntegration, TestSmoke1) {
  TestContext ctx;
  std::atomic<int> someTag {0};
  ctx.start([&ctx, &someTag](folly::Try<shared_ptr<RedisClient>> clientOpt) {
    auto clientPtr = clientOpt.value();
    clientPtr->set("foo", "fishes")
      .then([clientPtr, &ctx](try_response_t responseOpt) {
        EXPECT_TRUE(responseOpt.hasValue());
        auto response = responseOpt.value();
        EXPECT_TRUE(response.isType(RedisDynamicResponse::ResponseType::STATUS));
      })
      .then([clientPtr, &ctx]() {
        return clientPtr->get("foo");
      })
      .then([clientPtr, &ctx, &someTag](try_response_t responseOpt) {
        EXPECT_TRUE(responseOpt.hasValue());
        auto response = responseOpt.value();
        EXPECT_TRUE(response.isType(RedisDynamicResponse::ResponseType::STRING));
        auto asString = response.getString();
        EXPECT_TRUE(asString.hasValue());
        auto fromPiece = folly::to<std::string>(asString.value());
        EXPECT_EQ("fishes", fromPiece);
      })
      .then([&someTag, &ctx]() {
        someTag.store(17);
        ctx.post();
      });
  });
  ctx.wait();
  int result = someTag.load();
  EXPECT_EQ(17, result);
}




TEST(TestRedisIntegration, TestIncrDecr) {
  TestContext ctx;
  std::atomic<int> someTag {0};
  ctx.start([&ctx, &someTag](folly::Try<shared_ptr<RedisClient>> clientOpt) {
    auto clientPtr = clientOpt.value();
    clientPtr->set("saget", 0)
      .then([clientPtr, &ctx](try_response_t responseOpt) {
        EXPECT_TRUE(responseOpt.value().isType(
          RedisDynamicResponse::ResponseType::STATUS
        ));
        return clientPtr->incr("saget");
      })
      .then([clientPtr, &ctx](try_response_t responseOpt) {
        EXPECT_INT_RESPONSE(responseOpt, 1);
        return clientPtr->incrby("saget", 50);
      })
      .then([clientPtr, &ctx](try_response_t responseOpt) {
        EXPECT_INT_RESPONSE(responseOpt, 51);
        return clientPtr->decrby("saget", 25);
      })
      .then([clientPtr, &ctx](try_response_t responseOpt) {
        EXPECT_INT_RESPONSE(responseOpt, 26);
        return clientPtr->decr("saget");
      })
      .then([clientPtr, &ctx](try_response_t responseOpt) {
        EXPECT_INT_RESPONSE(responseOpt, 25);
      })
      .then([clientPtr, &ctx, &someTag]() {
        someTag.store(91);
        ctx.post();
      });
  });
  ctx.wait();
  EXPECT_EQ(91, someTag.load());
}



TEST(TestRedisIntegration, TestMSet1) {
  TestContext ctx;
  std::atomic<int> someTag {0};
  ctx.start([&ctx, &someTag](folly::Try<shared_ptr<RedisClient>> clientOpt) {
    auto clientPtr = clientOpt.value();
    clientPtr->mset({{"one", "aa"}, {"two", "bb"}, {"three", "cc"}})
      .then([clientPtr, &ctx](try_response_t responseOpt) {
        EXPECT_STATUS(responseOpt);
        return clientPtr->get("one");
      })
      .then([clientPtr, &ctx](try_response_t responseOpt) {
        EXPECT_STRING_RESPONSE(responseOpt, "aa");
        return clientPtr->get("two");
      })
      .then([clientPtr, &ctx](try_response_t responseOpt) {
        EXPECT_STRING_RESPONSE(responseOpt, "bb");
        return clientPtr->get("three");
      })
      .then([clientPtr, &ctx](try_response_t responseOpt) {
        EXPECT_STRING_RESPONSE(responseOpt, "cc");
      })
      .then([clientPtr, &ctx, &someTag]() {
        someTag.store(1523);
        ctx.post();
      });
  });
  ctx.wait();
  EXPECT_EQ(1523, someTag.load());
}

TEST(TestRedisIntegration, TestMSet2) {
  TestContext ctx;
  std::atomic<int> someTag {0};
  folly::Optional<folly::exception_wrapper> exn;
  ctx.start([&ctx, &someTag, &exn](folly::Try<shared_ptr<RedisClient>> clientOpt) {
    auto clientPtr = clientOpt.value();
    clientPtr->mset({{"one", "aa"}, {"two", "bb"}, {"three", "cc"}})
      .then([clientPtr, &ctx](try_response_t responseOpt) {
        EXPECT_STATUS(responseOpt);
        return clientPtr->mget({"one", "two", "three"});
      })
      .then([clientPtr, &ctx](try_response_t responseOpt) {
        EXPECT_TRUE(responseOpt.value().isType(ResponseType::ARRAY));
        auto asArray = responseOpt.value().getArray().value();
        std::vector<string> actual;
        for (auto item: asArray) {
          EXPECT_TRUE(item.isType(ResponseType::STRING));
          auto piece = item.getString().value();
          folly::fbstring itemStr {piece.start(), piece.size()};
          actual.push_back(itemStr.toStdString());
        }
        std::vector<string> expected {"aa", "bb", "cc"};
        EXPECT_EQ(expected, actual);
      })
      .then([clientPtr, &ctx, &someTag]() {
        someTag.store(2118);
        ctx.post();
      })
      .onError([&exn, &ctx, &someTag](std::exception &ex) {
        someTag.store(2118);
        exn.assign(ex);
        ctx.post();
      });
  });
  if (exn.hasValue()) {
    throw exn.value();
  }
  ctx.wait();
  EXPECT_EQ(2118, someTag.load());
}
