#pragma once

#include "../../_config.h"
#include <stdexcept>

namespace BSML {
    /// A supplied BSML attribute or host value could not be parsed.
    class BSML_EXPORT ParseException : public std::runtime_error {
        public:
            using std::runtime_error::runtime_error;
    };
}
