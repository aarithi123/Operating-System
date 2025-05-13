#pragma once

#include <functional>
#include <vector>
#include "FitFunctions.h"

// MemoryManager
class MemoryManager {
public:
    MemoryManager(unsigned wordSize, std::function<int(int, void *)> allocator);
    ~MemoryManager();
    void initialize(size_t sizeInWords);
    void shutdown();
    void *allocate(size_t sizeInBytes);
    void free(void *address);
    void setAllocator(std::function<int(int, void *)> allocator);
    int dumpMemoryMap(char *fileName);
    void *getList();
    void *getBitmap();
    unsigned getWordSize();
    void *getMemoryStart();
    unsigned getMemoryLimit();

private:
    unsigned int wordSize;
    size_t sizeInWords;                          // Total words allocated
    void *memoryStart;                           // Start of allocated memory
    std::vector<std::vector<int>> memoryTracker; // Tracking allocation details
    std::function<int(int, void *)> allocator;   // Memory Allocator
};
