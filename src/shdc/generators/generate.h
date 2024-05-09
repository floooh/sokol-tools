#pragma once
#include "types/gen_input.h"
#include "types/errmsg.h"
#include "types/format.h"

namespace shdc::gen {

ErrMsg generate(Format::Enum format, const GenInput& gen_input);

}
