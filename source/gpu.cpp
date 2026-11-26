#include "gpu.hpp"

#ifdef __SWITCH__
#include <deko3d.hpp>

#include <cstdint>
#include <cstdio>
#include <vector>

namespace
{
dk::UniqueDevice g_device;
dk::UniqueQueue g_queue;

constexpr uint32_t align_up(uint32_t v, uint32_t a)
{
    return (v + a - 1) & ~(a - 1);
}

// Header of a .dksh file (compiled by uam from shaders/*.glsl at build time).
struct DkshHeader
{
    uint32_t magic;
    uint32_t header_sz;
    uint32_t control_sz; // header + control section, read into CPU memory
    uint32_t code_sz;    // GPU code, follows the control section, goes into a Code mem block
    uint32_t programs_off;
    uint32_t num_programs;
};

// Loads a compiled shader into its own code memory block (which must outlive the shader).
bool load_shader(const char* path, dk::UniqueMemBlock& codeMem, dk::Shader& shader)
{
    std::FILE* f = std::fopen(path, "rb");
    if (!f)
        return false;

    DkshHeader hdr;
    std::vector<char> control;
    bool ok = std::fread(&hdr, sizeof(hdr), 1, f) == 1;
    if (ok)
    {
        control.resize(hdr.control_sz);
        std::rewind(f);
        ok = std::fread(control.data(), hdr.control_sz, 1, f) == 1;
    }
    if (ok)
    {
        // The GPU may read a little past the end of shader code, hence the extra unusable space.
        const uint32_t size =
            align_up(hdr.code_sz + DK_SHADER_CODE_UNUSABLE_SIZE, DK_MEMBLOCK_ALIGNMENT);
        codeMem = dk::MemBlockMaker{g_device, size}
                      .setFlags(DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached |
                                DkMemBlockFlags_Code)
                      .create();
        ok = codeMem && std::fread(codeMem.getCpuAddr(), hdr.code_sz, 1, f) == 1;
    }
    std::fclose(f);

    if (ok)
        dk::ShaderMaker{codeMem, 0}.setControl(control.data()).setProgramId(0).initialize(shader);
    return ok;
}
} // namespace

bool gpu_init()
{
    g_device = dk::DeviceMaker{}.create();
    if (!g_device)
    {
        std::printf("gpu: failed to create deko3d device\n");
        return false;
    }
    g_queue = dk::QueueMaker{g_device}.setFlags(DkQueueFlags_Compute).create();
    if (!g_queue)
    {
        std::printf("gpu: failed to create compute queue\n");
        g_device.destroy();
        return false;
    }
    return true;
}

void gpu_exit()
{
    if (g_queue)
        g_queue.waitIdle();
    g_queue.destroy();
    g_device.destroy();
}

bool gpu_selftest()
{
    if (!g_device)
        return false;

    dk::UniqueMemBlock codeMem;
    dk::Shader shader;
    if (!load_shader("romfs:/shaders/selftest.dksh", codeMem, shader))
    {
        std::printf("gpu: could not load romfs:/shaders/selftest.dksh\n");
        return false;
    }

    // One block: the first page holds the command list, the second the data buffer.
    // GpuUncached so the shader's writes go straight to memory and the CPU can read them back
    // without a cache flush.
    constexpr uint32_t kCmdSize = DK_MEMBLOCK_ALIGNMENT;
    constexpr uint32_t kCount = 64; // must match local_size_x in selftest.glsl
    dk::UniqueMemBlock mem =
        dk::MemBlockMaker{g_device, 2 * DK_MEMBLOCK_ALIGNMENT}
            .setFlags(DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuUncached)
            .create();
    auto* values = reinterpret_cast<uint32_t*>(static_cast<char*>(mem.getCpuAddr()) + kCmdSize);
    for (uint32_t i = 0; i < kCount; ++i)
        values[i] = i;

    dk::UniqueCmdBuf cmd = dk::CmdBufMaker{g_device}.create();
    cmd.addMemory(mem, 0, kCmdSize);
    cmd.bindShaders(DkStageFlag_Compute, {&shader});
    cmd.bindStorageBuffer(DkStage_Compute, 0, mem.getGpuAddr() + kCmdSize,
                          kCount * sizeof(uint32_t));
    cmd.dispatchCompute(1, 1, 1);
    g_queue.submitCommands(cmd.finishList());
    g_queue.waitIdle();

    for (uint32_t i = 0; i < kCount; ++i)
    {
        if (values[i] != i * 2)
        {
            std::printf("gpu: selftest FAILED at [%u]: got %u, expected %u\n", (unsigned)i,
                        (unsigned)values[i], (unsigned)(i * 2));
            return false;
        }
    }
    std::printf("gpu: selftest passed (%u values doubled on the GPU)\n", (unsigned)kCount);
    return true;
}

#else // desktop build: no GPU backend

bool gpu_init()
{
    return false;
}

void gpu_exit()
{
}

bool gpu_selftest()
{
    return false;
}

#endif
