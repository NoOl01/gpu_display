#pragma once
#include "../monitor.hpp"

class NVIDIAMonitor : public IGPUMonitor {
public:
    bool Init() override;
    GPUData Query() override;
    void Shutdown() override;
};