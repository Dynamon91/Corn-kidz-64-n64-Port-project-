#ifndef CK64_GAMEFLOW_SAVE_H
#define CK64_GAMEFLOW_SAVE_H
/* Deliberate subset bridge: preserves all 29 unmodeled Data fields in saves.
 * The gameflow core currently manages only 6 of Data's 34 fields. */
#include "ck64_gameflow.h"
#include "ck64_save.h"
#ifdef __cplusplus
extern "C" {
#endif

CK64Status ck64_gameflow_import_data(CK64GameFlow *flow,
                                     const CK64SaveData *data);
CK64Status ck64_gameflow_export_data(const CK64GameFlow *flow,
                                     CK64SaveData *data);

#ifdef __cplusplus
}
#endif
#endif
