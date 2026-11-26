#version 460

// Compute shader used by gpu_selftest(): doubles every value in the buffer.
// Every .glsl in this folder is compiled as a compute shader to romfs/shaders/<name>.dksh.

layout (local_size_x = 64) in;

layout (std430, binding = 0) buffer Data
{
    uint values[];
} d;

void main()
{
    d.values[gl_GlobalInvocationID.x] *= 2u;
}
