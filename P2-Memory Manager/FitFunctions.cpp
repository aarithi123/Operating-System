#include "FitFunctions.h"
#include <cstdint> // For uint16_t

// setAllocator sets the allocator to use bestFit function
// find the smallest free partition that would fit the memory request size (sizeInWords)
int bestFit(int sizeInWords, void *list) {
    // Cast the list to a uint16_t pointer for processing
    uint16_t *memInfo = (uint16_t *)list;

    // Check if there are no available memory holes
    if (memInfo[0] == 0) {
        return -1; // No memory holes available
    }

    // Initialize variables for the best-fitting hole search
    int bestIndex = -1;
    uint16_t smallestHoleSize = UINT16_MAX;

    // Find the smallest suitable hole
    for (int currentIndex = 0; currentIndex < memInfo[0]; currentIndex++) {
        uint16_t currentHoleSize = memInfo[2 + currentIndex * 2];
        uint16_t currentHoleAddress = memInfo[1 + currentIndex * 2];

        // Check if the current hole fits the size requirements and is smaller than the best so far
        if (currentHoleSize >= sizeInWords &&
            (bestIndex == -1 || currentHoleSize < smallestHoleSize)) {
            bestIndex = currentIndex;
            smallestHoleSize = currentHoleSize;
        }
    }

    // If no suitable hole was found, return -1
    if (bestIndex == -1) {
        return -1;
    }

    // Return the address of the best-fit hole
    return memInfo[1 + bestIndex * 2];
}

// setAllocator sets the allocator to use worstFit function
// find the largest free partition
/*
int worstFit(int sizeInWords, void *list) {
    uint16_t *memoryData = (uint16_t *)list;

    // If no memory holes are available, return -1 
    if (memoryData[0] == 0) { return -1; }

    // Find the index of the largest hole to fit sizeInWords
    int indexOfWorstHole = 0;
    for (int i = 1; i < memoryData[0]; i++) {
        if (memoryData[2 + 2 * i] >= memoryData[2 + 2 * indexOfWorstHole]) {
            indexOfWorstHole = i;
        }
    }

    // If no hole meets the sizeInWords requirement, return -1
    if (memoryData[2 + 2 * indexOfWorstHole] < sizeInWords) { return -1; }

    // Return the address of the worst-fit hole
    return memoryData[1 + 2 * indexOfWorstHole];
}
*/

int worstFit(int sizeInWords, void *list) {
    // Cast the input list to an array of uint16_t integers
    uint16_t *memInfo = (uint16_t *)list;

    // Check if there are any memory holes available
    int numHoles = memInfo[0];
    if (numHoles == 0) {
        return -1; // No memory holes available
    }

    // Initialize variables to track the largest available hole
    int largestHoleIndex = -1;
    uint16_t largestHoleSize = 0;

    // Iterate through all memory holes
    for (int currentIndex = 0; currentIndex < numHoles; ++currentIndex) {
        uint16_t currentHoleSize = memInfo[2 + 2 * currentIndex];

        // If current hole is larger than the largest found so far, update the tracking variables
        if (currentHoleSize >= sizeInWords && currentHoleSize > largestHoleSize) {
            largestHoleIndex = currentIndex;
            largestHoleSize = currentHoleSize;
        }
    }

    // If no suitable hole is found that can fit sizeInWords, return -1
    if (largestHoleIndex == -1) {
        return -1;
    }

    // Return the starting address of the largest hole (worst-fit)
    return memInfo[1 + 2 * largestHoleIndex];
}