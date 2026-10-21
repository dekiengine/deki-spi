#include "DekiSPI.h"
#include <deki/LogSystem.h>

namespace DekiSpi
{

DekiSPI::Factory DekiSPI::s_Factory = nullptr;
IDekiSPIBus* DekiSPI::s_Buses[DekiSPI::kMaxBuses] = {};

void DekiSPI::SetFactory(Factory factory)
{
    s_Factory = factory;
    DEKI_LOG_INTERNAL("DekiSPI: Factory registered");
}

IDekiSPIBus* DekiSPI::Create()
{
    if (!s_Factory)
    {
        DEKI_LOG_ERROR("DekiSPI: No factory registered - platform package must call SetFactory()");
        return nullptr;
    }
    return s_Factory();
}

bool DekiSPI::HasFactory()
{
    return s_Factory != nullptr;
}

void DekiSPI::RegisterBus(int host, IDekiSPIBus* bus)
{
    if (host < 0 || host >= kMaxBuses)
    {
        DEKI_LOG_ERROR("DekiSPI::RegisterBus: host %d out of range [0,%d)", host, kMaxBuses);
        return;
    }
    s_Buses[host] = bus;
}

IDekiSPIBus* DekiSPI::GetBus(int host)
{
    if (host < 0 || host >= kMaxBuses)
    {
        return nullptr;
    }
    return s_Buses[host];
}

}  // namespace DekiSpi
