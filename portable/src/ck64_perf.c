#include "ck64_perf.h"
#include <stdint.h>

/* Whole seconds already exceed the 99999-us display limit.
 * For fractional seconds, bounds-check multiplication before dividing.
 * libdragon's timer frequency is well below this multiplication limit.
 */
static bool duration_us(uint64_t start, uint64_t end,
                        uint64_t frequency, uint32_t *result) {
    if (end < start || frequency == 0 || result == 0) return false;
    const uint64_t delta = end - start;
    if (delta >= frequency) {
        *result = CK64_PERF_MAX_US;
        return true;
    }
    if (delta > UINT64_MAX / UINT64_C(1000000)) return false;
    const uint64_t micros = (delta * UINT64_C(1000000)) / frequency;
    *result = micros > CK64_PERF_MAX_US ? CK64_PERF_MAX_US
                                        : (uint32_t)micros;
    return true;
}
bool ck64_perf_measure(CK64PerfTimes *out, uint64_t before_get,
                       uint64_t after_get, uint64_t after_draw,
                       uint64_t after_show, uint64_t ticks_per_second) {
    if (!out || ticks_per_second == 0 ||
        after_get < before_get || after_draw < after_get ||
        after_show < after_draw)
        return false;
    CK64PerfTimes next = {0};
    if (!duration_us(before_get, after_get, ticks_per_second, &next.get_us) ||
        !duration_us(after_get, after_draw, ticks_per_second, &next.draw_us) ||
        !duration_us(after_draw, after_show, ticks_per_second, &next.show_us) ||
        !duration_us(before_get, after_show, ticks_per_second, &next.total_us))
        return false;
    next.valid = true;
    *out = next;
    return true;
}
