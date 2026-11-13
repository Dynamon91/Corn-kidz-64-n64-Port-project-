#include "ck64_clock.h"
#include <limits.h>
#include <string.h>

CK64ClockStatus ck64_clock_init(CK64Clock *clock, uint64_t ticks_per_second) {
    if (!clock || ticks_per_second < 4u ||
        ticks_per_second > UINT64_MAX / CK64_CLOCK_UPDATE_HZ)
        return CK64_CLOCK_BAD_ARGUMENT;
    memset(clock, 0, sizeof(*clock));
    clock->ticks_per_second = ticks_per_second;
    return CK64_CLOCK_OK;
}

CK64ClockStatus ck64_clock_advance(CK64Clock *clock, uint64_t now,
                                  CK64ClockResult *result) {
    uint64_t elapsed;
    uint64_t step_delta;
    uint64_t scaled;
    CK64ClockResult next = {0};
    if (!clock || !result || clock->ticks_per_second < 4u ||
        clock->ticks_per_second > UINT64_MAX / CK64_CLOCK_UPDATE_HZ ||
        clock->phase >= clock->ticks_per_second)
        return CK64_CLOCK_BAD_ARGUMENT;
    if (!clock->primed) {
        clock->last_ticks = now;
        clock->primed = true;
        *result = next;
        return CK64_CLOCK_OK;
    }
    if (now < clock->last_ticks) return CK64_CLOCK_BACKWARDS;
    elapsed = now - clock->last_ticks;
    clock->last_ticks = now;
    step_delta = clock->ticks_per_second / 4u;
    if (elapsed > step_delta) {
        elapsed = step_delta;
        next.dropped_time = true;
    }
    /* phase < tps; elapsed <= tps/4, and valid tps ensures multiplication
     * itself fits. At large tick rates, addition could overflow, so divide
     * into quotient and remainder before accumulating. */
    scaled = elapsed * CK64_CLOCK_UPDATE_HZ;
    /* The tps bound above plus phase could still overflow at the extreme;
     * reconstitute from quotient/remainder without accumulating large values. */
    next.updates = (uint32_t)(scaled / clock->ticks_per_second);
    scaled %= clock->ticks_per_second;
    if (scaled >= clock->ticks_per_second - clock->phase) {
        ++next.updates;
        clock->phase = scaled - (clock->ticks_per_second - clock->phase);
    } else {
        clock->phase += scaled;
    }
    if (next.updates > CK64_CLOCK_MAX_STEPS) {
        next.updates = CK64_CLOCK_MAX_STEPS;
        next.dropped_time = true;
    }
    *result = next;
    return CK64_CLOCK_OK;
}
