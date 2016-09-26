#pragma once
#include <folly/futures/Try.h>
#include <folly/futures/Unit.h>
#include <folly/Optional.h>
#include <folly/FBString.h>
#include "fredis/memcached/MemcachedConfig.h"

struct memcached_st;

namespace fredis { namespace memcached {

class MemcachedSyncClient {
 protected:
  MemcachedConfig config_;
  memcached_st* mcHandle_ {nullptr};
  MemcachedSyncClient(const MemcachedSyncClient&) = delete;
  MemcachedSyncClient& operator=(const MemcachedSyncClient&) = delete;

 public:
  MemcachedSyncClient();
  MemcachedSyncClient(const MemcachedConfig& config);
  MemcachedSyncClient(MemcachedConfig&& config);
  MemcachedSyncClient(MemcachedSyncClient&&);
  MemcachedSyncClient& operator=(MemcachedSyncClient&&);

  MemcachedConfig& getConfig();
  const MemcachedConfig& getConfig() const;

  folly::Try<folly::Unit> connect();
  void connectExcept();
  bool isConnected() const;

  using get_result_t = folly::Try<folly::Optional<folly::fbstring>>;
  get_result_t get(const folly::fbstring &key);

  using set_result_t = folly::Try<folly::Unit>;
  set_result_t set(const folly::fbstring &key, const folly::fbstring &val, time_t ttl = 0);
};

}} // fredis::memcached