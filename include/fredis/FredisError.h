#pragma once
#include <stdexcept>
#include <string>
#include "fredis/macros.h"

namespace fredis {

class FredisError: public std::runtime_error {
 public:
  template<typename T>
  FredisError(const T& msg): std::runtime_error(msg) {}
  FredisError(const char*);
};

}