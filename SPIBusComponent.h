#pragma once

#include <cstdint>
#include <deki/SetupComponent.h>
#include <deki/reflection/Property.h>
#include "IDekiSPI.h"

namespace DekiSpi
{

/// Boot-scene component that starts one physical SPI bus and registers it in
/// DekiSPI, where device drivers (a display, an SD card, a radio) find it by
/// host and add their devices to it.
///
/// Use one per physical bus, before every device on it in the setup list:
/// whoever starts a bus sets the longest transfer it takes, and a display
/// sends its pixels in long ones.
DEKI_CATEGORY("System")
DEKI_DESCRIPTION("Starts one SPI bus at boot. Every device on that bus (display, SD card, radio) shares it.")
class SPIBusComponent : public Deki::SetupComponent
{
public:
    DEKI_EXPORT
    DEKI_TOOLTIP("Which of the chip's SPI controllers to use, as the platform numbers them. On an ESP32-S3: 1 = SPI2, "
                 "2 = SPI3.")
    DEKI_RANGE(0, 3)
    int32_t spiHost = 1;

    DEKI_EXPORT
    DEKI_TOOLTIP("GPIO carrying the clock (SCK). Shared by every device on this bus.")
    DEKI_RANGE(-1, 48)
    int32_t sclkPin = -1;

    DEKI_EXPORT
    DEKI_TOOLTIP("GPIO carrying data out (MOSI); IO0 on a quad bus.")
    DEKI_RANGE(-1, 48)
    int32_t mosiPin = -1;

    DEKI_EXPORT
    DEKI_TOOLTIP("GPIO carrying data in (MISO); IO1 on a quad bus. -1 when nothing on the bus is read.")
    DEKI_RANGE(-1, 48)
    int32_t misoPin = -1;

    DEKI_EXPORT
    DEKI_TOOLTIP("IO2, for a quad bus (an AMOLED panel over QSPI). -1 on an ordinary SPI bus.")
    DEKI_RANGE(-1, 48)
    int32_t io2Pin = -1;

    DEKI_EXPORT
    DEKI_TOOLTIP("IO3, for a quad bus. -1 on an ordinary SPI bus.")
    DEKI_RANGE(-1, 48)
    int32_t io3Pin = -1;

    DEKI_EXPORT
    DEKI_TOOLTIP("The longest transfer the bus takes in one go. A display sends pixels in transfers this long, so "
                 "keep it at the chip's limit (32768 on an ESP32-S3) on a bus with a display.")
    DEKI_RANGE(64, 262144)
    int32_t maxTransferBytes = 32768;

    SPIBusComponent() = default;
    virtual ~SPIBusComponent() = default;

    void Setup(SetupCallback onComplete) override;
    const char* GetSetupName() const override { return "SPI Bus"; }

private:
    IDekiSPIBus* m_Bus = nullptr;
};

}  // namespace DekiSpi
