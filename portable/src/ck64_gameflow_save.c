#include "ck64_gameflow_save.h"

CK64Status ck64_gameflow_import_data(CK64GameFlow *g,
                                     const CK64SaveData *d) {
    if (!g||!d) return CK64_INVALID_ARGUMENT;
    /* Recover exactly the five fields represented by GameFlow, without
       pretending that the rest of original Data is modeled by this module. */
    g->data.item_total=d->itemTotal;
    g->data.current_level=d->currentLvl;
    g->data.total_deaths=d->totalDeaths;
    g->data.max_hp=d->maxHP;
    g->data.volume=d->volume;
    g->data.game_time=d->gameTime;
    return CK64_OK;
}
CK64Status ck64_gameflow_export_data(const CK64GameFlow *g,
                                     CK64SaveData *d) {
    if (!g||!d) return CK64_INVALID_ARGUMENT;
    d->itemTotal=g->data.item_total;
    d->currentLvl=g->data.current_level;
    d->totalDeaths=g->data.total_deaths;
    d->maxHP=g->data.max_hp;
    d->volume=g->data.volume;
    d->gameTime=g->data.game_time;
    return CK64_OK;
}
