#pragma once

#include <deki/providers/IPackage.h>
#include <cstddef>
#include <cstdint>

namespace DekiSpi
{

/// Data lines a transfer's data phase uses. Dual and Quad need a bus with
/// IO2 and IO3 wired (IDekiSPIBus::IsQuad) and a half-duplex device.
enum class Lines : uint8_t
{
    Single = 1,
    Dual = 2,
    Quad = 4
};

/// One device on a bus: its chip select, its clock, and the pin a display
/// uses to tell commands from data.
struct DeviceConfig
{
    /// -1 when the device has no chip select (a panel with CS tied low).
    int32_t csPin = -1;
    /// Data/command pin, driven per transfer (Transfer::dc) as that transfer
    /// starts on the wire, so a queued run of transfers can switch it.
    /// -1 when the device has none.
    int32_t dcPin = -1;
    uint32_t clockHz = 1000000;
    /// SPI mode 0-3 (clock polarity and phase).
    uint8_t mode = 0;
    /// Sends, then receives, instead of both at once. A display only writes;
    /// on ESP-IDF this is what allows 80 MHz on pins routed through the GPIO
    /// matrix, which full duplex limits to 40 MHz.
    bool halfDuplex = true;
    /// Transfers that may be queued at once (Queue).
    uint8_t queueDepth = 8;
};

/// One transaction: optional command and address phases on one line, then
/// the data. Sizes are in bytes, phases in bits.
struct Transfer
{
    const void* tx = nullptr;
    size_t txBytes = 0;
    void* rx = nullptr;
    /// Half duplex: read after the write. Full duplex: read while writing,
    /// and at most txBytes.
    size_t rxBytes = 0;
    Lines lines = Lines::Single;
    /// A command phase before the data, sent on one line: 0 for none. QSPI
    /// displays take 8 bits here (0x02 for a register write, 0x32 for pixels).
    uint8_t cmdBits = 0;
    uint16_t cmd = 0;
    /// An address phase after the command, on one line: 0 for none. QSPI
    /// displays take 24 bits, the register in the middle byte.
    uint8_t addrBits = 0;
    uint32_t addr = 0;
    /// Level of the device's DC pin while this transfer is clocked: 0 for a
    /// command, 1 for data, -1 to leave it.
    int8_t dc = -1;
    /// Leaves chip select asserted after this transfer, so the next one
    /// continues it (pixels sent in pieces). Only while the bus is held
    /// (Acquire).
    bool keepCs = false;
};

/// A device on an SPI bus, from IDekiSPIBus::AddDevice. Small transfers
/// (commands, sensor reads) go through Transmit; long writes (pixels) are
/// queued, go out by DMA one after the other, and are waited for.
class IDekiSPIDevice
{
public:
    virtual ~IDekiSPIDevice() = default;

    /// Sends `t` and returns when it is done. Waits for any queued transfers
    /// first.
    virtual bool Transmit(const Transfer& t) = 0;

    /// Queues `t` and returns once it is queued; it goes out by DMA after the
    /// transfers queued before it. `t.tx` must stay valid and unchanged until
    /// Wait says the transfer is done. A buffer outside DMA-capable memory
    /// works, but the platform's driver may copy it first. Blocks while the
    /// queue is full. False when the transfer is longer than
    /// GetMaxTransferBytes, or cannot be queued.
    virtual bool Queue(const Transfer& t) = 0;

    /// Waits until no more than `leaveInFlight` queued transfers are still
    /// in flight, so the buffer of the oldest can be used again. 0 waits for
    /// them all. False on a timeout or a failed transfer.
    virtual bool Wait(size_t leaveInFlight = 0, uint32_t timeoutMs = UINT32_MAX) = 0;

    /// Queued transfers not yet waited for.
    virtual size_t InFlight() const = 0;

    /// Holds the bus for this device until Release, so a run of transfers is
    /// not split by another device's. Other devices on the bus wait.
    /// Calls nest.
    virtual bool Acquire() = 0;
    virtual void Release() = 0;

    /// The longest transfer the bus takes in one go (ESP32-S3: 32 KB).
    virtual size_t GetMaxTransferBytes() const = 0;
    /// The clock the device actually runs at, which a divider may round.
    virtual uint32_t GetClockHz() const = 0;
};

/// An SPI bus, as a platform provides it. SPIBusComponent creates one from
/// the boot scene and registers it in DekiSPI, where device drivers find it
/// by host.
///
/// Host numbers are the platform's own (ESP-IDF: 1 = SPI2, 2 = SPI3 on an
/// ESP32-S3), as pins are.
class IDekiSPIBus : public Deki::IPackage
{
public:
    const char* GetPackageCategory() const override { return "spi"; }

    virtual int GetHost() const = 0;
    virtual int GetSclkPin() const = 0;
    virtual int GetMosiPin() const = 0;
    virtual int GetMisoPin() const = 0;
    /// True when IO2 and IO3 are wired, for quad transfers.
    virtual bool IsQuad() const = 0;
    virtual size_t GetMaxTransferBytes() const = 0;

    /// Adds a device to the started bus. The bus owns it and frees it on
    /// Shutdown. nullptr when the platform refuses it (no free CS slot, a
    /// clock it cannot make).
    virtual IDekiSPIDevice* AddDevice(const DeviceConfig& config) = 0;
};

}  // namespace DekiSpi
