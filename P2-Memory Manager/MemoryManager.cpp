#include "MemoryManager.h"
#include "FitFunctions.h"
#include <vector>
#include <functional>
#include <utility>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>  // mmap
#include <cstring>     // memset
#include <cmath>       // ceil
#include <iostream>
#include <algorithm>

using namespace std;


// constructor
// initial state of MemoryManager instance
MemoryManager::MemoryManager(unsigned wordSize, function<int(int, void *)> allocator)
    : wordSize(wordSize), 
      allocator(move(allocator)), 
      memoryStart(nullptr), 
      sizeInWords(0) {}

// destructor
// cleaning up memory by calling shutdown 
MemoryManager::~MemoryManager() {
    shutdown(); // Ensure proper cleanup
}


// Instantiates block of requested size, no larger than 65536 words; cleans up previous block if applicable
void MemoryManager::initialize(size_t requestedSize) {
    const size_t MAX_MEMORY_SIZE = 65536;  // Maximum allowable memory size in words

    // First, check if the requested size exceeds the maximum allowable size
    if (requestedSize > MAX_MEMORY_SIZE) {
        cerr << "Error: Memory size exceeds " << MAX_MEMORY_SIZE << " words." << endl;
        return;
    }

    // Release any previously allocated memory if it exists
    if (memoryStart != nullptr) {
        shutdown();  // shutdown() method should handle memory deallocation
    }

    // Calculate the size in bytes for mmap
    size_t totalSizeInBytes = requestedSize * wordSize;

    // Attempt to allocate memory using mmap
    void* allocatedMemory = mmap(nullptr, totalSizeInBytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (allocatedMemory == MAP_FAILED) {
        cerr << "Error: Memory allocation failed." << endl;
        memoryStart = nullptr;  // Ensure memoryStart is null after a failed allocation
        return;
    }

    // If mmap succeeds, update memoryStart and sizeInWords
    memoryStart = allocatedMemory;
    this->sizeInWords = requestedSize;

    // Initialize memory to zero
    memset(memoryStart, 0, totalSizeInBytes);

    // Reset the memory tracker, ready for new allocations
    memoryTracker.clear();
}


void MemoryManager::shutdown() {
    // check for allocated memory  and release them using munmap. After release, set memoryStart to nullptr to avoid dangling pointers scenario.
    if (memoryStart != nullptr) {
        munmap(memoryStart, sizeInWords * wordSize);
    }
    memoryStart = nullptr;
}

void *MemoryManager::allocate(size_t sizeInBytes) {
    // Check initial conditions: if no memory is initialized or the request exceeds available memory.
    if (memoryStart == nullptr || sizeInBytes > sizeInWords * wordSize) {
        return nullptr;
    }

    // Calculate the number of words needed, rounding up.
    int requiredWords = static_cast<int>((sizeInBytes + wordSize - 1) / wordSize);

    // Retrieve and process the free list only if necessary.
    void *freeList = getList();    // Get the list of available free memory blocks
    if (!freeList) {
        return nullptr;            // Fail allocation if we can't get a free list (though this condition is theoretical)
    }

    // Attempt to allocate memory using the allocator function.
    int allocationStart = allocator(requiredWords, freeList);

    // Immediately clean up the free list array to prevent memory leaks.
    delete[] static_cast<uint16_t *>(freeList);

    // Check if the allocation was successful.
    if (allocationStart == -1) {
        return nullptr;           // Allocation failed
    }

    // Calculate the start address of the allocated block.
    char *allocatedBlock = static_cast<char *>(memoryStart) + allocationStart * wordSize;

    // Mark the memory as used by setting it to a specific pattern, here using 0xFF.
    memset(allocatedBlock, 0xFF, requiredWords * wordSize);

    // Track the memory usage.
    memoryTracker.emplace_back(vector<int>{allocationStart, requiredWords});

    return allocatedBlock;
}

void MemoryManager::free(void *address) {
    if (memoryStart == nullptr || address == nullptr) {
        return;
    }

    ptrdiff_t offset = static_cast<char *>(address) - static_cast<char *>(memoryStart);

    for (auto it = memoryTracker.begin(); it != memoryTracker.end(); ++it) {
        ptrdiff_t blockStart = (*it)[0] * wordSize;
        if (blockStart == offset) {
            memset(address, 0, (*it)[1] * wordSize);
            memoryTracker.erase(it);
            break;
        }
    }
}


// setAllocator function allows to use custom memory allocation algorithms, such as bestFit or worstFit.
// It enables allocator to call betsFit and worstFit functions indirectly during memory allocation.
// setAllocator gets called by allocator function 
void MemoryManager::setAllocator(function<int(int, void *)> allocator) {                                                                                                                           
    this->allocator = move(allocator);                                                                                                                                                             
}                                                                                                                                                                                                       
   
// write a text representation of the free memory blocks
int MemoryManager::dumpMemoryMap(char *fileName) {
    // Ensure the memory is initialized.
    if (memoryStart == nullptr) {
        return -1;
    }

    // Open the file with read, write permissions; create if not exists; truncate if exists.
    int fileDescriptor = open(fileName, O_RDWR | O_CREAT | O_TRUNC, 0777);
    if (fileDescriptor == -1) {
        perror("Failed to open file");
        return -1;
    }

    // Get the list of free memory blocks.
    uint16_t *freeList = static_cast<uint16_t *>(getList());
    if (freeList == nullptr) {
        close(fileDescriptor);
        return -1;
    }

    // Check if there are any free blocks to write.
    if (freeList[0] > 0) {
        string buffer;
        for (int i = 0; i < freeList[0]; ++i) {
            // Format each memory block's information.
            int index = 2 * i + 1;
            buffer += "[" + to_string(freeList[index]) + ", " + to_string(freeList[index + 1]) + "]";
            if (i < freeList[0] - 1) {
                buffer += " - ";
            }
        }

        // Write the formatted string to the file.
        ssize_t writeResult = write(fileDescriptor, buffer.c_str(), buffer.size());
        if (writeResult == -1) {
            perror("Failed to write to file");
            delete[] freeList;
            close(fileDescriptor);
            return -1;
        }
    }

    // Clean up
    delete[] freeList;
    close(fileDescriptor);

    // Return the file descriptor (could instead return 0 for success).
    return 0;  // Indicating success.
}


// returns list of available free memory holes
void *MemoryManager::getList() {
    // Return null if the memory has not been initialized.
    if (memoryStart == nullptr) {
        return nullptr;
    }

    // Using a vector to track the free memory blocks.
    vector<uint16_t> freeMemoryBlocks;
    freeMemoryBlocks.reserve(sizeInWords);  // Optimizing for possible capacity.
    freeMemoryBlocks.push_back(0);          // Initialize the count of free blocks to 0.

    // Iterate through the memory, checking for free memory holes.
    bool isPreviousBlockAllocated = true;
    for (size_t i = 0; i < sizeInWords; i++) {
        unsigned char *currentBlock = static_cast<unsigned char *>(memoryStart) + i * wordSize;
        bool isCurrentBlockFree = (*currentBlock == 0);

        if (isPreviousBlockAllocated && isCurrentBlockFree) {
            // Start of a new free block.
            freeMemoryBlocks.push_back(static_cast<uint16_t>(i)); // Start index.
            freeMemoryBlocks.push_back(1);                        // Length.
            freeMemoryBlocks[0]++;                                 // Incrementing the count of free blocks.
        } else if (!isPreviousBlockAllocated && isCurrentBlockFree) {
            // Extending the current free block.
            freeMemoryBlocks.back()++;
        }

        isPreviousBlockAllocated = !isCurrentBlockFree;
    }

    // Create a dynamic array to return.
    uint16_t *freeList = new uint16_t[freeMemoryBlocks.size()];
    copy(freeMemoryBlocks.begin(), freeMemoryBlocks.end(), freeList);

    return freeList;
}


// generate a bitmap representing memory indicating  whether each memory block (word) is free (0) or allcated (1)
void *MemoryManager::getBitmap() {
    // if memory block has not been initialized, no need to generate bitmap
    if (memoryStart == nullptr) {
        return nullptr;
    }

    // bitmap size (rounded). Each bit in the map is a word of 8 bits
    int bitmapSize = static_cast<int>(ceil(static_cast<double>(sizeInWords) / 8.0));
    char *bitmap = new char[bitmapSize + 2];          // the extra two bytes to store the size of the bitmap 
    bitmap[0] = static_cast<char>(bitmapSize);
    bitmap[1] = static_cast<char>(bitmapSize >> 8);

    // build the bitmap
    int i = 0;  // Initialize index for the outer loop

    while (i < bitmapSize) {                          // Outer loop to iterate through each byte/word in the memory block
        char byte = 0;
        int bitIndex = 7;                             // Initialize the bitIndex for the inner loop
        while (bitIndex >= 0) {                       // Inner loop to iterate through each bit
            byte <<= 1;
            if (static_cast<char *>(memoryStart)[(i * 8 + bitIndex) * wordSize] != 0) {
                byte += 1;
            }
            bitIndex--;                               // Decrement bitIndex
        }
        bitmap[i + 2] = byte;                         // Set the computed byte into the bitmap
        i++;                                          // Increment the outer loop index
    }
    return bitmap;
}

unsigned MemoryManager::getWordSize() { 
    return wordSize; 
}

void *MemoryManager::getMemoryStart() { 
    return memoryStart; 
}

unsigned MemoryManager::getMemoryLimit() { 
    return sizeInWords * wordSize; 
}
