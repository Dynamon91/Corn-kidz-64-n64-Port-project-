#ifndef CK64_SAVE_H
#define CK64_SAVE_H
/* Portable reconstructed Data snapshot and platform-independent on-media format.
 * Source field metadata from Corn Kidz 64 v1.029 Assembly-CSharp.dll.
 * This is NOT the original Unity BinaryFormatter save1.cow format.
 * No IO, dynamic allocation, Unity, libdragon, or host-endianness assumptions.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    CK64_SWITCH_COUNT = 1000,
    CK64_ITEM_COUNT = 1000,
    CK64_UPGRADE_COUNT = 100,
    CK64_INT_ARRAY_COUNT = 100,
    CK64_SAVE_HEADER_SIZE = 20,
    CK64_SAVE_PAYLOAD_SIZE = 1513,
    CK64_SAVE_RECORD_SIZE = CK64_SAVE_HEADER_SIZE + CK64_SAVE_PAYLOAD_SIZE,
    CK64_SAVE_VERSION = 1
};

/* Names, scalars and types map to the 34 fields of the original Data class.
 * Each original System.Boolean[] is stored as uint8_t (0 or 1) in RAM;
 * on-media arrays are packed into bits. These RAM fields are not serialized
 * by writing a raw struct image.
 */
typedef struct CK64SaveData {
    uint8_t switches[CK64_SWITCH_COUNT];
    uint8_t items[CK64_ITEM_COUNT];
    uint8_t upgrades[CK64_UPGRADE_COUNT];
    int32_t ints[CK64_INT_ARRAY_COUNT];
    int32_t cranks[CK64_INT_ARRAY_COUNT];
    int32_t lvlItems1[CK64_INT_ARRAY_COUNT];
    int32_t itemTotal;
    int32_t currentLvl;
    int32_t HPupTotal;
    int32_t screwTotal;
    int32_t maxHP;
    int32_t totalDeaths;
    int32_t gameTime;
    bool bNonEmpty;
    int32_t currentCamMode;
    int32_t ctrlType;
    bool camInvertOff;
    bool bRumble;
    bool bSkipLauncher;
    bool bFullscreen;
    bool bWidescreen;
    int32_t clothes;
    bool bVsync;
    bool bRAnalogDpad;
    bool bAnalogX2;
    bool b1080;
    bool bIgnoreDpadX;
    bool bP1only;
    int32_t volume;
    int32_t antialias;
    bool bMusic;
    bool bAchDisable;
    bool b60fps;
    bool bForce60hz;
} CK64SaveData;

typedef enum CK64SaveStatus {
    CK64_SAVE_OK = 0,
    CK64_SAVE_INVALID_ARGUMENT,
    CK64_SAVE_BUFFER_TOO_SMALL,
    CK64_SAVE_INVALID_MAGIC,
    CK64_SAVE_UNSUPPORTED_VERSION,
    CK64_SAVE_BAD_LENGTH,
    CK64_SAVE_BAD_CHECKSUM,
    CK64_SAVE_NONCANONICAL,
    CK64_SAVE_INVALID_BOOLEAN
} CK64SaveStatus;

/* Initializes a freshly allocated Data-equivalent object with CLR zero values,
 * then applies GameCtrl.ResetSav. Do NOT assume Unity inspector defaults.
 */
void ck64_save_init_fresh(CK64SaveData *data);
/* Models only the fields ACTUALLY assigned by GameCtrl.ResetSav. It resets
 * the six arrays, 14 scalars, and PRESERVES the other 14 scalar fields such
 * as volume, bMusic, controller settings, and display settings.
 */
void ck64_save_reset(CK64SaveData *data);

/* On-media v1, exact 1533 bytes:
 * 0..7 magic "CK64SAVE", 8..9 version (BE u16), 10..11 flags/reserved (=0),
 * 12..15 payload size (BE u32), 16..19 CRC32/ISO-HDLC of payload (BE u32).
 * Payload: switches 125 bytes, items 125, upgrades 13, ints/cranks/lvlItems1
 * each 100 x big-endian i32, 12 scalar i32 in the exact order in the source,
 * followed by 16 scalar bools in a BE u16 (bit0 is bNonEmpty). Bitmap arrays
 * use ascending indices, bit0 = element index 0. Unused upgrade bits must 0.
 * Device IO is intentionally OUTSIDE this code.
 */
CK64SaveStatus ck64_save_encode(const CK64SaveData *data,
                               uint8_t *output, size_t capacity,
                               size_t *bytes_written);
CK64SaveStatus ck64_save_decode(CK64SaveData *data,
                               const uint8_t *input, size_t length);

#ifdef __cplusplus
}
#endif
#endif
