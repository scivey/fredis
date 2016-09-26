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
#include "fredis/memcached/MemcachedSyncClient.h"
#include "fredis/memcached/MemcachedConfig.h"
#include <libmemcached-1.0/memcached.h>

using fredis::folly_util::EBThread;
using namespace fredis::memcached;
using std::shared_ptr;
using std::thread;
using FBat = folly::Baton<std::atomic>;
using folly::fbstring;


int main() {
  google::InstallFailureSignalHandler();
  LOG(INFO) << "start";
  MemcachedConfig config {
    folly::SocketAddress("127.0.0.1", 11211)
  };
  MemcachedSyncClient client(std::move(config));
  client.connectExcept();
  CHECK(client.isConnected());
  auto result = client.get("foo");
  result.throwIfFailed();
  auto keyVal = result.value();
  if (!keyVal.hasValue()) {
    LOG(INFO) << "key was empty!";
  } else {
    fbstring kv = keyVal.value();
    LOG(INFO) << "result: '" << kv << "'";
  }
  LOG(INFO) << "end";
}
