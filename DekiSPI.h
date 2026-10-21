#pragma once

#include "IDekiSPI.h"
#include "DekiSPIPackage.h"

namespace DekiSpi
{

/// Creates SPI buses and finds them by host.
///
/// - The platform package registers a factory with SetFactory().
/// - SPIBusComponent (one per physical bus in the boot scene) calls Create(),
///   then Configure() and Initialize(), then RegisterBus(host, bus).
/// - Device drivers (a display, an SD card, a radio) call GetBus(host) and
///   add their device to it.
class DEKI_SPI_API DekiSPI
{
public:
    using Factory = IDekiSPIBus* (*)();

    static void SetFactory(Factory factory);
    static IDekiSPIBus* Create();
    static bool HasFactory();

    static void RegisterBus(int host, IDekiSPIBus* bus);
    static IDekiSPIBus* GetBus(int host);

    static constexpr int kMaxBuses = 4;

private:
    static Factory s_Factory;
    static IDekiSPIBus* s_Buses[kMaxBuses];
};

}  // namespace DekiSpi
