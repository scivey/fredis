#include "fredis/redis/RedisDynamicResponse.h"
#include "fredis/redis/RedisError.h"
#include <folly/Format.h>
#include <hiredis/hiredis.h>

using namespace std;
using folly::Try;
using folly::StringPiece;
using folly::make_exception_wrapper;
namespace fredis { namespace redis {

using ResponseType = RedisDynamicResponse::ResponseType;

RedisDynamicResponse::RedisDynamicResponse(redisReply *hiredisRep)
  : hiredisReply_(hiredisRep) {}

RedisDynamicResponse::RedisDynamicResponse(const RedisDynamicResponse& other)
  : hiredisReply_(other.hiredisReply_) {}

RedisDynamicResponse& RedisDynamicResponse::operator=(
    const RedisDynamicResponse& other) {
  hiredisReply_ = other.hiredisReply_;
  return *this;
}

Try<ResponseType> RedisDynamicResponse::getType() const {
  DCHECK(!!hiredisReply_);
  return detail::responseTypeOfInt(hiredisReply_->type);
}

Try<const char*> RedisDynamicResponse::getTypeString() const {
  auto resType = getType();
  if (resType.hasValue()) {
    return Try<const char*> { detail::stringOfResponseType(resType.value()) };
  }
  std::string msg = resType.exception().what().toStdString();
  return Try<const char*> {
    make_exception_wrapper<RedisProtocolError>(msg)
  };
}

bool RedisDynamicResponse::isType(RedisDynamicResponse::ResponseType resType) const {
  auto hiType = getType();
  if (!hiType.hasValue()) {
    return false;
  }
  return hiType.value() == resType;
}

StringPiece RedisDynamicResponse::toStringPieceUnchecked() {
  return StringPiece(
    hiredisReply_->str, hiredisReply_->len
  );
}

RedisDynamicResponse::try_string_t RedisDynamicResponse::getString() {
  if (!isType(ResponseType::STRING)) {
    return try_string_t{make_exception_wrapper<RedisTypeError>(
      "Called getString() on a non-string response."
    )};
  }
  return try_string_t {toStringPieceUnchecked()};
}

RedisDynamicResponse::try_string_t RedisDynamicResponse::getStatusString() {
  if (!isType(ResponseType::STATUS)) {
    return try_string_t{make_exception_wrapper<RedisTypeError>(
      "Called getStatusString() on a non-status response."
    )};
  }
  return try_string_t {toStringPieceUnchecked()};
}

RedisDynamicResponse::try_string_t RedisDynamicResponse::getErrorString() {
  if (!isType(ResponseType::ERROR)) {
    return try_string_t{make_exception_wrapper<RedisTypeError>(
      "Called getErrorString() on a non-status response."
    )};
  }
  return try_string_t {toStringPieceUnchecked()};
}

RedisDynamicResponse::try_int_t RedisDynamicResponse::getInt() {
  if (!isType(ResponseType::INTEGER)) {
    return try_int_t{make_exception_wrapper<RedisTypeError>(
      "Called getInt() on a non-integer response."
    )};
  }
  return try_int_t {hiredisReply_->integer};
}

RedisDynamicResponse::try_array_t RedisDynamicResponse::getArray() {
  if (!isType(ResponseType::ARRAY)) {
    return try_array_t {make_exception_wrapper<RedisTypeError>(
      "Called getArray() on a non-array response."
    )};
  }
  folly::fbvector<RedisDynamicResponse> responses;
  responses.reserve(hiredisReply_->elements);
  auto currentElem = (redisReply**) hiredisReply_->element;
  for (size_t i = 0; i < hiredisReply_->elements; i++) {
    responses.push_back(RedisDynamicResponse(*currentElem));
    currentElem++;
  }
  return try_array_t {std::move(responses)};
}

void RedisDynamicResponse::pprintTo(std::ostream &oss) {
  if (isType(ResponseType::INTEGER)) {
    oss << "{ INTEGER: (" << getInt().value() << ")}";
  } else if (isType(ResponseType::STRING)) {
    oss << "{ STRING: '" << getString().value() << "'}";
  } else if (isType(ResponseType::STATUS)) {
    oss << "{ STATUS: '" << getStatusString().value() << "'}";
  } else if (isType(ResponseType::ERROR)) {
    oss << "{ ERROR: '" << getErrorString().value() << "'}";
  } else if (isType(ResponseType::NIL)) {
    oss << "{ NIL }";
  } else if (isType(ResponseType::ARRAY)) {
    oss << "{ ARRAY: [";
    for (auto child: getArray().value()) {
      oss << "\n\t";
      child.pprintTo(oss);
      oss << ",";
    }
    oss << "\n]}";
  } else {
    oss << "{ UKNOWN_TYPE [" << hiredisReply_->type << "] }";
  }
}


folly::fbstring RedisDynamicResponse::pprint() {
  std::ostringstream oss;
  pprintTo(oss);
  return oss.str();
}


namespace detail {
RedisDynamicResponse::ResponseType responseTypeOfIntExcept(int typeCode) {
  switch(typeCode) {
    case REDIS_REPLY_INTEGER:
      return RedisDynamicResponse::ResponseType::INTEGER;
    case REDIS_REPLY_STRING:
      return RedisDynamicResponse::ResponseType::STRING;
    case REDIS_REPLY_STATUS:
      return RedisDynamicResponse::ResponseType::STATUS;
    case REDIS_REPLY_ERROR:
      return RedisDynamicResponse::ResponseType::ERROR;
    case REDIS_REPLY_ARRAY:
      return RedisDynamicResponse::ResponseType::ARRAY;
    case REDIS_REPLY_NIL:
      return RedisDynamicResponse::ResponseType::NIL;
    default:
      throw RedisProtocolError(folly::sformat(
        "Unrecognized response type: {}", typeCode
      ));
  }
}

folly::Try<RedisDynamicResponse::ResponseType> responseTypeOfInt(int typeCode) {
  try {
    return folly::Try<RedisDynamicResponse::ResponseType>{
      responseTypeOfIntExcept(typeCode)
    };
  } catch (const RedisProtocolError &err) {
    return folly::Try<RedisDynamicResponse::ResponseType> {
      folly::make_exception_wrapper<RedisProtocolError>(err)
    };
  }
}

using response_type_map = std::map<
  RedisDynamicResponse::ResponseType, const char*
>;

using ResponseType = RedisDynamicResponse::ResponseType;

static const response_type_map typeNames {
  {ResponseType::STRING, "STRING"},
  {ResponseType::ARRAY, "ARRAY"},
  {ResponseType::STATUS, "STATUS"},
  {ResponseType::ERROR, "ERROR"},
  {ResponseType::INTEGER, "INTEGER"},
  {ResponseType::NIL, "NIL"}
};

const char* stringOfResponseType(ResponseType resType) {
  return typeNames.find(resType)->second;
}

} // detail


}} // fredis::redis

