#pragma once

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#endif

typedef struct {
    float temperature;
    float load;
} GPUData;

typedef void (*GPUMonitorCallback)(GPUData* data);

#ifdef __cplusplus
extern "C" {
#endif

EXPORT void SetMonitorCallback(GPUMonitorCallback callback);

EXPORT int StartMonitoring();

EXPORT void StopMonitoring();

#ifdef __cplusplus
}
#endif