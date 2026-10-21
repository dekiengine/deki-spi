// Package entry point for deki-spi.
#include "DekiSPIPackage.h"
#include <deki/interop/Plugin.h>
#include <deki/LogSystem.h>

extern void DekiSPIRegisterComponents();
extern int DekiSPIGetAutoComponentCount();
extern const Deki::ComponentMeta* DekiSPIGetAutoComponentMeta(int index);

namespace DekiSpi
{

#ifdef DEKI_EDITOR
#endif

static bool s_SPIRegistered = false;

}  // namespace DekiSpi
// The exports below are C symbols at global scope; the package's own
// registration helpers and statics live in its namespace.
using namespace DekiSpi;

extern "C"
{
    DEKI_SPI_API int DekiSPIEnsureRegistered(void)
    {
#ifdef DEKI_EDITOR
        if (s_SPIRegistered)
        {
            return ::DekiSPIGetAutoComponentCount();
        }
        s_SPIRegistered = true;
        ::DekiSPIRegisterComponents();
        return ::DekiSPIGetAutoComponentCount();
#else
        return 0;
#endif
    }

    DEKI_PLUGIN_API const char* DekiPluginGetName(void)
    {
        return "Deki SPI Package";
    }
    DEKI_PLUGIN_API const char* DekiPluginGetVersion(void)
    {
#ifdef DEKI_PACKAGE_VERSION
        return DEKI_PACKAGE_VERSION;
#else
        return "0.0.0-dev";
#endif
    }
    DEKI_PLUGIN_API int DekiPluginInit(void)
    {
        return 0;
    }
    DEKI_PLUGIN_API void DekiPluginShutdown(void)
    {
        s_SPIRegistered = false;
    }

#ifdef DEKI_EDITOR
    DEKI_PLUGIN_API int DekiPluginGetComponentCount(void)
    {
        return ::DekiSPIGetAutoComponentCount();
    }
    DEKI_PLUGIN_API const Deki::ComponentMeta* DekiPluginGetComponentMeta(int index)
    {
        return ::DekiSPIGetAutoComponentMeta(index);
    }
#else
    DEKI_PLUGIN_API int DekiPluginGetComponentCount(void)
    {
        return 0;
    }
    DEKI_PLUGIN_API const Deki::ComponentMeta* DekiPluginGetComponentMeta(int)
    {
        return nullptr;
    }
#endif

    DEKI_PLUGIN_API void DekiPluginRegisterComponents(void)
    {
#ifdef DEKI_EDITOR
        DekiSPIEnsureRegistered();
#endif
    }

}  // extern "C"
