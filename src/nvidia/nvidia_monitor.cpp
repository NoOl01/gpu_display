#include "nvidia_monitor.hpp"


enum class nvmlReturn_t
{
    NVML_SUCCESS = 0,
    NVML_ERROR_UNINITIALIZED = 1,
    NVML_ERROR_INVALID_ARGUMENT = 2,
    NVML_ERROR_NOT_SUPPORTED = 3,
    NVML_ERROR_NO_PERMISSION = 4,
    NVML_ERROR_INSUFFICIENT_POWER = 8,
    NVML_ERROR_DRIVER_NOT_LOADED = 9,
    NVML_ERROR_IRQ_ISSUE = 11,
    NVML_ERROR_GPU_IS_LOST = 15,
    NVML_ERROR_UNKNOWN = 999
};

enum class nvmlTemperatureSensors_t
{
    NVML_TEMPERATURE_GPU = 0,
    NVML_TEMPERATURE_COUNT
};


struct nvmlDevice_st; // Proprietary/Reserved by NVIDIA.

using nvmlDevice_t = struct nvmlDevice_st*;

using nvmlTemperature_v1_t = struct {
    unsigned int version;
    nvmlTemperatureSensors_t sensorType;
    unsigned int temperature;
};

using nvmlUtilization_t = struct {
    std::uint32_t gpu;
    std::uint32_t memory;
};


using NVML_INIT_V2  = nvmlReturn_t(*)(void);
using NVML_SHUTDOWN = nvmlReturn_t(*)(void);
using NVML_DEVICE_GET_HANDLE_BY_INDEX_V2 = nvmlReturn_t(*)(std::uint32_t index, nvmlDevice_t * device);
using NVML_DEVICE_GET_TEMPERATURE        = nvmlReturn_t(*)(nvmlDevice_t device, nvmlTemperatureSensors_t sensorType, std::uint32_t * temperature);
using NVML_DEVICE_GET_UTILIZATION_RATES  = nvmlReturn_t(*)(nvmlDevice_t device, nvmlUtilization_t * utilization);


// Init
static NVML_INIT_V2 nvmlInit_v2{nullptr}; // Might be unsafe if there's no gpu in the system
static NVML_SHUTDOWN nvmlShutdown{nullptr};

// Enum
static NVML_DEVICE_GET_HANDLE_BY_INDEX_V2 nvmlDeviceGetHandleByIndex_v2{nullptr};

// Sensors
static NVML_DEVICE_GET_TEMPERATURE nvmlDeviceGetTemperature{nullptr}; 
static NVML_DEVICE_GET_UTILIZATION_RATES nvmlDeviceGetUtilizationRates{nullptr};



[[nodiscard]] bool NVIDIAMonitor::Init() {
    m_hDLL = LoadLibraryA("nvml.dll");
    if(!m_hDLL) return false;

    // Init
    nvmlInit_v2 = reinterpret_cast<NVML_INIT_V2>(
        GetProcAddress(m_hDLL, "nvmlInit_v2"));
    nvmlShutdown = reinterpret_cast<NVML_SHUTDOWN>(
        GetProcAddress(m_hDLL, "nvmlShutdown"));
    
    // Enum
    nvmlDeviceGetHandleByIndex_v2 = reinterpret_cast<NVML_DEVICE_GET_HANDLE_BY_INDEX_V2>(
        GetProcAddress(m_hDLL, "nvmlDeviceGetHandleByIndex_v2"));

    // Sensors
    nvmlDeviceGetTemperature = reinterpret_cast<NVML_DEVICE_GET_TEMPERATURE>(
        GetProcAddress(m_hDLL, "nvmlDeviceGetTemperature"));
    nvmlDeviceGetUtilizationRates = reinterpret_cast<NVML_DEVICE_GET_UTILIZATION_RATES>(
        GetProcAddress(m_hDLL, "nvmlDeviceGetUtilizationRates"));


    if(!nvmlInit_v2 || !nvmlShutdown) return false;
    if(nvmlInit_v2() != nvmlReturn_t::NVML_SUCCESS) return false;

    IsInitialized = true;
    return true;
}


[[nodiscard]] GPUData NVIDIAMonitor::Query() {
    GPUData data = {0, 0};
    nvmlDevice_t CurrentDevice;
    nvmlReturn_t CurrentCallResult;

    std::uint32_t CurrentTemperature;

    nvmlUtilization_t CurrentUtilization{
        .gpu = 0,
        .memory = 0
    };


    if((CurrentCallResult = nvmlDeviceGetHandleByIndex_v2((std::uint32_t)0, &CurrentDevice)) != nvmlReturn_t::NVML_SUCCESS)
    {
        // Definitely a stub. There's no predictable order of indexing in NVML
        // so it's better to replace indexes to UUID or PCI/BUS ids.
        switch(CurrentCallResult)
        {
            case nvmlReturn_t::NVML_ERROR_INVALID_ARGUMENT:
                if((CurrentCallResult = nvmlDeviceGetHandleByIndex_v2((std::uint32_t)1, &CurrentDevice)) != nvmlReturn_t::NVML_SUCCESS)
                    return data;
                break;
        }
    }

    if((CurrentCallResult = nvmlDeviceGetTemperature(CurrentDevice, nvmlTemperatureSensors_t::NVML_TEMPERATURE_GPU, &CurrentTemperature)) != nvmlReturn_t::NVML_SUCCESS)
    {
        CurrentTemperature = 0;
    }


    if((CurrentCallResult = nvmlDeviceGetUtilizationRates(CurrentDevice, &CurrentUtilization)) != nvmlReturn_t::NVML_SUCCESS)
    {
        CurrentUtilization.gpu = 0;
        CurrentUtilization.memory = 0;
    }

    data.temperature = static_cast<float>(CurrentTemperature);
    data.load = static_cast<float>(CurrentUtilization.gpu);

    return data;
}


void NVIDIAMonitor::Shutdown() {
    nvmlShutdown();
}
