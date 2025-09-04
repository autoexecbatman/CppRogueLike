// UniqueId.cpp - Implementation of unique ID system
#include "UniqueId.h"

namespace UniqueId
{
    // Start IDs at 1 (0 is reserved for INVALID_ID)
    std::atomic<IdType> Generator::nextId{1};
}
