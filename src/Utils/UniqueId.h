#pragma once

#include <cstdint>
#include <atomic>

// - Unique ID system for game objects
namespace UniqueId
{
    using IdType = uint64_t;
    
    // Thread-safe unique ID generator
    class Generator
    {
    private:
        static std::atomic<IdType> nextId;
        
    public:
        static IdType generate()
        {
            return nextId.fetch_add(1, std::memory_order_relaxed);
        }
        
        // For save/load - set the next ID to resume from
        static void set_next_id(IdType id)
        {
            nextId.store(id, std::memory_order_release);
        }
        
        static IdType peek_next_id()
        {
            return nextId.load(std::memory_order_acquire);
        }
    };
    
    // Invalid/null ID constant
    constexpr IdType INVALID_ID = 0;
}
