#include <stdint.h>



typedef uint16_t module_counter_t;
#define MAX_MODULES ((1ULL << sizeof(module_counter_t) * 8) - 1)

typedef uint16_t thread_counter_t;
#define MAX_THREADS ((1ULL << sizeof(thread_counter_t) * 8) - 1)