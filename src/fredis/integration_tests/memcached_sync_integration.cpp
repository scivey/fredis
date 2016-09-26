#include <gtest/gtest.h>
#include "fredis/memcached/MemcachedSyncClient.h"
#include "fredis/memcached/MemcachedConfig.h"

using namespace fredis::memcached;
using namespace std;

#define FXSTR(x) #x

#define CHECK_SET(client, key, val) \
  do { \
    EXPECT_TRUE(client.isConnected()); \
    auto setResponse = client.set(key, val); \
    EXPECT_FALSE(setResponse.hasException()); \
  } while (0)

#define EXPECT_TO_GET(client, key, expected) \
  do { \
    EXPECT_TRUE(client.isConnected()); \
    auto getResponse = client.get(key); \
    EXPECT_FALSE(getResponse.hasException()); \
    std::string actual = getResponse.value().value().toStdString(); \
    EXPECT_EQ(expected, actual); \
  } while (0)


TEST(TestMemcachedSyncIntegration, TestSanity1) {
  MemcachedSyncClient client { MemcachedConfig {
    folly::SocketAddress("127.0.0.1", 11211)
  }};
  client.connectExcept();
  EXPECT_TRUE(client.isConnected());
  CHECK_SET(client, "foo", "f1");
  EXPECT_TO_GET(client, "foo", "f1");
  CHECK_SET(client, "bar", "b1");
  EXPECT_TO_GET(client, "bar", "b1");
  CHECK_SET(client, "foo", "f2");
  EXPECT_TO_GET(client, "foo", "f2");
  EXPECT_TO_GET(client, "bar", "b1");
  EXPECT_TO_GET(client, "foo", "f2");
}
