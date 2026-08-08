#pragma once

#include "../../core/include/nocturne/sdk/nocturne_sdk_api.h"

namespace nocturne {

class NocturneSDK {
public:
    static constexpr const char* version() {
        return NOCTURNE_SDK_VERSION_STRING;
    }
};

} // namespace nocturne
