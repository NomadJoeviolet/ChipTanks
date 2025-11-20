
#include "FreeRTOS.h"
#include "task.h" // pvPortMalloc and vPortFree are defined here
#include <stddef.h> // Use C header for size_t in embedded environments

/**
 * @brief Overload the global new operator to use FreeRTOS memory management.
 * @param size The size of the memory block to allocate.
 * @return A pointer to the allocated memory, or nullptr if the allocation fails.
 */
void* operator new(size_t size) {
    void* ptr = pvPortMalloc(size);
    return ptr;
}

/**
 * @brief Overload the global delete operator to use FreeRTOS memory management.
 * @param ptr A pointer to the memory block to deallocate.
 */
void operator delete(void* ptr) noexcept {
    if (ptr) {
        vPortFree(ptr);
    }
}

/**
 * @brief Overload the global array new operator.
 * @param size The size of the memory block to allocate for the array.
 * @return A pointer to the allocated memory, or nullptr if the allocation fails.
 */
void* operator new[](size_t size) {
    void* ptr = pvPortMalloc(size);
    return ptr;
}

/**
 * @brief Overload the global array delete operator.
 * @param ptr A pointer to the memory block to deallocate.
 */
void operator delete[](void* ptr) noexcept {
    if (ptr) {
        vPortFree(ptr);
    }
}

// Optional: Overloads for C++14 and later, which include a size parameter.
void operator delete(void* ptr, size_t size) noexcept {
    (void)size; // The size parameter is often unused in custom deallocations.
    if (ptr) {
        vPortFree(ptr);
    }
}

void operator delete[](void* ptr, size_t size) noexcept {
    (void)size;
    if (ptr) {
        vPortFree(ptr);
    }
}

