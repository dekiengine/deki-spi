# Changelog

Notable changes to `deki-spi`. Engine and editor changes are in the
[engine changelog](https://github.com/dekiengine/deki-engine/blob/master/CHANGELOG.md).

A package's `minEngine` names the engine version it needs. Before 1.0 a
breaking change bumps the minor across the editor, the engine and every
package together, so a package with no changes of its own is still released
alongside one that has them.

## Unreleased

### Added
- The package: `IDekiSPIBus` and `IDekiSPIDevice`, the `DekiSPI` registry the
  platform's factory and the buses are found in, and `SPIBusComponent`, which
  starts a bus from the boot scene. A transfer has optional command and
  address phases, one, two or four data lines, a data/command pin level and a
  chip select that can stay asserted into the next transfer; long writes are
  queued for DMA and waited for. A queued transfer of four bytes or less is
  copied, so a command can be queued from the stack.
