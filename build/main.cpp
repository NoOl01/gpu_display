#include <windows.h>
#include <iostream>
#include <thread>
#include <chrono>

typedef struct {
    float temperature;
    float load;
} GPUData;

typedef void (*GPUMonitoringCallback)(GPUData* data);

typedef void (*SetMonitoringCallback_t) (GPUMonitoringCallback);
typedef int (*StartMonitoring_t)();
typedef int (*StopMonitoring_t)();

void onGPUData(GPUData* data) {
    std::cout << "Temp: " << data->temperature << " C" << std::endl;
    std::cout << "Load: " << data->load << " %" << std::endl;
    std::cout << "---" << std::endl;
}

int main() {
    HMODULE dll = LoadLibraryA("gpu_display.dll");
    if (!dll) {
        std::cout << "Failed to load gpu_display.dll" << std::endl;
        return -1;
    }

    auto SetMonitorCallback = reinterpret_cast<SetMonitoringCallback_t>(GetProcAddress(dll, "SetMonitorCallback"));
    auto StartMonitoring = reinterpret_cast<StartMonitoring_t>(GetProcAddress(dll, "StartMonitoring"));
    auto StopMonitoring = reinterpret_cast<StopMonitoring_t>(GetProcAddress(dll, "StopMonitoring"));

    SetMonitorCallback(onGPUData);

    if (StartMonitoring() != 0) {
        std::cout << "Failed to start monitoring" << std::endl;
        return -1;
    }

    std::this_thread::sleep_for(std::chrono::seconds(10));

    StopMonitoring();
    FreeLibrary(dll);

    return 0;
}
