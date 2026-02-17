#if defined(_WIN32)
    #if defined(COMMON_API_EXPORTS)
        #define GW_API __declspec(dllexport)
    #else
        #define GW_API __declspec(dllimport)
    #endif
#else
    #define GW_API
#endif