# Deki SPI

SPI bus interface for the Deki Engine. Defines `IDekiSPIBus` (a bus and its pins) and `IDekiSPIDevice` (a device on it: blocking transfers for commands and sensor reads, queued DMA transfers for long writes such as a display's pixels, and holding the bus for a run of them), plus an `SPIBusComponent` that starts a bus from the boot scene.

A transfer can carry a command and an address phase and send its data on one, two or four lines, so a QSPI display panel is driven through the same interface. A device's data/command pin is set per transfer, as that transfer starts on the wire.

Several devices share one bus: start it once with `SPIBusComponent`, before every device on it, and each driver adds its device with `DekiSPI::GetBus(host)->AddDevice(...)`. The platform package supplies the backend (ESP-IDF's `spi_master` in `deki-esp32-integration`).

Part of [Deki Engine](https://github.com/dekiengine/deki-engine).

## Namespace

Types live in `DekiSpi`. Scene files store the qualified name, and so does code:

```cpp
using namespace DekiSpi;
IDekiSPIBus* bus = DekiSPI::GetBus(1);
```

## Install

Package Manager in the Deki Editor, or `DekiEditor --packages-add deki-spi <project>`.

## License

Apache 2.0. See [LICENSE](LICENSE).
