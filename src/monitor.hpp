#pragma once
#include "gpu_monitor.hpp"

class IGPUMonitor {
public:
    [[nodiscard]] virtual bool Init() = 0;
    [[nodiscard]] virtual GPUData Query() = 0;
    virtual void Shutdown() = 0;

    virtual ~IGPUMonitor() = default;
};
