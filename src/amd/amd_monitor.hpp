#pragma once
#include <windows.h>

#include "../monitor.hpp"
#include "../gpu_monitor.hpp"

class AMDMonitor : public IGPUMonitor {
    public:
    bool Init() override;
    GPUData Query() override;
    void Shutdown() override;

    private:
    HMODULE m_hDLL = nullptr;
    void* m_context = nullptr;
    int m_adapterIndex = -1;
    int m_odVersion = 0;
};