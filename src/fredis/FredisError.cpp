#include "fredis/FredisError.h"
#include <string>

namespace fredis {

FredisError::FredisError(const char* msg)
  : std::runtime_error(std::string(msg)) {}

}
