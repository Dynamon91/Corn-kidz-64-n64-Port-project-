#include "ck64_save.h"
#include <string.h>

#define SW_BYTES 125u
#define IT_BYTES 125u
#define UP_BYTES 13u
#define HEADER CK64_SAVE_HEADER_SIZE
#define TOTAL CK64_SAVE_RECORD_SIZE

static const uint8_t magic[8] = {'C','K','6','4','S','A','V','E'};

static void put16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)v;
}
static void put32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}
static uint16_t get16(const uint8_t *p) {
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}
static uint32_t get32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}
static uint32_t checksum(const uint8_t *p, size_t n) {
    uint32_t crc = UINT32_MAX;
    for (size_t i=0;i<n;++i) {
        crc ^= (uint32_t)p[i];
        for (unsigned k=0;k<8u;++k)
            crc = (crc >> 1) ^ ((uint32_t)0xEDB88320u &
                                (uint32_t)(0u - (crc & 1u)));
    }
    return ~crc;
}
static bool bitmap_valid(const uint8_t *src, size_t n) {
    for (size_t i=0;i<n;++i) if (src[i] > 1u) return false;
    return true;
}
static void pack_bitmap(uint8_t *out, const uint8_t *in, size_t n) {
    size_t bytes=(n+7u)/8u;
    memset(out,0,bytes);
    for (size_t i=0;i<n;++i) {
        if (in[i]) out[i/8u] |= (uint8_t)(1u<<(i%8u));
    }
}
static void unpack_bitmap(uint8_t *out, const uint8_t *in, size_t n) {
    for (size_t i=0;i<n;++i)
        out[i] = (uint8_t)((in[i/8u]>>(i%8u))&1u);
}
static void pack_ints(uint8_t *dst, const int32_t *src, size_t n) {
    for (size_t i=0;i<n;++i) put32(dst+4u*i,(uint32_t)src[i]);
}
static void unpack_ints(int32_t *dst, const uint8_t *src, size_t n) {
    for (size_t i=0;i<n;++i) {
        uint32_t bits=get32(src+4u*i);
        /* Casting high-bit unsigned values to signed is implementation-defined.
           This conversion is arithmetic, avoids illegal signed overflow, and
           works the same on N64 and a host compiler. */
        if (bits<=INT32_MAX) dst[i]=(int32_t)bits;
        else dst[i]=-(int32_t)(UINT32_MAX-bits)-1;
    }
}
static uint16_t bool_flags(const CK64SaveData *d) {
    return (uint16_t)((uint16_t)d->bNonEmpty       << 0u |
                      (uint16_t)d->camInvertOff    << 1u |
                      (uint16_t)d->bRumble         << 2u |
                      (uint16_t)d->bSkipLauncher   << 3u |
                      (uint16_t)d->bFullscreen     << 4u |
                      (uint16_t)d->bWidescreen     << 5u |
                      (uint16_t)d->bVsync          << 6u |
                      (uint16_t)d->bRAnalogDpad    << 7u |
                      (uint16_t)d->bAnalogX2       << 8u |
                      (uint16_t)d->b1080           << 9u |
                      (uint16_t)d->bIgnoreDpadX    << 10u |
                      (uint16_t)d->bP1only         << 11u |
                      (uint16_t)d->bMusic          << 12u |
                      (uint16_t)d->bAchDisable     << 13u |
                      (uint16_t)d->b60fps          << 14u |
                      (uint16_t)d->bForce60hz      << 15u);
}
static void read_flags(CK64SaveData *d, uint16_t f) {
    d->bNonEmpty      =(f>>0u)&1u;
    d->camInvertOff   =(f>>1u)&1u;
    d->bRumble        =(f>>2u)&1u;
    d->bSkipLauncher  =(f>>3u)&1u;
    d->bFullscreen    =(f>>4u)&1u;
    d->bWidescreen    =(f>>5u)&1u;
    d->bVsync         =(f>>6u)&1u;
    d->bRAnalogDpad   =(f>>7u)&1u;
    d->bAnalogX2      =(f>>8u)&1u;
    d->b1080          =(f>>9u)&1u;
    d->bIgnoreDpadX   =(f>>10u)&1u;
    d->bP1only        =(f>>11u)&1u;
    d->bMusic         =(f>>12u)&1u;
    d->bAchDisable    =(f>>13u)&1u;
    d->b60fps         =(f>>14u)&1u;
    d->bForce60hz     =(f>>15u)&1u;
}
void ck64_save_init_fresh(CK64SaveData *d) {
    if (!d) return;
    memset(d,0,sizeof(*d));
    ck64_save_reset(d);
}
void ck64_save_reset(CK64SaveData *d) {
    if (!d) return;
    memset(d->switches,0,sizeof(d->switches));
    memset(d->items,0,sizeof(d->items));
    memset(d->upgrades,0,sizeof(d->upgrades));
    memset(d->ints,0,sizeof(d->ints));
    memset(d->cranks,0,sizeof(d->cranks));
    memset(d->lvlItems1,0,sizeof(d->lvlItems1));
    d->itemTotal=0;
    d->currentLvl=1;
    d->HPupTotal=0;
    d->screwTotal=0;
    d->maxHP=8;
    d->totalDeaths=0;
    d->gameTime=0;
    d->bNonEmpty=false;
    d->currentCamMode=0;
    d->camInvertOff=false;
    d->bRumble=false;
    d->bSkipLauncher=false;
    d->bWidescreen=false;
    d->clothes=0;
}

CK64SaveStatus ck64_save_encode(const CK64SaveData *d, uint8_t *out,
                               size_t capacity, size_t *written) {
    uint8_t *p;
    if (written) *written=0;
    if (!d||!out) return CK64_SAVE_INVALID_ARGUMENT;
    if (capacity<TOTAL) return CK64_SAVE_BUFFER_TOO_SMALL;
    if (!bitmap_valid(d->switches,CK64_SWITCH_COUNT) ||
        !bitmap_valid(d->items,CK64_ITEM_COUNT) ||
        !bitmap_valid(d->upgrades,CK64_UPGRADE_COUNT))
        return CK64_SAVE_INVALID_BOOLEAN;
    p=out+HEADER;
    pack_bitmap(p,d->switches,CK64_SWITCH_COUNT);p+=SW_BYTES;
    pack_bitmap(p,d->items,CK64_ITEM_COUNT);p+=IT_BYTES;
    pack_bitmap(p,d->upgrades,CK64_UPGRADE_COUNT);p+=UP_BYTES;
    pack_ints(p,d->ints,CK64_INT_ARRAY_COUNT);p+=4u*CK64_INT_ARRAY_COUNT;
    pack_ints(p,d->cranks,CK64_INT_ARRAY_COUNT);p+=4u*CK64_INT_ARRAY_COUNT;
    pack_ints(p,d->lvlItems1,CK64_INT_ARRAY_COUNT);p+=4u*CK64_INT_ARRAY_COUNT;
    const int32_t scalars[12]={
        d->itemTotal,d->currentLvl,d->HPupTotal,d->screwTotal,d->maxHP,
        d->totalDeaths,d->gameTime,d->currentCamMode,d->ctrlType,
        d->clothes,d->volume,d->antialias
    };
    pack_ints(p,scalars,12u);p+=48u;
    put16(p,bool_flags(d));p+=2u;
    /* Compile-independent length check, no pointer serialization. */
    if ((size_t)(p-(out+HEADER))!=CK64_SAVE_PAYLOAD_SIZE)
        return CK64_SAVE_BAD_LENGTH;
    memcpy(out,magic,sizeof(magic));
    put16(out+8u,CK64_SAVE_VERSION);
    put16(out+10u,0u);
    put32(out+12u,CK64_SAVE_PAYLOAD_SIZE);
    put32(out+16u,checksum(out+HEADER,CK64_SAVE_PAYLOAD_SIZE));
    if (written) *written=TOTAL;
    return CK64_SAVE_OK;
}

CK64SaveStatus ck64_save_decode(CK64SaveData *d,const uint8_t *in,size_t length) {
    CK64SaveData tmp;
    const uint8_t *p;
    if (!d||!in) return CK64_SAVE_INVALID_ARGUMENT;
    if (length!=TOTAL) return CK64_SAVE_BAD_LENGTH;
    if (memcmp(in,magic,sizeof(magic))!=0) return CK64_SAVE_INVALID_MAGIC;
    if (get16(in+8u)!=CK64_SAVE_VERSION) return CK64_SAVE_UNSUPPORTED_VERSION;
    if (get16(in+10u)!=0u || get32(in+12u)!=CK64_SAVE_PAYLOAD_SIZE)
        return CK64_SAVE_BAD_LENGTH;
    if (get32(in+16u)!=checksum(in+HEADER,CK64_SAVE_PAYLOAD_SIZE))
        return CK64_SAVE_BAD_CHECKSUM;
    /* The 100-upgrades bitmap contains four unused trailing bits. Reject
       them to guarantee canonical records and catch malformed encoders. */
    if ((in[HEADER+SW_BYTES+IT_BYTES+UP_BYTES-1u]&0xF0u)!=0u)
        return CK64_SAVE_NONCANONICAL;
    memset(&tmp,0,sizeof(tmp));
    p=in+HEADER;
    unpack_bitmap(tmp.switches,p,CK64_SWITCH_COUNT);p+=SW_BYTES;
    unpack_bitmap(tmp.items,p,CK64_ITEM_COUNT);p+=IT_BYTES;
    unpack_bitmap(tmp.upgrades,p,CK64_UPGRADE_COUNT);p+=UP_BYTES;
    unpack_ints(tmp.ints,p,CK64_INT_ARRAY_COUNT);p+=4u*CK64_INT_ARRAY_COUNT;
    unpack_ints(tmp.cranks,p,CK64_INT_ARRAY_COUNT);p+=4u*CK64_INT_ARRAY_COUNT;
    unpack_ints(tmp.lvlItems1,p,CK64_INT_ARRAY_COUNT);p+=4u*CK64_INT_ARRAY_COUNT;
    int32_t scalar[12];unpack_ints(scalar,p,12u);p+=48u;
    tmp.itemTotal=scalar[0];tmp.currentLvl=scalar[1];
    tmp.HPupTotal=scalar[2];tmp.screwTotal=scalar[3];
    tmp.maxHP=scalar[4];tmp.totalDeaths=scalar[5];tmp.gameTime=scalar[6];
    tmp.currentCamMode=scalar[7];tmp.ctrlType=scalar[8];
    tmp.clothes=scalar[9];tmp.volume=scalar[10];tmp.antialias=scalar[11];
    read_flags(&tmp,get16(p));p+=2u;
    if ((size_t)(p-(in+HEADER))!=CK64_SAVE_PAYLOAD_SIZE)
        return CK64_SAVE_BAD_LENGTH;
    /* Never modify caller state unless the full input validated. */
    *d=tmp;
    return CK64_SAVE_OK;
}
