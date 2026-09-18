#include <atomic>
// UnitTest_UniqueId.cpp
#include "UniqueId.h" // Assuming this is where UniqueId definitions reside
#include <cassert>   // For assert
#include <iostream>  // For cout

namespace UniqueId
{
    std::atomic<IdType> Generator::nextId{1};
}

void testSingleThreadedIdGeneration() {
    using namespace UniqueId;
    
    IdType id = Generator::getNextId();
    assert(id == 1 && "Initial ID should be 1");
    
    id = Generator::getNextId();
    assert(id == 2 && "Second call to getNextId should return 2");
}

void testConcurrentIdGeneration() {
    // This function would ideally use threading constructs and checks for thread safety.
    // Due to the lack of multi-threading support in UniqueId.h, this is a placeholder.
    std::cout << "Concurrent testing not implemented due to missing context." << std::endl;
}

std::atomic<unsigned long long> UniqueId::Generator::nextId = 1;
int main() {
    try {
        testSingleThreadedIdGeneration();
        // Uncomment the following line if you want to test concurrent scenarios, but ensure UniqueId is thread-safe.
        // testConcurrentIdGeneration();
        
        std::cout << "All tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
    }
    
    return 0;
}