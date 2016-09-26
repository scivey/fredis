#include <string>
#include <memory>
#include <thread>
#include <folly/Baton.h>
#include <folly/io/async/EventBase.h>
#include <folly/futures/Try.h>
#include <glog/logging.h>

#include "fredis/folly_util/EBThread.h"
#include "fredis/redis/RedisClient.h"

using fredis::folly_util::EBThread;
using namespace fredis::redis;
using std::shared_ptr;
using std::thread;
using FBat = folly::Baton<std::atomic>;

int main() {
  google::InstallFailureSignalHandler();
  LOG(INFO) << "start";
  auto ebt = EBThread::createShared();
  ebt->start();
  FBat baton;
  std::shared_ptr<RedisClient> savedRef {nullptr};
  ebt->runInEventBaseThread(
    [ebt, &baton, &savedRef]
    () {
      auto clientPtr = RedisClient::createShared(ebt->getBase(), "127.0.0.1", 6379);
      savedRef = clientPtr;
      clientPtr->connect()
        .then(
          [&baton, clientPtr]
          (folly::Try<shared_ptr<RedisClient>> clientOpt) {
            LOG(INFO) << "connected... has val? : " << clientOpt.hasValue();
            shared_ptr<RedisClient> client = clientOpt.value();
            client->set("foo", "123456789")
              .then([&baton, clientPtr](folly::Try<RedisDynamicResponse> response) {
                LOG(INFO) << "finished setting.";
                LOG(INFO) << "result : ...";
                if (!response.hasValue()) {
                  LOG (INFO) << "exception! : " << response.exception().what();
                } else {
                  LOG(INFO) << response.value().pprint();
                }
                baton.post();
              });
          }
        );
    }
  );
  baton.wait();
  ebt->stop();
  ebt->join();
  LOG(INFO) << "end";
}

