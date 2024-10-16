
#if   defined (__WIN32)  \
   || defined (_WIN32)   \
   || defined (WIN32)    \
   || defined (_WIN64)   \
   || defined (__WIN64)  \
   || defined (WIN64)    \
   || defined (__WINNT)

    #define IS_WINDOWS 1
#endif

#include <string>
#include <iostream>


#ifdef IS_WINDOWS
    #include <windows.h>

    #define DLL_EXPORT extern "C" __declspec(dllexport)
#else
    #define DLL_EXPORT extern "C"
#endif


DLL_EXPORT double test_function1(const char* string, double number)
{
    double result = number;

    std::cout << "Received: " << string << std::endl;

    if (string != nullptr)
    {
        std::string str(string);
        result += str.length();
    }

    return result;
}


DLL_EXPORT int64_t test_function2(int64_t a, int64_t b)
{
    return a * b;
}


struct point
{
    int64_t x;
    int64_t y;
};


DLL_EXPORT int32_t test_function3(point pt)
{
    return (pt.x >= 0) && (pt.y >= 0) && (pt.x <= 100) && (pt.y <= 100);
}


#ifdef IS_WINDOWS
    BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
    {
        switch (ul_reason_for_call)
        {
            case DLL_PROCESS_ATTACH:
            case DLL_THREAD_ATTACH:
            case DLL_THREAD_DETACH:
            case DLL_PROCESS_DETACH:
                break;
        }
        return TRUE;
    }
#endif
