#ifndef cuncuno_h
#define cuncuno_h

// TODO Use C or C++ as interface????

#ifdef _WIN32
        #ifdef cuno_EXPORTS
                #define cuncunoExport __declspec(dllexport)
        #else
                #define cuncunoExport __declspec(dllimport)
        #endif
        // cunExport becomes export or import.
        // cunModExport is always export and should be used for functions
        //   exported by mods.
        // TODO A system of exports from mods with structs of function pointers.
        // TODO That guarantees keeping the namespace clean.
        // TODO Then each mod could just have a single dllexport function that
        // TODO provides all setup and other exports in the struct.
        #define cuncunoModExport __declspec(dllexport)
#else
        #define cuncunoExport
        #define cuncunoModExport
#endif

namespace cuncuno {

// TODO C++ here.

}

#ifdef __cplusplus
extern "C" {
#endif

// TODO C here.

#ifdef __cplusplus
}
#endif

#endif
