#include "ck64_perf.h"
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void test_separate_stages(void) {
    CK64PerfTimes p = {0};
    const uint64_t base = 1000000;
    const uint64_t f = 46875000; /* libdragon ticks/second */
    assert(ck64_perf_measure(&p,base,
                            base+f/1000,
                            base+f/1000+8*f/1000,
                            base+f/1000+8*f/1000+3*f/1000,f));
    assert(p.valid && p.get_us==1000 && p.draw_us==8000 &&
           p.show_us==3000 && p.total_us==12000);
}
static void test_zero_duration_and_saturation(void) {
    CK64PerfTimes p = {0};
    assert(ck64_perf_measure(&p,0,0,0,0,46875000));
    assert(p.valid && p.get_us==0 && p.draw_us==0 &&
           p.show_us==0 && p.total_us==0);
    assert(ck64_perf_measure(&p,100,46975100,46975100,46975100,
                             46875000));
    assert(p.get_us==CK64_PERF_MAX_US);
    assert(p.total_us==CK64_PERF_MAX_US);
}
static void test_invalid_inputs_do_not_mutate(void) {
    CK64PerfTimes p={12,34,56,78,true};
    CK64PerfTimes before=p;
    assert(!ck64_perf_measure(NULL,1,2,3,4,46875000));
    assert(!ck64_perf_measure(&p,2,1,3,4,46875000));
    assert(!ck64_perf_measure(&p,1,3,2,4,46875000));
    assert(!ck64_perf_measure(&p,1,2,4,3,46875000));
    assert(!ck64_perf_measure(&p,1,2,3,4,0));
    assert(!ck64_perf_measure(&p,0,UINT64_MAX/2,UINT64_MAX/2,
                              UINT64_MAX/2,UINT64_MAX));
    assert(memcmp(&p,&before,sizeof p)==0);
}
int main(void) {
    test_separate_stages();
    test_zero_duration_and_saturation();
    test_invalid_inputs_do_not_mutate();
    puts("Phase 13 timer segmentation tests passed");
    return 0;
}
