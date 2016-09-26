#pragma once

#include <string>
#include <folly/FBString.h>
#include <folly/FBVector.h>
#include <folly/Range.h>
#include <folly/futures/Try.h>

struct redisReply;

namespace fredis { namespace redis {

class RedisDynamicResponse {
 public:
  enum class ResponseType {
    STATUS, ERROR, INTEGER, NIL, STRING, ARRAY
  };
  using try_array_t = folly::Try<folly::fbvector<RedisDynamicResponse>>;
  using try_string_t = folly::Try<folly::StringPiece>;
  using try_int_t = folly::Try<int64_t>;
 protected:
  redisReply *hiredisReply_ {nullptr};
  folly::StringPiece toStringPieceUnchecked();
 public:
  RedisDynamicResponse(redisReply *redisRep);
  bool isType(ResponseType resType) const;
  bool isNil() const;
  try_string_t getString();
  try_string_t getStatusString();
  try_string_t getErrorString();
  try_int_t getInt();
  try_array_t getArray();

  void pprintTo(std::ostream &oss);
  folly::fbstring pprint();
};

namespace detail {
RedisDynamicResponse::ResponseType responseTypeOfIntExcept(int);
folly::Try<RedisDynamicResponse::ResponseType> responseTypeOfInt(int);
const char* stringOfResponseType(RedisDynamicResponse::ResponseType);
}

}} // fredis::redis
