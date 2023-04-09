#pragma once

#include <AtStream.h>

enum EspWifiClientMode {
    EspWifiClientMode_Station = 1,
    EspWifiClientMode_AccessPoint = 2,
    EspWifiClientMode_AccessPointAndStation = 3
};

enum EspWifiClientFlags {
    EspWifiClientFlags_Connect = (1 << 4)
};

enum EspWifiClientNetworkMode {
    EspWifiClientNetworkMode_Open = 0,
    EspWifiClientNetworkMode_WEP = 1,
    EspWifiClientNetworkMode_WPA_PSK = 2,
    EspWifiClientNetworkMode_WPA2_PSK = 3,
    EspWifiClientNetworkMode_WPA_WPA2_PSK = 4
};

struct EspWifiClientNetwork {
    EspWifiClientNetworkMode mode;
    const char *ssid;
    int rssi;
};

class EspWifiClient : public AtStream {
public:
    explicit EspWifiClient(Stream &stream) : AtStream(stream) {}

    bool tick() override;
    bool ready() override;

    /**
     * Send command for retrieve version from module.
     */
    AtStreamResult version();

    /**
     * Send command for restart module.
     */
    AtStreamResult restart();

    /**
     * Send command for restore all configurations.
     */
    AtStreamResult restore();

    /**
     * Send command for disconnect from WiFi access point (AP).
     */
    AtStreamResult disconnect();

    /**
     * Set mode for module.
     */
    AtStreamResult setMode(EspWifiClientMode mode);

    /**
     * Set options for scan networks.
     */
    AtStreamResult scanNetworksOptions(bool orderByRssi);

    /**
     * Send command for scan networks.
     */
    AtStreamResult scanNetworks();

    /**
     * Get scanned networks.
     * Before get networks you must call `scanNetworks`. This method parse networks from last buffer.
     * After call next command, buffer will not contain networks info.
     * This method change `count` parameter if you try to get more networks then possible.
     */
    EspWifiClientNetwork *getNetworks(uint8_t &count, AtStreamResult &result);

    /**
     * Free memory allocated for getting networks.
     */
    static void freeNetworks(EspWifiClientNetwork *&networks, uint8_t count);

    /**
     * Send command for connect to network without password.
     */
    AtStreamResult connect(const char *ssid);

    /**
     * Send command for connect to network with password.
     */
    AtStreamResult connect(const char *ssid, const char *password);

    /**
     * Send command for use specific Domain Name Server.
     * For test and production you can use 8.8.8.8 (global NS).
     */
    AtStreamResult useDns(const char *dns);
};
