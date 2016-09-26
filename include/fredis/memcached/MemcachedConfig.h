#pragma once
#include <initializer_list>
#include <folly/futures/Try.h>
#include <folly/futures/Unit.h>
#include <folly/FBVector.h>
#include <folly/FBString.h>
#include <folly/SocketAddress.h>


namespace fredis { namespace memcached {


class MemcachedConfig {
 protected:
  folly::fbvector<folly::SocketAddress> serverHosts_;
 public:
  using server_init_list = std::initializer_list<folly::SocketAddress>;
  MemcachedConfig();

  MemcachedConfig(server_init_list&& servers);

  template<typename TCollection>
  MemcachedConfig(const TCollection &servers) {
    addServers(servers);
  }

  void addServers(server_init_list&& servers);

  template<typename TCollection>
  void addServers(const TCollection &servers) {
    for (const folly::SocketAddress &sock: servers) {
      serverHosts_.push_back(sock);
    }
  }

  bool hasAnyServers() const;
  folly::Try<folly::fbstring> toConfigString();
};

namespace detail {
folly::Try<folly::fbstring> validateMemcachedConfigStr(const folly::fbstring&);
}

}} // fredis::memcached

