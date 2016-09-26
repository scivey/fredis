#include "fredis/memcached/MemcachedConfig.h"
#include "fredis/memcached/MemcachedSyncClient.h"
#include "fredis/memcached/MemcachedError.h"
#include <libmemcached/memcached.h>
#include <folly/ScopeGuard.h>
#include <folly/ExceptionWrapper.h>
#include <glog/logging.h>

using folly::Try;
using folly::Unit;
using folly::fbstring;
using folly::make_exception_wrapper;
using folly::makeGuard;

using namespace std;

namespace fredis { namespace memcached {

MemcachedSyncClient::MemcachedSyncClient(const MemcachedConfig& config)
  : config_(config){}

MemcachedSyncClient::MemcachedSyncClient(MemcachedConfig&& config)
  : config_(std::forward<MemcachedConfig>(config)){}

MemcachedSyncClient::MemcachedSyncClient(MemcachedSyncClient&& other)
  : config_(other.config_), mcHandle_(other.mcHandle_) {
  other.mcHandle_ = nullptr;
}

MemcachedSyncClient& MemcachedSyncClient::operator=(
    MemcachedSyncClient &&other) {
  std::swap(config_, other.config_);
  std::swap(mcHandle_, other.mcHandle_);
  return *this;
}

Try<Unit> MemcachedSyncClient::connect() {
  if (isConnected()) {
    return Try<Unit>{
      make_exception_wrapper<AlreadyConnected>(
        "memcached client already connected"
      )
    };
  }
  if (!config_.hasAnyServers()) {
    return Try<Unit>{
      make_exception_wrapper<ConfigurationError>(
        "No servers are configured."
      )
    };
  }
  auto confStrOpt = config_.toConfigString();
  if (confStrOpt.hasException()) {
    return Try<Unit>{std::move(confStrOpt.exception())};
  }
  mcHandle_ = ::memcached(
    confStrOpt.value().c_str(),
    confStrOpt.value().size()
  );
  if (!mcHandle_) {
    return Try<Unit> {
      make_exception_wrapper<ConnectionError>(
        "libmemcached's memcached() function "
        "returned a nullptr with no explanation."
      )
    };
  }
  if (memcached_last_error(mcHandle_) != MEMCACHED_SUCCESS) {
    Try<Unit> result {
      make_exception_wrapper<ConnectionError>(
        memcached_last_error_message(mcHandle_)
      )
    };
    free(mcHandle_);
    mcHandle_ = nullptr;
    return result;
  }
  return Try<Unit>{Unit{}};
}

bool MemcachedSyncClient::isConnected() const {
  return !!mcHandle_;
}

void MemcachedSyncClient::connectExcept() {
  connect().throwIfFailed();
}


MemcachedConfig& MemcachedSyncClient::getConfig() {
  return config_;
}

const MemcachedConfig& MemcachedSyncClient::getConfig() const {
  return config_;
}

using get_result_t = MemcachedSyncClient::get_result_t;

get_result_t MemcachedSyncClient::get(const fbstring &key) {
  DCHECK(isConnected());
  memcached_return_t errCode;
  size_t valLen {0};
  uint32_t flags {0};
  auto valBuff = memcached_get(mcHandle_, key.c_str(), key.size(),
    &valLen, &flags, &errCode);
  auto guard = folly::makeGuard([&valBuff]() {
    if (valBuff != nullptr) {
      free(valBuff);
    }
  });
  get_result_t result;
  if (errCode != MEMCACHED_SUCCESS) {
    result = get_result_t {make_exception_wrapper<ProtocolError>(
      memcached_last_error_message(mcHandle_)
    )};
  }
  if (valLen == 0) {
    result = get_result_t {folly::Optional<fbstring>{}};
  } else {
    result = get_result_t {folly::Optional<fbstring>{
      fbstring{valBuff, valLen}
    }};
  }
  return result;
}

}} // fredis::memcached