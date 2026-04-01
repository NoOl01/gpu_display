#include "amd_monitor.hpp"


#include "../../lib/adl/include/adl_sdk.h"
#include "../../lib/adl/include/adl_defines.h"
#include "../../lib/adl/include/adl_structures.h"


using ADL_MAIN_MALLOC_CALLBACK          = void * (__stdcall *)(int iSize);
using ADL2_MAIN_CONTROL_CREATE          = int(*)(ADL_MAIN_MALLOC_CALLBACK callback, int iEnumConnectedAdapters, void ** context);
using ADL2_MAIN_CONTROL_DESTROY         = int(*)(void * context);
using ADL2_ADAPTER_NUMBEROFADAPTERS_GET = int(*)(void * context, int * lpNumAdapters);
using ADL2_ADAPTER_ADAPTERINFO_GET      = int(*)(void * context, AdapterInfo * lpInfo, int iInputSize);
using ADL2_OVERDRIVE_CAPS               = int(*)(void * context, int iAdapterIndex, int * lpSupported, int * lpEnabled, int * lpVersion);

// OD6
using ADL2_OVERDRIVE6_TEMPERATURE_GET   = int(*)(void * context, int iAdapterIndex, int * lpTemperature);
using ADL2_OVERDRIVE6_CURRENTSTATUS_GET = int(*)(void * context, int iAdapterIndex, ADLOD6CurrentStatus * lpCurrentStatus);

// ODN (7)
using ADL2_OVERDRIVEN_TEMPERATURE_GET       = int(*)(void * context, int iAdapterIndex, int iTemperatureSource, int * lpTemperature);
using ADL2_OVERDRIVEN_PERFORMANCESTATUS_GET = int(*)(void * context, int iAdapterIndex, ADLODNPerformanceStatus * lpPerformanceStatus);

// OD8
using ADL2_NEW_QUERYPMLOGDATA_GET         = int(*)(void * context, int iAdapterIndex, ADLPMLogData * lpDataOutput);
using ADL2_OVERDRIVE8_CURRENT_SETTING_GET = int(*)(void * context, int iAdapterIndex, ADLOD8CurrentSetting * lpCurrentSetting);


static ADL2_MAIN_CONTROL_CREATE ADL2_Main_Control_Create{nullptr};
static ADL2_MAIN_CONTROL_DESTROY ADL2_Main_Control_Destroy{nullptr};
static ADL2_ADAPTER_NUMBEROFADAPTERS_GET ADL2_Adapter_NumberOfAdapters_Get{nullptr};
static ADL2_ADAPTER_ADAPTERINFO_GET ADL2_Adapter_AdapterInfo_Get{nullptr};
static ADL2_OVERDRIVE_CAPS ADL2_Overdrive_Caps{nullptr};

// OD6
static ADL2_OVERDRIVE6_TEMPERATURE_GET ADL2_Overdrive6_Temperature_Get{nullptr};
static ADL2_OVERDRIVE6_CURRENTSTATUS_GET ADL2_Overdrive6_CurrentStatus_Get{nullptr};

// ODN (7)
static ADL2_OVERDRIVEN_TEMPERATURE_GET ADL2_OverdriveN_Temperature_Get{nullptr};
static ADL2_OVERDRIVEN_PERFORMANCESTATUS_GET ADL2_OverdriveN_PerformanceStatus_Get{nullptr};

// OD8
static ADL2_NEW_QUERYPMLOGDATA_GET ADL2_New_QueryPMLogData_Get{nullptr};
static ADL2_OVERDRIVE8_CURRENT_SETTING_GET ADL2_Overdrive8_Current_Setting_Get{nullptr};

static void __stdcall * ADL_Main_Memory_Alloc(int size) {
    return malloc(size);
}

[[nodiscard]] bool AMDMonitor::Init() {
    m_hDLL = LoadLibraryA("atiadlxx.dll");
    if (!m_hDLL) m_hDLL = LoadLibraryA("atiadlxy.dll");
    if (!m_hDLL) return false;

    ADL2_Main_Control_Create = reinterpret_cast<ADL2_MAIN_CONTROL_CREATE>(
        GetProcAddress(m_hDLL, "ADL2_Main_Control_Create"));
    ADL2_Main_Control_Destroy = reinterpret_cast<ADL2_MAIN_CONTROL_DESTROY>(
        GetProcAddress(m_hDLL, "ADL2_Main_Control_Destroy"));
    ADL2_Adapter_NumberOfAdapters_Get = reinterpret_cast<ADL2_ADAPTER_NUMBEROFADAPTERS_GET>(
        GetProcAddress(m_hDLL, "ADL2_Adapter_NumberOfAdapters_Get"));
    ADL2_Adapter_AdapterInfo_Get = reinterpret_cast<ADL2_ADAPTER_ADAPTERINFO_GET>(
        GetProcAddress(m_hDLL, "ADL2_Adapter_AdapterInfo_Get"));
    ADL2_Overdrive_Caps = reinterpret_cast<ADL2_OVERDRIVE_CAPS>(
        GetProcAddress(m_hDLL, "ADL2_Overdrive_Caps"));

    // OD6
    ADL2_Overdrive6_Temperature_Get = reinterpret_cast<ADL2_OVERDRIVE6_TEMPERATURE_GET>(
        GetProcAddress(m_hDLL, "ADL2_Overdrive6_Temperature_Get"));
    ADL2_Overdrive6_CurrentStatus_Get = reinterpret_cast<ADL2_OVERDRIVE6_CURRENTSTATUS_GET>(
        GetProcAddress(m_hDLL, "ADL2_Overdrive6_CurrentStatus_Get"));

    // ODN (7)
    ADL2_OverdriveN_Temperature_Get = reinterpret_cast<ADL2_OVERDRIVEN_TEMPERATURE_GET>(
        GetProcAddress(m_hDLL, "ADL2_OverdriveN_Temperature_Get"));
    ADL2_OverdriveN_PerformanceStatus_Get = reinterpret_cast<ADL2_OVERDRIVEN_PERFORMANCESTATUS_GET>(
        GetProcAddress(m_hDLL, "ADL2_OverdriveN_PerformanceStatus_Get"));

    // OD8
    ADL2_New_QueryPMLogData_Get = reinterpret_cast<ADL2_NEW_QUERYPMLOGDATA_GET>(
        GetProcAddress(m_hDLL, "ADL2_New_QueryPMLogData_Get"));
    ADL2_Overdrive8_Current_Setting_Get = reinterpret_cast<ADL2_OVERDRIVE8_CURRENT_SETTING_GET>(
        GetProcAddress(m_hDLL, "ADL2_Overdrive8_Current_Setting_Get"));

    if (!ADL2_Main_Control_Create || !ADL2_Adapter_NumberOfAdapters_Get) return false;
    if (ADL2_Main_Control_Create(ADL_Main_Memory_Alloc, 1, &m_context) != 0) return false;

    int numAdapters = 0;
    ADL2_Adapter_NumberOfAdapters_Get(m_context, &numAdapters);

    auto *infos = new AdapterInfo[static_cast<size_t>(numAdapters)];
    ADL2_Adapter_AdapterInfo_Get(m_context, infos,
                                 static_cast<int>(sizeof(AdapterInfo) * static_cast<size_t>(numAdapters)));

    for (int i = 0; i < numAdapters; i++) {
        if (infos[i].iVendorID == 1002 && infos[i].iPresent) {
            m_adapterIndex = infos[i].iAdapterIndex;
            break;
        }
    }
    delete[] infos;

    if (ADL2_Overdrive_Caps) {
        int supported = 0, enabled = 0;
        ADL2_Overdrive_Caps(m_context, m_adapterIndex, &supported, &enabled, &m_odVersion);
    }

    IsInitialized = true;
    return m_adapterIndex != -1;
}


[[nodiscard]] GPUData AMDMonitor::Query() {
    GPUData data = {0, 0};

    switch (m_odVersion) {
        case 6: {
            int temperature = 0;
            if (ADL2_Overdrive6_Temperature_Get &&
                ADL2_Overdrive6_Temperature_Get(m_context, m_adapterIndex, &temperature) == 0) {
                data.temperature = static_cast<float>(temperature) / 1000.0f;
                }
            ADLOD6CurrentStatus status = {};
            if (ADL2_Overdrive6_CurrentStatus_Get &&
                ADL2_Overdrive6_CurrentStatus_Get(m_context, m_adapterIndex, &status) == 0) {
                data.load = static_cast<float>(status.iActivityPercent);
                }
            break;
        }
        case 7: {
            int temperature = 0;
            if (ADL2_OverdriveN_Temperature_Get &&
                ADL2_OverdriveN_Temperature_Get(m_context, m_adapterIndex, 1, &temperature) == 0) {
                data.temperature = static_cast<float>(temperature) / 1000.0f;
                }
            ADLODNPerformanceStatus perfStatus = {};
            if (ADL2_OverdriveN_PerformanceStatus_Get &&
                ADL2_OverdriveN_PerformanceStatus_Get(m_context, m_adapterIndex, &perfStatus) == 0) {
                data.load = static_cast<float>(perfStatus.iGPUActivityPercent);
                }
            break;
        }
        case 8: {
            ADLPMLogData pmData = {};
            if (ADL2_New_QueryPMLogData_Get &&
                ADL2_New_QueryPMLogData_Get(m_context, m_adapterIndex, &pmData) == 0) {
                data.temperature = static_cast<float>(pmData.ulValues[ADL_PMLOG_TEMPERATURE_EDGE][1]);
                data.load        = static_cast<float>(pmData.ulValues[ADL_PMLOG_INFO_ACTIVITY_GFX][1]);
                }
            break;
        }
        default: {
            std::cerr << "unknown OD version: " << m_odVersion << std::endl;
            break;
        }
    }

    return data;
}


void AMDMonitor::Shutdown() {
    if (ADL2_Main_Control_Destroy && m_context) {
        ADL2_Main_Control_Destroy(m_context);
        m_context = nullptr;
    }
    if (m_hDLL) {
        FreeLibrary(m_hDLL);
        m_hDLL = nullptr;
    }
}
