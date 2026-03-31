#include "gpu_monitor.hpp"

#include <thread>
#include <atomic>
#include <chrono>

#include "monitor.hpp"
#include "amd/amd_monitor.hpp"
#include "nvidia/nvidia_monitor.hpp"

static IGPUMonitor *g_backend = nullptr;
static GPUMonitorCallback g_callback = nullptr;
static std::thread g_thread;
static std::atomic g_running = false;

extern "C" {
EXPORT void SetMonitorCallback(GPUMonitorCallback callback) {
    g_callback = callback;
}

EXPORT int StartMonitoring() {
    auto *amd = new AMDMonitor();
    if (amd->Init()) {
        g_backend = amd;
    } else {
        delete amd;
        auto *nvidia = new NVIDIAMonitor();
        if (nvidia->Init()) {
            g_backend = nvidia;
        } else {
            delete nvidia;
            return -1;
        }
    }
    g_running = true;
    g_thread = std::thread([] {
        while (g_running) {
            if (g_callback) {
                GPUData data = g_backend->Query();
                g_callback(&data);
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });

    return 0;
}

EXPORT void StopMonitoring() {
    g_running = false;
    if (g_thread.joinable()) g_thread.join();
    if (g_backend) {
        g_backend->Shutdown();
        delete g_backend;
        g_backend = nullptr;
    }
}
}
