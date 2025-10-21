#version 430

@vs vs

layout(location = 0) in vec2 in_pos;
layout(location = 1) in vec2 in_uv;
out vec2 uv;
void main() {
    gl_Position = vec4(in_pos, 0, 1);
    uv = vec2(in_uv.x, 1 - in_uv.y);
}

@end


@fs fs

#define GLOBAL_ILLUMINATION_COLOR (vec3(1))
#define GLOBAL_ILLUMINATION       (GLOBAL_ILLUMINATION_COLOR * 0.2)

#define MAX_REFLECTIONS (3)
#define RAYS_PER_PIXEL  (4)

#define WORLD_UP (vec3(0, 0, 1))

#define PI (radians(180))

#define FOV (radians(120))

#define RW (render_resolution[0])
#define RH (render_resolution[1])

#define EPS (1e-6f)

#define BLACK (vec4(0, 0, 0, 1))
#define WHITE (vec4(1, 1, 1, 1))

struct uv_t {
    vec2 uv;
};

struct basichitinfo_t {
    float dst;
    vec2 uv;
};

struct hitinfo_t {
    basichitinfo_t hit;
    uint model;
    uint tri;
};

struct ray_t {
    vec3 pos;
    vec3 dir;
};

struct material_t {
    float light;
    float specular;
    float glossiness;
};

struct triangle_t {
    vec3 v0;
    vec3 edge1;
    vec3 edge2;

    vec3 normal;

    vec2 uv0;
    vec2 uv1;
    vec2 uv2;

    material_t m;
};

struct boundingbox_t {
    vec3 min;
    vec3 max;
};

struct bvh_t {
    boundingbox_t bb;
    uint start;
    uint end;
    uint child_a;
    uint child_b;
};

struct texinfo_t {
    vec2 pos;
    vec2 scale;
};

struct modelinfo_t {
    int texture;
    uint bvh_root;
    mat4 transform;
    mat4 inverse_transform;
};

layout(binding = 0) uniform texture2D scene_texture_atlas;
layout(binding = 0) uniform sampler scene_sampler;

layout(binding = 1) uniform ssbo_vector_lengths {
    int models_count;
};

layout(binding = 2) uniform camera {
    vec2 render_resolution;
    vec3 camera_pos;
    vec3 camera_dir;
    vec3 camera_right;
    vec3 camera_down;
};

layout(std430, binding = 3) readonly buffer scene_tris
{
    triangle_t tris[];
};

layout(std430, binding = 4) readonly buffer scene_bvhs
{
    bvh_t bvhs[];
};

layout(std430, binding = 5) readonly buffer scene_textures
{
    texinfo_t textures[];
};

layout(std430, binding = 6) readonly buffer scene_models
{
    modelinfo_t models[];
};


in vec2 uv;
out vec4 frag_color;

uint pcg32(inout uint seed) {
    seed = seed * 747796405 + 2891336453;
    const uint word = ((seed >> ((seed >> 28) + 4)) ^ seed) * 277803737;
    return (word >> 22) ^ word;
}

uint lcg(inout uint seed) {
    seed = seed * 1664525 + 1013904223;
    return seed;
}

uint xorshift32(inout uint seed) {
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
}

float rand01(inout uint seed) {
    return lcg(seed) / 4294967296.f;
}

vec3 cosine_sample_hemisphere(inout uint seed) {
    float r = sqrt(rand01(seed));
    float theta = 2 * PI * rand01(seed);

    float x = r * cos(theta);
    float y = r * sin(theta);
    float z = sqrt(max(0, 1 - x * x - y * y));

    return vec3(x, y, z);
}

mat3 make_tbn(vec3 n) {
    vec3 t = normalize(abs(n.z) < 1 - EPS ? cross(vec3(0, 0, 1), n)
                                          : cross(vec3(0, 1, 0), n));
    vec3 b = cross(n, t);
    return mat3(t, b, n);
}

float ray_box(const ray_t r, const boundingbox_t bb)
{
    const vec3 inv_dir = 1 / r.dir;
    const vec3 transformed_min = (bb.min - r.pos) * inv_dir;
    const vec3 transformed_max = (bb.max - r.pos) * inv_dir;

    const vec3 tsm = min(transformed_min, transformed_max);
    const vec3 tbg = max(transformed_min, transformed_max);

    const float tmin = max(max(max(tsm.x, tsm.y), tsm.z), 0);
    const float tmax = min(min(tbg.x, tbg.y), tbg.z);

    if (tmax < tmin)
        return -1;

    return tmin;
}

vec2 interpolate_uv(const uint tri, const float u, const float v) {
    const float w = 1 - u - v;
    return tris[tri].uv0 * w + tris[tri].uv1 * u + tris[tri].uv2 * v;
}

basichitinfo_t ray_tri(const ray_t r, const uint tri)
{
    const vec3 ray_cross_e2 = cross(r.dir, tris[tri].edge2);
    const float det = dot(tris[tri].edge1, ray_cross_e2);

    if (det > -EPS && det < EPS)
        return basichitinfo_t(-1, vec2(0));

    const vec3 s = r.pos - tris[tri].v0;
    const float inv_det = 1 / det;

    const float u = dot(s, ray_cross_e2) * inv_det;

    if (u < 0 || u > 1)
        return basichitinfo_t(-1, vec2(0));

    const vec3 s_cross_e1 = cross(s, tris[tri].edge1);
    const float v = dot(r.dir, s_cross_e1) * inv_det;

    if (v < 0 || u + v > 1)
        return basichitinfo_t(-1, vec2(0));

    const float t = dot(tris[tri].edge2, s_cross_e1) * inv_det;

    if (t > 0)
        return basichitinfo_t(t, interpolate_uv(tri, u, v));

    return basichitinfo_t(-1, vec2(0));
}

hitinfo_t ray_mesh(ray_t r, const uint model)
{
    ray_t world_ray = r;
    r.pos = (models[model].inverse_transform * vec4(r.pos, 1)).xyz;
    r.dir = normalize(mat3(models[model].inverse_transform) * r.dir);

    hitinfo_t result;
    result.hit.dst = -1;
    
    uint stack[32];
    int stack_ptr = 0;
    stack[stack_ptr++] = models[model].bvh_root;

    while (stack_ptr > 0) {
        uint node_idx = stack[--stack_ptr];
        bvh_t node = bvhs[node_idx];

        float node_dst = ray_box(r, node.bb);
        if (node_dst < 0)
            continue;
        if (result.hit.dst >= 0 && node_dst >= result.hit.dst)
            continue;

        if (node.child_a == 0 || node.child_b == 0) {
            for (uint i = node.start; i < node.end; ++i) {
                basichitinfo_t hit = ray_tri(r, i);
                if (hit.dst > 0 && (hit.dst < result.hit.dst || result.hit.dst < 0)) {
                    result = hitinfo_t(hit, model, i);
                }
            }

            continue;
        }

        float dst_a = ray_box(r, bvhs[node.child_a].bb);
        float dst_b = ray_box(r, bvhs[node.child_b].bb);

        bool a_valid = (dst_a >= 0) && (result.hit.dst < 0 || dst_a < result.hit.dst);
        bool b_valid = (dst_b >= 0) && (result.hit.dst < 0 || dst_b < result.hit.dst);

        if (a_valid && b_valid) {
            if (dst_a < dst_b) {
                stack[stack_ptr++] = node.child_b;
                stack[stack_ptr++] = node.child_a;
            } else {
                stack[stack_ptr++] = node.child_a;
                stack[stack_ptr++] = node.child_b;
            }

            continue;
        }

        if (a_valid)
            stack[stack_ptr++] = node.child_a;
        if (b_valid)
            stack[stack_ptr++] = node.child_b;
    }

    if (result.hit.dst >= 0) {
        vec3 model_hit = r.pos + r.dir * result.hit.dst;
        vec4 world_hit = models[model].transform * vec4(model_hit, 1);
        result.hit.dst = distance(world_ray.pos, world_hit.xyz);
    }
    return result;
}

hitinfo_t ray_scene(ray_t r)
{
    /*
     * TODO: Maybe sorting objects by distance and checking bvhs
     * Now I can sort models[] in C code
     */
    hitinfo_t result;
    result.hit.dst = -1;
    for (uint i = 0; i < models_count; ++i) {
        ray_t r_copy = r;
        r_copy.pos = (models[i].inverse_transform * vec4(r_copy.pos, 1)).xyz;
        r_copy.dir = normalize(mat3(models[i].inverse_transform) * r_copy.dir);
        float bb_dst = ray_box(r_copy, bvhs[models[i].bvh_root].bb);
        if (bb_dst >= 0) {
            const vec3 model_hit = r_copy.pos + r_copy.dir * bb_dst;
            const vec4 world_hit = models[i].transform * vec4(model_hit, 1);
            bb_dst = distance(r.pos, world_hit.xyz);
        }
        if (bb_dst < 0 || (0 <= result.hit.dst && result.hit.dst < bb_dst)) continue;

        const hitinfo_t mesh = ray_mesh(r, i);
        if (mesh.hit.dst > 0 && (result.hit.dst < 0 || mesh.hit.dst < result.hit.dst))
            result = mesh;
    }

    result.hit.dst -= EPS;
    return result;
}

vec3 trace_color(ray_t r, inout uint rng_seed)
{
    vec3 light_color = vec3(0);
    vec3 ray_color = vec3(1);

    for (uint bounce = 0; bounce < MAX_REFLECTIONS; ++bounce) {
        hitinfo_t hit = ray_scene(r);
        material_t m = tris[hit.tri].m;

        mat3 normal_matrix = mat3(transpose(models[hit.model].inverse_transform));
        vec3 surf_normal = normalize(normal_matrix * tris[hit.tri].normal);

        if (dot(r.dir, surf_normal) > 0) surf_normal = -surf_normal;

        r.pos += r.dir * hit.hit.dst;

        mat3 tbn = make_tbn(surf_normal);
        if (rand01(rng_seed) > m.glossiness)
            r.dir = normalize(mix(tbn * cosine_sample_hemisphere(rng_seed), reflect(r.dir, surf_normal), m.specular));
        else
            r.dir = reflect(r.dir, surf_normal);


        if (hit.hit.dst < 0) {
            light_color += GLOBAL_ILLUMINATION * ray_color;

            if (m.light == 0) ray_color *= GLOBAL_ILLUMINATION_COLOR;

            break;
        }

        vec3 texture_color = (mod(floor(hit.hit.uv.x * 32.0) + floor(hit.hit.uv.y * 32.0), 2.0) == 0.0) 
                   ? vec3(1, 0, 1)
                   : vec3(0);
        if (models[hit.model].texture >= 0) {
            const texinfo_t t = textures[models[hit.model].texture];
            texture_color = texture(
                sampler2D(
                    scene_texture_atlas,
                    scene_sampler
                ),
                hit.hit.uv * t.scale + t.pos
            ).rgb;
        }

        light_color += texture_color * m.light * ray_color;

        if (m.light == 0) ray_color *= texture_color;
    }

    return light_color;
}

void main()
{
    uint rng_seed = uint(uv.x * 1024 * 1024) ^ uint(uv.y * 1024 * 1024);

    /*
     * cameraDir should be normalized by CPU
     */
    ray_t r = ray_t(camera_pos, camera_dir);

    /*
     * TODO: Can I just put constants in camera vectors? idk
     */
    r.dir += 2 * (uv.x - 0.5) * camera_right * tan(FOV / 2);
    r.dir += 2 * (uv.y - 0.5) * camera_down * tan(FOV / 2) * RH / RW;
    r.dir = normalize(r.dir);

    vec3 result_color = vec3(0);
    for (uint i = 0; i < RAYS_PER_PIXEL; ++i) {
        result_color += trace_color(r, rng_seed);
    }
    result_color /= RAYS_PER_PIXEL;

    frag_color = vec4(result_color, 1);
}

@end

@program raytracer vs fs


@vs vs_fullscreen
layout(location = 0) in vec2 in_pos;
layout(location = 1) in vec2 in_uv;
out vec2 uv;
void main() {
    gl_Position = vec4(in_pos, 0, 1);
    uv = in_uv;
}
@end

@fs fs_fullscreen
layout(binding = 0) uniform texture2D tex;
layout(binding = 0) uniform sampler smp;
in vec2 uv;
out vec4 frag_color;
void main() {
    frag_color = texture(sampler2D(tex, smp), uv);
}
@end

@program fullscreen vs_fullscreen fs_fullscreen
