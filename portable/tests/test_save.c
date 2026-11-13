#include "ck64_save.h"
#include "ck64_gameflow_save.h"
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define START_SCALAR (CK64_SAVE_HEADER_SIZE+125+125+13+1200)
#define FLAG_OFFSET (START_SCALAR+48)

static uint8_t record[CK64_SAVE_RECORD_SIZE];
static uint8_t record2[CK64_SAVE_RECORD_SIZE];

static void test_reset(void) {
    CK64SaveData d;
    memset(&d,0xFF,sizeof(d));
    ck64_save_init_fresh(&d);
    assert(d.currentLvl==1 && d.maxHP==8);
    assert(d.volume==0 && d.bMusic==false && d.ctrlType==0);
    for(size_t i=0;i<1000;i++)
        assert(d.switches[i]==0 && d.items[i]==0);
    for(size_t i=0;i<100;i++)
        assert(!d.upgrades[i] && !d.ints[i] && !d.cranks[i] && !d.lvlItems1[i]);
}
static void test_reset_preserves_settings(void) {
    CK64SaveData d;
    ck64_save_init_fresh(&d);
    d.volume=75;d.bMusic=true;d.ctrlType=5;d.bFullscreen=true;
    d.bVsync=true;d.antialias=4;d.b60fps=true;d.bForce60hz=true;
    d.items[99]=1;d.ints[11]=-123;
    d.maxHP=100;d.currentLvl=7;d.bWidescreen=true;
    ck64_save_reset(&d);
    assert(d.volume==75 && d.bMusic && d.ctrlType==5 && d.bFullscreen);
    assert(d.bVsync && d.antialias==4 && d.b60fps && d.bForce60hz);
    assert(d.items[99]==0 && d.ints[11]==0 && d.maxHP==8);
    assert(d.currentLvl==1 && !d.bWidescreen);
}
static void test_default_golden(void) {
    CK64SaveData d;
    size_t n=0;
    ck64_save_init_fresh(&d);
    assert(ck64_save_encode(&d,record,sizeof(record),&n)==CK64_SAVE_OK);
    assert(n==1533 && !memcmp(record,"CK64SAVE",8));
    assert(record[8]==0 && record[9]==1);
    assert(record[12]==0 && record[13]==0 && record[14]==0x05 && record[15]==0xE9);
    assert(record[16]==0x84 && record[17]==0xCA && record[18]==0x69 && record[19]==0xDD); /* independent Python zlib.crc32 */
    assert(record[START_SCALAR+7]==1 && record[START_SCALAR+19]==8);
    assert(record[FLAG_OFFSET]==0 && record[FLAG_OFFSET+1]==0);
    assert(ck64_save_decode(&d,record,n)==CK64_SAVE_OK);
}
static void test_roundtrip_all_fields(void) {
    CK64SaveData a,b;
    size_t n=0;
    ck64_save_init_fresh(&a);
    a.switches[0]=1;a.switches[7]=1;a.switches[8]=1;a.switches[999]=1;
    a.items[11]=1;a.items[999]=1;
    a.upgrades[3]=1;a.upgrades[99]=1;
    for(unsigned i=0;i<100;i++) {
        a.ints[i]=(int32_t)(i*i);
        a.cranks[i]=-(int32_t)(i+1);
        a.lvlItems1[i]=(int32_t)(i*321);
    }
    a.ints[0]=INT32_MIN;a.ints[99]=INT32_MAX;
    a.itemTotal=70;a.currentLvl=2;a.HPupTotal=3;a.screwTotal=300;
    a.maxHP=12;a.totalDeaths=1234;a.gameTime=1234567;
    a.currentCamMode=-7;a.ctrlType=10;a.clothes=0x11223344;
    a.volume=99;a.antialias=-1;
    a.bNonEmpty=true;a.camInvertOff=true;a.bRumble=true;
    a.bSkipLauncher=true;a.bFullscreen=true;a.bWidescreen=true;
    a.bVsync=true;a.bRAnalogDpad=true;a.bAnalogX2=true;a.b1080=true;
    a.bIgnoreDpadX=true;a.bP1only=true;a.bMusic=true;
    a.bAchDisable=true;a.b60fps=true;a.bForce60hz=true;
    assert(ck64_save_encode(&a,record,sizeof(record),&n)==CK64_SAVE_OK);
    assert(record[20]==0x81 && record[21]==0x01);
    assert(record[START_SCALAR]==0 && record[START_SCALAR+3]==70);
    assert(record[FLAG_OFFSET]==0xFF && record[FLAG_OFFSET+1]==0xFF);
    assert(ck64_save_decode(&b,record,n)==CK64_SAVE_OK);
    assert(b.ints[0]==INT32_MIN && b.ints[99]==INT32_MAX);
    assert(b.cranks[99]==-100 && b.currentCamMode==-7);
    assert(b.clothes==0x11223344 && b.antialias==-1);
    assert(b.switches[999] && b.items[999] && b.upgrades[99]);
    assert(b.bForce60hz && b.bNonEmpty && b.bMusic);
    assert(ck64_save_encode(&b,record2,sizeof(record2),&n)==CK64_SAVE_OK);
    assert(!memcmp(record,record2,sizeof(record)));
}
static void test_bad_checksums_atomic(void) {
    CK64SaveData a,b;
    ck64_save_init_fresh(&a);
    a.totalDeaths=1234;
    assert(ck64_save_encode(&a,record,sizeof(record),NULL)==CK64_SAVE_OK);
    memset(&b,0xAB,sizeof(b));
    CK64SaveData original=b;
    record[50]^=0x10;
    assert(ck64_save_decode(&b,record,sizeof(record))==CK64_SAVE_BAD_CHECKSUM);
    assert(!memcmp(&b,&original,sizeof(b)));
}
static void test_bad_size_and_header(void) {
    CK64SaveData d;
    ck64_save_init_fresh(&d);
    assert(ck64_save_encode(&d,record,sizeof(record),NULL)==CK64_SAVE_OK);
    assert(ck64_save_decode(&d,record,CK64_SAVE_RECORD_SIZE-1)==CK64_SAVE_BAD_LENGTH);
    record[0]='X';
    assert(ck64_save_decode(&d,record,sizeof(record))==CK64_SAVE_INVALID_MAGIC);
    record[0]='C';record[9]=2;
    assert(ck64_save_decode(&d,record,sizeof(record))==CK64_SAVE_UNSUPPORTED_VERSION);
    record[9]=1;record[11]=1;
    assert(ck64_save_decode(&d,record,sizeof(record))==CK64_SAVE_BAD_LENGTH);
    record[11]=0;record[15]=0;
    assert(ck64_save_decode(&d,record,sizeof(record))==CK64_SAVE_BAD_LENGTH);
}
static void test_bad_input(void) {
    CK64SaveData d;
    size_t n=777;
    ck64_save_init_fresh(&d);
    assert(ck64_save_encode(NULL,record,sizeof(record),&n)==CK64_SAVE_INVALID_ARGUMENT);
    assert(n==0);
    assert(ck64_save_encode(&d,record,2,&n)==CK64_SAVE_BUFFER_TOO_SMALL);
    assert(n==0);
    assert(ck64_save_decode(NULL,record,sizeof(record))==CK64_SAVE_INVALID_ARGUMENT);
    assert(ck64_save_decode(&d,NULL,sizeof(record))==CK64_SAVE_INVALID_ARGUMENT);
    d.switches[999]=2;
    assert(ck64_save_encode(&d,record,sizeof(record),&n)==CK64_SAVE_INVALID_BOOLEAN);
}
static void test_bridge_preserves_unmodeled(void) {
    CK64GameFlow flow;
    CK64SaveData data;
    CK64Backend backend={0};
    ck64_gameflow_init(&flow,backend);
    ck64_save_init_fresh(&data);
    data.itemTotal=199;data.currentLvl=3;data.totalDeaths=8;
    data.maxHP=12;data.volume=50;data.gameTime=33;
    data.upgrades[99]=1;data.ints[3]=-100;data.bMusic=true;
    assert(ck64_gameflow_import_data(&flow,&data)==CK64_OK);
    assert(flow.data.current_level==3 && flow.data.item_total==199);
    assert(flow.data.game_time==33);
    flow.data.item_total=200;flow.data.total_deaths=9;flow.data.game_time=34;
    assert(ck64_gameflow_export_data(&flow,&data)==CK64_OK);
    assert(data.itemTotal==200 && data.totalDeaths==9 && data.gameTime==34);
    assert(data.upgrades[99]==1 && data.ints[3]==-100 && data.bMusic);
}
int main(void) {
    test_reset();test_reset_preserves_settings();test_default_golden();test_roundtrip_all_fields();
    test_bad_checksums_atomic();test_bad_size_and_header();test_bad_input();
    test_bridge_preserves_unmodeled();
    puts("8 save and interoperability tests passed");
    return 0;
}
