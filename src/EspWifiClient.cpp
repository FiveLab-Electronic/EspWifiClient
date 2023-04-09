#include "EspWifiClient.h"

bool EspWifiClient::tick() {
    bool result = AtStream::tick();

    if (result) {
        return true;
    }

    if (_flags & AtStreamFlags_ReceivedResponse && _flags & EspWifiClientFlags_Connect) {
        AT_STREAM_CLEAR_BIT(_flags, EspWifiClientFlags_Connect);

        // Process connect.
        size_t bufferSize = strlen(_responseData.buffer);
        size_t startLinePosition = 0;

        for (size_t i = 0; i < bufferSize; i++) {
            if (_responseData.buffer[i] == '\n') {
                // Found line separator. Check.
                size_t lineSize = i - startLinePosition;

                if (lineSize) {
                    processResponseLine(&_responseData.buffer[startLinePosition], i - startLinePosition);
                }

                startLinePosition = i + 1;
            }
        }

        return true;
    }

    return false;
}

bool EspWifiClient::ready() {
    bool result = AtStream::ready();

    if (result) {
        if (_flags & AtStreamFlags_ReceivedResponse && _flags & EspWifiClientFlags_Connect) {
            // Must set flags for connected or not.
            return false;
        }
    }

    return result;
}

AtStreamResult EspWifiClient::version() {
    return commandExecute("GMR");
}

AtStreamResult EspWifiClient::restart() {
    return commandExecute("RST");
}

AtStreamResult EspWifiClient::restore() {
    return commandExecute("RESTORE");
}

AtStreamResult EspWifiClient::disconnect() {
    _networkState = _networkState & ~(EspWifiClientNetworkState_Connected | EspWifiClientNetworkState_GotIp);

    return commandExecute("CWQAP");
}

AtStreamResult EspWifiClient::setMode(EspWifiClientMode mode) {
    AtStreamArgument arguments[1] = {
        {AtStreamArgumentType_Integer, nullptr, (uint8_t) mode}
    };

    return commandExecute("CWMODE_CUR", arguments, 1);
}

AtStreamResult EspWifiClient::scanNetworksOptions(bool orderByRssi) {
    AtStreamArgument arguments[2] = {
        {AtStreamArgumentType_Integer, nullptr, (uint8_t) (orderByRssi ? 1 : 0)},
        {AtStreamArgumentType_Integer, nullptr, 0b01111111}
    };

    return commandExecute("CWLAPOPT", arguments, 2);
}

AtStreamResult EspWifiClient::scanNetworks() {
    return commandExecute("CWLAP");
}

EspWifiClientNetwork *EspWifiClient::getNetworks(uint8_t &count, AtStreamResult &result) {
    result = AtStreamResult_Ok;

    if (strcmp("CWLAP", _lastCommand) != 0) {
        result = AtStreamResult_WrongCommand;

        return nullptr;
    }

    if (count == 0) {
        return nullptr;
    }

    uint8_t parsedNetworks = 0;
    auto networks = (EspWifiClientNetwork *) malloc(sizeof(EspWifiClientNetwork) * parsedNetworks);

    uint16_t responseBufferSize = strlen(_responseData.buffer);

    for (uint16_t i = 0; i < responseBufferSize; i++) {
        if (memcmp("\n+CWLAP:", &_responseData.buffer[i], 8) == 0) {
            // Found "+CWLAP:(.....)" instruction. It's info about possible network.
            uint16_t startNetworkInfoPoint = i + 9; // Ignore start parenthesis

            auto endPositionChar = strchr(&_responseData.buffer[startNetworkInfoPoint], '\n');
            endPositionChar -= 1; // Ignore last parenthesis

            auto networkLineSize = (uint16_t) (endPositionChar - &_responseData.buffer[startNetworkInfoPoint]);

            uint8_t countArguments = 0;
            AtStreamArgument *arguments = AtStream::parseArgumentsStr(&_responseData.buffer[startNetworkInfoPoint], networkLineSize, countArguments);

            networks = (EspWifiClientNetwork *) realloc(networks, sizeof(EspWifiClientNetwork) * (parsedNetworks + 1));

            networks[parsedNetworks] = {
                (EspWifiClientNetworkMode) arguments[0].intValue,
                strdup(arguments[1].strPtr),
                arguments[2].intValue
            };

            parsedNetworks++;

            AtStream::freeArguments(arguments, countArguments);

            if (parsedNetworks == count) {
                // Finish parse.
                break;
            }
        }
    }

    count = parsedNetworks;

    return networks;
}

void EspWifiClient::freeNetworks(EspWifiClientNetwork *&networks, uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        free((char *) networks[i].ssid);
    }

    free(networks);
    networks = nullptr;
}

AtStreamResult EspWifiClient::connect(const char *ssid) {
    AT_STREAM_SET_BIT(_flags, EspWifiClientFlags_Connect);

    AtStreamArgument arguments[2] = {
        {AtStreamArgumentType_String, ssid},
        {AtStreamArgumentType_String, ""}
    };

    return commandExecute("CWJAP", arguments, 2);
}

AtStreamResult EspWifiClient::connect(const char *ssid, const char *password) {
    AT_STREAM_SET_BIT(_flags, EspWifiClientFlags_Connect);

    AtStreamArgument arguments[2] = {
        {AtStreamArgumentType_String, ssid},
        {AtStreamArgumentType_String, password}
    };

    return commandExecute("CWJAP", arguments, 2);
}

AtStreamResult EspWifiClient::localNetInfo() {
    if (!(_networkState & EspWifiClientNetworkState_Connected) || !(_networkState & EspWifiClientNetworkState_GotIp)) {
        // Not connected or not received IP address.
        return AtStreamResult_WrongCommand;
    }

    return commandExecute("CIFSR");
}

AtStreamResult EspWifiClient::useDns(const char *dns) {
    AtStreamArgument arguments[1] = {
        {AtStreamArgumentType_String, dns}
    };

    return commandExecute("CIPDOMAIN", arguments, 1);
}

void EspWifiClient::receiveNotRelatedLine() {
    processResponseLine(_responseLine.buffer, strlen(_responseLine.buffer));
}

void EspWifiClient::processResponseLine(const char *buffer, size_t size) {
    if (size == 15 && memcmp("WIFI DISCONNECT", buffer, size) == 0) {
        _networkState = _networkState & ~(EspWifiClientNetworkState_Connected | EspWifiClientNetworkState_GotIp);
    }

    if (size == 14 && memcmp("WIFI CONNECTED", buffer, size) == 0) {
        _networkState = _networkState | EspWifiClientNetworkState_Connected;
    }

    if (size == 11 && memcmp("WIFI GOT IP", buffer, size) == 0) {
        _networkState = _networkState | EspWifiClientNetworkState_GotIp;
    }

    // @todo Process "DHCP TIMEOUT"?
}
