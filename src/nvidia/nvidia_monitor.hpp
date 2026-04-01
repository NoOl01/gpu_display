#pragma once



#include <iostream>
#include <cstdint>


#include <windows.h>


#include "../monitor.hpp"


#define WIN32_LEAN_AND_MEAN // significant reduction in size


class NVIDIAMonitor : public IGPUMonitor {
public:
    ~NVIDIAMonitor()
    {
        if(IsInitialized) Shutdown();
    }
    [[nodiscard]] bool Init() override;
    [[nodiscard]] GPUData Query() override;
    void Shutdown() override;
private:
    HMODULE m_hDLL{nullptr};
    bool IsInitialized{false};
};
