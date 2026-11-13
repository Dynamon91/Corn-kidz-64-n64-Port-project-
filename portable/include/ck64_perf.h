#ifndef CK64_PERF_H
#define CK64_PERF_H
/* Phase 13 diagnostic timings. Not measurements of N64 RDP/RSP workload
 * or emulator real-time frame rate. Each stage captures one function call.
 */
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define CK64_PERF_MAX_US 99999u
typedef struct CK64PerfTimes {
    uint32_t get_us;    /* Call duration of display_get */
    uint32_t draw_us;   /* CPU software framebuffer drawing */
    uint32_t show_us;   /* Call duration of display_show */
    uint32_t total_us;  /* Display_get entry to display_show return */
    bool valid;         /* All four measures derived from monotonic ticks */
} CK64PerfTimes;

/* Inputs are five monotonic timer samples and a ticks/sec frequency.
 * Values saturate to 99999 us, never truncate. On invalid input,
 * returns false without changing *out. Does not allocate memory.
 */
bool ck64_perf_measure(CK64PerfTimes *out, uint64_t before_get,
                       uint64_t after_get, uint64_t after_draw,
                       uint64_t after_show, uint64_t ticks_per_second);
#ifdef __cplusplus
}
#endif
#endif
