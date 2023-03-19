#pragma once
#include <ArduinoJson.h>

struct PSRAMAllocator
{
    void *allocate(size_t size)
    {
        return heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
    }

    void deallocate(void *pointer)
    {
        heap_caps_free(pointer);
    }

    void *reallocate(void *pointer, size_t new_size)
    {
        return heap_caps_realloc(pointer, new_size, MALLOC_CAP_SPIRAM);
    }
};

using PSRAMJsonDocument = ArduinoJson::BasicJsonDocument<PSRAMAllocator>;