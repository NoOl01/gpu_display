#pragma once

#include <iostream>


#include <windows.h>


#include "../monitor.hpp"
#include "../gpu_monitor.hpp"

#define WIN32_LEAN_AND_MEAN


class AMDMonitor : public IGPUMonitor {
    public:
        ~AMDMonitor()
        {
            if(IsInitialized) Shutdown();
        }
        [[nodiscard]] bool Init() override;
        [[nodiscard]] GPUData Query() override;
        void Shutdown() override;

    private:
        HMODULE m_hDLL = nullptr;
        void* m_context = nullptr;
        int m_adapterIndex = -1;
        int m_odVersion = 0;
        bool IsInitialized{false};
};
