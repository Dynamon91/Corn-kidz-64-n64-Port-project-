#ifndef CK64_MESH_FORMAT_H
#define CK64_MESH_FORMAT_H
/* CKM1 v0 -- experimental *private* prequantized geometry reader.
 * NOT an RSP display list or a textured renderer. No commercial mesh bytes
 * are embedded here. The buffer must remain allocated while a view is used.
 * All words and floats are BE for native R4300i-friendly serialization.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct CK64MeshView {
    const uint8_t *bytes;
    size_t byte_length;
    uint32_t vertex_count, triangle_count;
    float center[3];
    float units_per_i16;
} CK64MeshView;

typedef struct CK64MeshPosition {float x,y,z;} CK64MeshPosition;
typedef struct CK64MeshTriangle {uint16_t a,b,c;} CK64MeshTriangle;

/* Checks magic, exact payload size, finite header, positive scale, every
 * triangle index. Failure leaves `out` unchanged. */
bool ck64_mesh_view_open(CK64MeshView *out, const uint8_t *data, size_t bytes);
/* After successful open, random access performs no allocations. */
bool ck64_mesh_position(const CK64MeshView *view, uint32_t vertex,
                        CK64MeshPosition *out);
bool ck64_mesh_triangle(const CK64MeshView *view, uint32_t triangle,
                        CK64MeshTriangle *out);
#endif
