#ifndef GPU_STRUCTURES_H
#define GPU_STRUCTURES_H

#include "../vec3.h"

struct mat4 {
    float m[16];  // 4x4 matrix stored in row-major order
};

struct primitive_gpu_data {
    int type;  // 0=sphere, 1=quad, 2=box, 3=constant_medium
    int material_index;
    int transform_index;  // For rotate/translate operations
    vec3 center;
    vec3 u, v, w;  // For quads and boxes
    float radius;  // For spheres
    float density; // For constant medium
};

struct transform_gpu_data {
    int type;      // 0=none, 1=translate, 2=rotate_y
    vec3 offset;   // For translate
    float angle;   // For rotate_y
    mat4 matrix;   // Precomputed transform matrix
};

struct material_gpu_data {
    int type;      // 0=lambertian, 1=metal, 2=dielectric, 3=diffuse_light, 4=isotropic
    vec3 albedo;
    float fuzz;
    float ir;
    int texture_type;  // 0=solid, 1=checker, 2=noise, 3=image
    vec3 texture_data; // Parameters for procedural textures
    int texture_id;    // For image textures
};

struct bvh_node_gpu_data {
    vec3 min;
    vec3 max;
    int left_index;
    int right_index;
    int primitive_index;
};

#endif 