#include <string>
#include <memory>
#include <thread>
#include <folly/Baton.h>
#include <folly/io/async/EventBase.h>
#include <folly/futures/Try.h>
#include <folly/FBString.h>
#include <folly/ExceptionWrapper.h>

#include <glog/logging.h>

#include "fredis/folly_util/EBThread.h"
#include "fredis/macros.h"
#include "fredis/FredisError.h"
#include "fredis/redis/RedisClient.h"

using fredis::folly_util::EBThread;
using namespace fredis::redis;
using namespace std;
using FBat = folly::Baton<std::atomic>;
using folly::fbstring;
using ResponseType = RedisDynamicResponse::ResponseType;

class SubHandler: public RedisSubscription::EventHandler {
 public:
  void onMessage(RedisDynamicResponse&& msg) override {
    LOG(INFO) << "message!";
    // RedisDynamicResponse m2 = std::move(msg);
    // auto strung = m2.pprint();
    if (msg.isType(ResponseType::ARRAY)) {
      LOG(INFO) << "ARRAY!";
      auto asArray = msg.getArray().value();
      LOG(INFO) << "n elems: " << asArray.size();
      for (auto& elem: asArray) {
        LOG(INFO) << "ELEM!";
        LOG(INFO) << elem.pprint();
      }
    }
  }
  void onStarted() override {
    LOG(INFO) << "onStarted!";
  }
  void onStopped() override {
    LOG(INFO) << "onStopped!";
  }
};

int main() {
  google::InstallFailureSignalHandler();
  LOG(INFO) << "start";
  auto ebt = EBThread::createShared();
  ebt->start();
  std::shared_ptr<RedisClient> ptrCopy;
  std::shared_ptr<RedisSubscription> subCopy;
  FBat baton;
  ebt->runInEventBaseThread([&baton, &ptrCopy, &ebt, &subCopy]() {
    auto client = RedisClient::createShared(
      ebt->getBase(), "127.0.0.1", 6379
    );
    ptrCopy = client;
    client->connect()
      .then([client, &subCopy]() {
        LOG(INFO) << "subscribe start!";
        auto subscriptionOpt = client->subscribe(
          std::unique_ptr<SubHandler>{new SubHandler},
          "chan1"
        );
        subscriptionOpt.throwIfFailed();
        subCopy = subscriptionOpt.value();
      })
      .onError([client, &baton](const std::exception &ex) {
        LOG(INFO) << "error! : '" << ex.what() << "'";
        baton.post();
      });
  });
  baton.wait();
  this_thread::sleep_for(chrono::milliseconds(60000));
  ebt->stop();
  ebt->join();
  LOG(INFO) << "end";
}
