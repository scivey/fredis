#include "fredis/memcached/MemcachedConfig.h"
#include "fredis/memcached/MemcachedError.h"
#include <libmemcached/memcached.h>

using folly::Try;
using folly::Unit;
using folly::fbstring;
using std::ostringstream;
using folly::make_exception_wrapper;

namespace fredis { namespace memcached {

MemcachedConfig::MemcachedConfig(){}

MemcachedConfig::MemcachedConfig(MemcachedConfig::server_init_list&& servers)
  : serverHosts_(std::forward<MemcachedConfig::server_init_list>(servers)){}

void MemcachedConfig::addServers(MemcachedConfig::server_init_list&& servers) {
  for (auto &&server: servers) {
    serverHosts_.push_back(server);
  }
}

Try<fbstring> MemcachedConfig::toConfigString() {
  std::ostringstream oss;
  size_t lastIdx = serverHosts_.size();
  if (lastIdx > 0) {
    lastIdx--;
  }
  size_t idx = 0;
  for (auto &server: serverHosts_) {
    oss << "--SERVER=" << server.getAddressStr() << ":" << server.getPort();
    if (idx < lastIdx) {
      oss << " ";
    }
    idx++;
  }
  fbstring confStr = oss.str();
  auto err = detail::validateMemcachedConfigStr(confStr);
  if (err.hasException()) {
    return err;
    // return Try<fbstring>{
    //   make_exception_wrapper<MemcachedError>(
    //     std::move(err)
    //   )
    // };
  }
  return Try<fbstring>{confStr};
}

bool MemcachedConfig::hasAnyServers() const {
  return serverHosts_.size() > 0;
}

namespace detail {

Try<fbstring> validateMemcachedConfigStr(const fbstring& configStr) {
  char errBuff[512];
  auto rc = libmemcached_check_configuration(configStr.c_str(),
    configStr.size(), (char*) errBuff, sizeof(errBuff));
  if (memcached_failed(rc)) {
    std::ostringstream msg;
    msg << "Configuration Error: '" << errBuff << "'";
    return folly::Try<fbstring>{
      make_exception_wrapper<ConfigurationError>(msg.str())
    };
  }
  return Try<fbstring>{fbstring{configStr}};
}

void validateMemcachedConfigStrExcept(const fbstring& configStr) {
  auto result = validateMemcachedConfigStr(configStr);
  result.throwIfFailed();
}

} // detail

}} // fredis::memcached

