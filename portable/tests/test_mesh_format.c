#include "ck64_mesh_format.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void be16(uint8_t *p,uint16_t v) {p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void be32(uint8_t *p,uint32_t v) {
 p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);
 p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;
}
static void bef(uint8_t *p,float f) {
 uint32_t u;memcpy(&u,&f,sizeof u);be32(p,u);
}
static void sample(uint8_t *b) {
 memset(b,0,52);memcpy(b,"CKM1",4);be32(b+4,3);be32(b+8,1);
 bef(b+12,10.0f);bef(b+16,20.0f);bef(b+20,-30.0f);bef(b+24,0.5f);
 be16(b+28,2);be16(b+30,0);be16(b+32,0);
 be16(b+34,0);be16(b+36,2);be16(b+38,0);
 be16(b+40,0);be16(b+42,0);be16(b+44,2);
 be16(b+46,0);be16(b+48,1);be16(b+50,2);
}
static void valid_header(void) {
 uint8_t b[52];sample(b);CK64MeshView m={0};
 assert(ck64_mesh_view_open(&m,b,sizeof b));
 assert(m.vertex_count==3&&m.triangle_count==1&&m.byte_length==52u);
}
static void valid_positions(void) {
 uint8_t b[52];sample(b);CK64MeshView m;CK64MeshPosition v;
 assert(ck64_mesh_view_open(&m,b,52));
 assert(ck64_mesh_position(&m,0,&v));
 assert(fabsf(v.x-11)<0.0001f&&fabsf(v.y-20)<0.0001f&&fabsf(v.z+30)<0.0001f);
 assert(ck64_mesh_position(&m,1,&v)&&fabsf(v.y-21)<0.0001f);
}
static void valid_triangles(void) {
 uint8_t b[52];sample(b);CK64MeshView m;CK64MeshTriangle t;
 assert(ck64_mesh_view_open(&m,b,52));
 assert(ck64_mesh_triangle(&m,0,&t)&&t.a==0&&t.b==1&&t.c==2);
 assert(!ck64_mesh_triangle(&m,1,&t));
}
static void reject_bad_magic(void) {
 uint8_t b[52];sample(b);b[0]='A';CK64MeshView m;
 assert(!ck64_mesh_view_open(&m,b,52));
}
static void reject_length(void) {
 uint8_t b[52];sample(b);CK64MeshView m;
 assert(!ck64_mesh_view_open(&m,b,51));
 assert(!ck64_mesh_view_open(&m,b,53));
}
static void reject_counts(void) {
 uint8_t b[52];sample(b);CK64MeshView m;
 be32(b+4,0);assert(!ck64_mesh_view_open(&m,b,52));
 sample(b);be32(b+8,65536);assert(!ck64_mesh_view_open(&m,b,52));
}
static void reject_invalid_indices(void) {
 uint8_t b[52];sample(b);CK64MeshView m;
 be16(b+50,3);assert(!ck64_mesh_view_open(&m,b,52));
}
static void reject_bad_scale(void) {
 uint8_t b[52];sample(b);CK64MeshView m;
 bef(b+24,-1);assert(!ck64_mesh_view_open(&m,b,52));
 sample(b);be32(b+24,0x7fc00000u);assert(!ck64_mesh_view_open(&m,b,52));
}
static void atomic_failure(void) {
 uint8_t b[52];sample(b);CK64MeshView m={0};
 assert(!ck64_mesh_view_open(NULL,b,52));
 assert(!ck64_mesh_view_open(&m,NULL,52));
 assert(!ck64_mesh_view_open(&m,b,20));
 assert(m.bytes==NULL && m.vertex_count==0);
}
static void reject_out_of_range_queries(void) {
 uint8_t b[52];sample(b);CK64MeshView m;CK64MeshPosition v={99,99,99};
 assert(ck64_mesh_view_open(&m,b,52));
 assert(!ck64_mesh_position(&m,3,&v));
 assert(v.x==99 && v.y==99 && v.z==99);
 assert(!ck64_mesh_position(NULL,0,&v));
 assert(!ck64_mesh_position(&m,0,NULL));
}
int main(void) {
 valid_header();valid_positions();valid_triangles();reject_bad_magic();
 reject_length();reject_counts();reject_invalid_indices();reject_bad_scale();
 atomic_failure();reject_out_of_range_queries();
 puts("10 CKM1 C11 scenario groups passed");return 0;
}
