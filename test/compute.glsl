@cs cs
layout(binding=0) uniform params {
    float dt;
    int num_particles;
};

struct particle_t {
    vec4 pos;
    vec4 vel;
};

layout(binding=0) buffer ssbo {
    particle_t prt[];
};

layout(binding=1) readonly buffer ssbo2 {
    particle_t prt2[];
};

layout(local_size_x=64, local_size_y=1, local_size_z=1) in;

void main() {
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= num_particles) {
        return;
    }
    vec4 pos = prt2[idx].pos;
    vec4 vel = prt2[idx].vel;
    vel.y -= 1.0 * dt;
    pos += vel * dt;
    if (pos.y < -2.0) {
        pos.y = -1.8;
        vel *= vec4(0.8, -0.8, 0.8, 0);
    }
    prt[idx].pos = pos;
    prt[idx].vel = vel;
}
@end

@program compute cs
