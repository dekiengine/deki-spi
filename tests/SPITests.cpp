// deki-spi: the bus registry device drivers find their bus in, and
// SPIBusComponent, which starts a bus from the boot scene and registers it.
// The bus is a fake made by the factory a platform package would register.

#include <gtest/gtest.h>

#include "DekiSPI.h"
#include "SPIBusComponent.h"

#include <deki/PackageConfig.h>

#include <memory>
#include <string>
#include <vector>

using namespace DekiSpi;

namespace
{

struct BusLog
{
    Deki::PackageConfig config;
    int created = 0;
    int initialized = 0;
    bool failInitialize = false;
};
BusLog g_Log;

// Records what a driver asks of its device.
class FakeDevice : public IDekiSPIDevice
{
public:
    explicit FakeDevice(const DeviceConfig& c) : config(c) {}
    bool Transmit(const Transfer&) override { return true; }
    bool Queue(const Transfer&) override { return true; }
    bool Wait(size_t, uint32_t) override { return true; }
    size_t InFlight() const override { return 0; }
    bool Acquire() override { return true; }
    void Release() override {}
    size_t GetMaxTransferBytes() const override { return 32768; }
    uint32_t GetClockHz() const override { return config.clockHz; }
    DeviceConfig config;
};

class FakeBus : public IDekiSPIBus
{
public:
    const char* GetPackageId() const override { return "spi"; }
    const char* GetPackageName() const override { return "Fake SPI"; }
    void Configure(const Deki::PackageConfig& config) override { g_Log.config = config; }
    bool Initialize() override
    {
        ++g_Log.initialized;
        return !g_Log.failInitialize;
    }
    void Shutdown() override {}
    void Update(float) override {}
    Deki::PackageState GetState() const override { return Deki::PackageState::Running; }
    const char* GetLastError() const override { return g_Log.failInitialize ? "refused" : ""; }
    int GetHost() const override { return g_Log.config.GetInt("spiHost", -1); }
    int GetSclkPin() const override { return g_Log.config.GetPin("SCLK"); }
    int GetMosiPin() const override { return g_Log.config.GetPin("MOSI"); }
    int GetMisoPin() const override { return g_Log.config.GetPin("MISO"); }
    bool IsQuad() const override { return g_Log.config.GetPin("IO2") >= 0 && g_Log.config.GetPin("IO3") >= 0; }
    size_t GetMaxTransferBytes() const override
    {
        return static_cast<size_t>(g_Log.config.GetInt("maxTransferBytes", 0));
    }
    IDekiSPIDevice* AddDevice(const DeviceConfig& config) override
    {
        devices.push_back(std::make_unique<FakeDevice>(config));
        return devices.back().get();
    }
    std::vector<std::unique_ptr<FakeDevice>> devices;
};

IDekiSPIBus* CreateFakeBus()
{
    ++g_Log.created;
    return new FakeBus();
}

class SPITest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        g_Log = BusLog{};
        DekiSPI::SetFactory(&CreateFakeBus);
    }
    void TearDown() override
    {
        DekiSPI::SetFactory(nullptr);
        for (int host = 0; host < DekiSPI::kMaxBuses; ++host)
        {
            delete DekiSPI::GetBus(host);
            DekiSPI::RegisterBus(host, nullptr);
        }
    }

    // The T-Deck Plus's shared bus: display, SD card and radio on SPI2.
    static void WireTDeck(SPIBusComponent& c)
    {
        c.spiHost = 1;
        c.sclkPin = 40;
        c.mosiPin = 41;
        c.misoPin = 38;
    }

    // Runs Setup and returns what it reported.
    static bool RunSetup(SPIBusComponent& c)
    {
        bool reported = false;
        bool called = false;
        c.Setup(
            [&](bool ok)
            {
                reported = ok;
                called = true;
            });
        EXPECT_TRUE(called);
        return reported;
    }
};

}  // namespace

// --- Registry ---

TEST_F(SPITest, BusesAreFoundByHost)
{
    FakeBus a;
    FakeBus b;
    DekiSPI::RegisterBus(1, &a);
    DekiSPI::RegisterBus(2, &b);
    EXPECT_EQ(DekiSPI::GetBus(1), &a);
    EXPECT_EQ(DekiSPI::GetBus(2), &b);
    EXPECT_EQ(DekiSPI::GetBus(0), nullptr);
    DekiSPI::RegisterBus(1, nullptr);
    DekiSPI::RegisterBus(2, nullptr);
}

TEST_F(SPITest, HostsOutsideTheRegistryAreRefused)
{
    FakeBus a;
    DekiSPI::RegisterBus(-1, &a);
    DekiSPI::RegisterBus(DekiSPI::kMaxBuses, &a);
    EXPECT_EQ(DekiSPI::GetBus(-1), nullptr);
    EXPECT_EQ(DekiSPI::GetBus(DekiSPI::kMaxBuses), nullptr);
}

TEST_F(SPITest, CreateNeedsAFactory)
{
    DekiSPI::SetFactory(nullptr);
    EXPECT_FALSE(DekiSPI::HasFactory());
    EXPECT_EQ(DekiSPI::Create(), nullptr);
    DekiSPI::SetFactory(&CreateFakeBus);
    EXPECT_TRUE(DekiSPI::HasFactory());
    IDekiSPIBus* bus = DekiSPI::Create();
    EXPECT_NE(bus, nullptr);
    delete bus;
}

// --- SPIBusComponent ---

TEST_F(SPITest, SetupConfiguresAndRegistersTheBus)
{
    SPIBusComponent c;
    WireTDeck(c);
    c.maxTransferBytes = 16384;
    EXPECT_TRUE(RunSetup(c));
    EXPECT_EQ(g_Log.config.packageId, "spi");
    EXPECT_EQ(g_Log.config.GetPin("SCLK"), 40);
    EXPECT_EQ(g_Log.config.GetPin("MOSI"), 41);
    EXPECT_EQ(g_Log.config.GetPin("MISO"), 38);
    EXPECT_EQ(g_Log.config.GetPin("IO2"), -1);
    EXPECT_EQ(g_Log.config.GetPin("IO3"), -1);
    EXPECT_EQ(g_Log.config.GetInt("spiHost"), 1);
    EXPECT_EQ(g_Log.config.GetInt("maxTransferBytes"), 16384);
    ASSERT_NE(DekiSPI::GetBus(1), nullptr);
    EXPECT_FALSE(DekiSPI::GetBus(1)->IsQuad());
}

TEST_F(SPITest, AQuadBusPassesBothExtraLines)
{
    SPIBusComponent c;
    c.spiHost = 1;
    c.sclkPin = 47;
    c.mosiPin = 18;
    c.misoPin = 7;
    c.io2Pin = 48;
    c.io3Pin = 5;
    EXPECT_TRUE(RunSetup(c));
    ASSERT_NE(DekiSPI::GetBus(1), nullptr);
    EXPECT_TRUE(DekiSPI::GetBus(1)->IsQuad());
}

TEST_F(SPITest, HalfAQuadBusIsRefused)
{
    SPIBusComponent c;
    WireTDeck(c);
    c.io2Pin = 48;
    EXPECT_FALSE(RunSetup(c));
    EXPECT_EQ(g_Log.created, 0);
    EXPECT_EQ(DekiSPI::GetBus(1), nullptr);
}

TEST_F(SPITest, ABusNeedsAClockAndADataLine)
{
    SPIBusComponent noClock;
    WireTDeck(noClock);
    noClock.sclkPin = -1;
    EXPECT_FALSE(RunSetup(noClock));

    SPIBusComponent noData;
    WireTDeck(noData);
    noData.mosiPin = -1;
    noData.misoPin = -1;
    EXPECT_FALSE(RunSetup(noData));

    // Write-only is a bus: a display has no MISO.
    SPIBusComponent writeOnly;
    WireTDeck(writeOnly);
    writeOnly.misoPin = -1;
    EXPECT_TRUE(RunSetup(writeOnly));
}

TEST_F(SPITest, AHostOutOfRangeIsRefusedBeforeTheBusIsMade)
{
    SPIBusComponent c;
    WireTDeck(c);
    c.spiHost = DekiSPI::kMaxBuses;
    EXPECT_FALSE(RunSetup(c));
    EXPECT_EQ(g_Log.created, 0);
}

TEST_F(SPITest, ABusThatFailsToStartIsNotRegistered)
{
    g_Log.failInitialize = true;
    SPIBusComponent c;
    WireTDeck(c);
    EXPECT_FALSE(RunSetup(c));
    EXPECT_EQ(DekiSPI::GetBus(1), nullptr);
}

TEST_F(SPITest, SetupFailsWithoutABackend)
{
    DekiSPI::SetFactory(nullptr);
    SPIBusComponent c;
    WireTDeck(c);
    EXPECT_FALSE(RunSetup(c));
    EXPECT_EQ(DekiSPI::GetBus(1), nullptr);
}

TEST_F(SPITest, TwoComponentsCannotStartOneHost)
{
    SPIBusComponent first;
    WireTDeck(first);
    EXPECT_TRUE(RunSetup(first));
    IDekiSPIBus* bus = DekiSPI::GetBus(1);

    SPIBusComponent second;
    WireTDeck(second);
    EXPECT_FALSE(RunSetup(second));
    EXPECT_EQ(DekiSPI::GetBus(1), bus);
    EXPECT_EQ(g_Log.created, 1);
}

TEST_F(SPITest, RunningSetupAgainKeepsTheSameBus)
{
    SPIBusComponent c;
    WireTDeck(c);
    EXPECT_TRUE(RunSetup(c));
    IDekiSPIBus* bus = DekiSPI::GetBus(1);
    EXPECT_TRUE(RunSetup(c));
    EXPECT_EQ(DekiSPI::GetBus(1), bus);
    EXPECT_EQ(g_Log.created, 1);
    EXPECT_EQ(g_Log.initialized, 2);
}

TEST_F(SPITest, DevicesShareTheBusTheyFindByHost)
{
    SPIBusComponent c;
    WireTDeck(c);
    ASSERT_TRUE(RunSetup(c));
    IDekiSPIBus* bus = DekiSPI::GetBus(1);
    ASSERT_NE(bus, nullptr);

    DeviceConfig display;
    display.csPin = 12;
    display.dcPin = 11;
    display.clockHz = 80000000;
    DeviceConfig radio;
    radio.csPin = 9;
    radio.clockHz = 8000000;
    radio.halfDuplex = false;
    IDekiSPIDevice* d = bus->AddDevice(display);
    IDekiSPIDevice* r = bus->AddDevice(radio);
    ASSERT_NE(d, nullptr);
    ASSERT_NE(r, nullptr);
    EXPECT_NE(d, r);
    EXPECT_EQ(d->GetClockHz(), 80000000u);
    EXPECT_EQ(r->GetClockHz(), 8000000u);
}

// --- Defaults ---

TEST(SPIDefaults, ATransferIsOneLineWithNoPhasesAndLeavesDcAlone)
{
    Transfer t;
    EXPECT_EQ(t.lines, Lines::Single);
    EXPECT_EQ(t.cmdBits, 0);
    EXPECT_EQ(t.addrBits, 0);
    EXPECT_EQ(t.dc, -1);
    EXPECT_FALSE(t.keepCs);
}

TEST(SPIDefaults, ADeviceIsHalfDuplexWithNoPinsAndAQueue)
{
    DeviceConfig d;
    EXPECT_EQ(d.csPin, -1);
    EXPECT_EQ(d.dcPin, -1);
    EXPECT_TRUE(d.halfDuplex);
    EXPECT_GT(d.queueDepth, 1);
}
