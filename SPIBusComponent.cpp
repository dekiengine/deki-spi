#include "SPIBusComponent.h"
#include "DekiSPI.h"
#include <deki/PackageConfig.h>
#include <deki/LogSystem.h>
#include <string>

namespace DekiSpi
{

void SPIBusComponent::Setup(SetupCallback onComplete)
{
    auto finish = [&](bool ok)
    {
        if (onComplete)
        {
            onComplete(ok);
        }
    };

    // A host the registry cannot hold would start the bus and register
    // nothing, and every device on it would then fail with "no SPI bus".
    if (spiHost < 0 || spiHost >= DekiSPI::kMaxBuses)
    {
        DEKI_LOG_ERROR("SPIBusComponent: SPI host %d is out of range (0-%d)", (int)spiHost, DekiSPI::kMaxBuses - 1);
        finish(false);
        return;
    }
    if (sclkPin < 0 || (mosiPin < 0 && misoPin < 0))
    {
        DEKI_LOG_ERROR("SPIBusComponent: host %d needs a clock pin and a data pin", (int)spiHost);
        finish(false);
        return;
    }
    // Half a quad bus is a wiring mistake, not a bus with two lines.
    if ((io2Pin < 0) != (io3Pin < 0))
    {
        DEKI_LOG_ERROR("SPIBusComponent: host %d has IO2 or IO3 but not both; a quad bus needs both", (int)spiHost);
        finish(false);
        return;
    }
    IDekiSPIBus* existing = DekiSPI::GetBus(spiHost);
    if (existing && existing != m_Bus)
    {
        DEKI_LOG_ERROR("SPIBusComponent: SPI host %d is already started by another SPI Bus component", (int)spiHost);
        finish(false);
        return;
    }

    if (!m_Bus)
    {
        m_Bus = DekiSPI::Create();
        if (!m_Bus)
        {
            DEKI_LOG_ERROR("SPIBusComponent: No SPI backend registered");
            finish(false);
            return;
        }
    }

    Deki::PackageConfig config;
    config.packageId = "spi";
    config.enabled = true;
    config.pins["SCLK"] = sclkPin;
    config.pins["MOSI"] = mosiPin;
    config.pins["MISO"] = misoPin;
    config.pins["IO2"] = io2Pin;
    config.pins["IO3"] = io3Pin;
    config.settings["spiHost"] = std::to_string(spiHost);
    config.settings["maxTransferBytes"] = std::to_string(maxTransferBytes);

    m_Bus->Configure(config);

    const bool success = m_Bus->Initialize();
    if (success)
    {
        DekiSPI::RegisterBus(spiHost, m_Bus);
    }
    else
    {
        DEKI_LOG_ERROR("SPIBusComponent: Failed to start SPI host %d: %s", (int)spiHost, m_Bus->GetLastError());
    }
    finish(success);
}

}  // namespace DekiSpi
