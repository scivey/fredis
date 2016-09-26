#include <string>
#include <memory>
#include <thread>
#include <folly/Baton.h>
#include <folly/io/async/EventBase.h>
#include <folly/futures/Try.h>
#include <glog/logging.h>

#include "fredis/folly_util/EBThread.h"
#include "fredis/RedisClient.h"

using fredis::folly_util::EBThread;
using namespace fredis;
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
      RedisClient::connect(ebt->getBase(), "127.0.0.1", 6379)
        .then(
          [&baton, &savedRef]
          (folly::Try<shared_ptr<RedisClient>> clientOpt) {
            LOG(INFO) << "connected... has val? : " << clientOpt.hasValue();
            shared_ptr<RedisClient> client = clientOpt.value();
            savedRef = client;
            client->set("foo", "123456789")
              .then([&baton](folly::Try<RedisResponse> response) {
                LOG(INFO) << "finished setting.";
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

