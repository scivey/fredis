#pragma once
#include <stdexcept>
#include "fredis/macros.h"

namespace fredis {

FREDIS_DECLARE_EXCEPTION(FredisError, std::runtime_error);

}