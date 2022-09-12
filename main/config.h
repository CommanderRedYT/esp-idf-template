#pragma once

#include "sdkconfig.h"

// system includes
#include <string>
#include <array>
#include <optional>

// esp-idf includes
#include <esp_sntp.h>

// 3rdparty lib includes
#include <fmt/core.h>
#include <configmanager.h>
#include <configconstraints_base.h>
#include <configconstraints_espchrono.h>
#include <configwrapper.h>
#include <espwifiutils.h>
#include <espchrono.h>
#include <makearray.h>

// local includes

using namespace espconfig;
using namespace wifi_stack;

std::string defaultHostname();

enum class AllowReset { NoReset, DoReset };
constexpr auto NoReset = AllowReset::NoReset;
constexpr auto DoReset = AllowReset::DoReset;

template<typename T>
class ConfigWrapperLegacy final : public ConfigWrapper<T>
{
    CPP_DISABLE_COPY_MOVE(ConfigWrapperLegacy)
    using Base = ConfigWrapper<T>;

public:
    using value_t = typename Base::value_t;
    using ConstraintCallback = typename Base::ConstraintCallback;

    using DefaultValueCallbackRef = T(&)();
    using DefaultValueCallbackPtr = T(*)();

    ConfigWrapperLegacy(const T &defaultValue, AllowReset allowReset, ConstraintCallback constraintCallback, const char *nvsName) :
            m_allowReset{allowReset == AllowReset::DoReset},
            m_nvsName{nvsName},
            m_defaultType{DefaultByValue},
            m_defaultValue{defaultValue},
            m_constraintCallback{constraintCallback}
    {}

    ConfigWrapperLegacy(T &&defaultValue, AllowReset allowReset, ConstraintCallback constraintCallback, const char *nvsName) :
            m_allowReset{allowReset == AllowReset::DoReset},
            m_nvsName{nvsName},
            m_defaultType{DefaultByValue},
            m_defaultValue{std::move(defaultValue)},
            m_constraintCallback{constraintCallback}
    {}

    ConfigWrapperLegacy(const ConfigWrapper<T> &factoryConfig, ConstraintCallback constraintCallback, const char *nvsName) :
            m_allowReset{true},
            m_nvsName{nvsName},
            m_defaultType{DefaultByFactoryConfig},
            m_factoryConfig{&factoryConfig},
            m_constraintCallback{constraintCallback}
    {}

    ConfigWrapperLegacy(const DefaultValueCallbackRef &defaultCallback, AllowReset allowReset, ConstraintCallback constraintCallback, const char *nvsName) :
            m_allowReset{allowReset == AllowReset::DoReset},
            m_nvsName{nvsName},
            m_defaultType{DefaultByCallback},
            m_defaultCallback{&defaultCallback},
            m_constraintCallback{constraintCallback}
    {}

    ~ConfigWrapperLegacy()
    {
        switch (m_defaultType)
        {
        case DefaultByValue:         m_defaultValue.~T(); break;
        case DefaultByFactoryConfig: /*m_factoryConfig.~typeof(m_factoryConfig)();*/ break;
        case DefaultByCallback:      /*m_defaultCallback.~typeof(m_defaultCallback)();*/ break;
        }
    }

    const char *nvsName() const override final { return m_nvsName; }
    bool allowReset() const override final { return m_allowReset; }
    T defaultValue() const override final
    {
        switch (m_defaultType)
        {
        case DefaultByValue:         return m_defaultValue;
        case DefaultByFactoryConfig: assert(m_factoryConfig->loaded()); return m_factoryConfig->value();
        case DefaultByCallback:      return m_defaultCallback();
        }

        __builtin_unreachable();
    }

    ConfigConstraintReturnType checkValue(value_t value) const override final
    {
        if (!m_constraintCallback)
            return {};

        return m_constraintCallback(value);
    }

private:
    const bool m_allowReset;
    const char * const m_nvsName;

    const enum : uint8_t { DefaultByValue, DefaultByFactoryConfig, DefaultByCallback } m_defaultType;

    union
    {
        const T m_defaultValue;
        const ConfigWrapper<T> * const m_factoryConfig;
        const DefaultValueCallbackPtr m_defaultCallback;
    };

    const ConstraintCallback m_constraintCallback;
};

class WiFiConfig
{
public:
    WiFiConfig(const char *ssidNvsKey, const char *keyNvsKey,
               const char *useStaticIpKey, const char *staticIpKey, const char *staticSubnetKey, const char *staticGatewayKey,
               const char *useStaticDnsKey, const char *staticDns0Key, const char *staticDns1Key, const char *staticDns2Key) :
        ssid         {std::string{},              DoReset, StringMaxSize<32>,                              ssidNvsKey         },
        key          {std::string{},              DoReset, StringOr<StringEmpty, StringMinMaxSize<8, 64>>, keyNvsKey          },
        useStaticIp  {false,                      DoReset, {},                                             useStaticIpKey     },
        staticIp     {wifi_stack::ip_address_t{}, DoReset, {},                                             staticIpKey        },
        staticSubnet {wifi_stack::ip_address_t{}, DoReset, {},                                             staticSubnetKey    },
        staticGateway{wifi_stack::ip_address_t{}, DoReset, {},                                             staticGatewayKey   },
        useStaticDns {false,                      DoReset, {},                                             useStaticDnsKey    },
        staticDns0   {wifi_stack::ip_address_t{}, DoReset, {},                                             staticDns0Key      },
        staticDns1   {wifi_stack::ip_address_t{}, DoReset, {},                                             staticDns1Key      },
        staticDns2   {wifi_stack::ip_address_t{}, DoReset, {},                                             staticDns2Key      }
    {}

    ConfigWrapperLegacy<std::string> ssid;
    ConfigWrapperLegacy<std::string> key;
    ConfigWrapperLegacy<bool> useStaticIp;
    ConfigWrapperLegacy<wifi_stack::ip_address_t> staticIp;
    ConfigWrapperLegacy<wifi_stack::ip_address_t> staticSubnet;
    ConfigWrapperLegacy<wifi_stack::ip_address_t> staticGateway;
    ConfigWrapperLegacy<bool> useStaticDns;
    ConfigWrapperLegacy<wifi_stack::ip_address_t> staticDns0;
    ConfigWrapperLegacy<wifi_stack::ip_address_t> staticDns1;
    ConfigWrapperLegacy<wifi_stack::ip_address_t> staticDns2;
};

class ConfigContainer
{
    using mac_t = wifi_stack::mac_t;

public:
    //                                            default                                 allowReset constraints                    nvsName
    ConfigWrapperLegacy<std::optional<mac_t>> baseMacAddressOverride{std::nullopt,              DoReset,   {},                            "baseMacAddrOver"     };
    ConfigWrapperLegacy<std::string> hostname           {defaultHostname,                       DoReset,   StringMinMaxSize<4, 32>,       "hostname"            };

#ifdef CONFIG_ETH_ENABLED
    ConfigWrapperLegacy<bool> ethEnabled                {true,                                   DoReset,   {},                         "ethEnabled"          };
    ConfigWrapperLegacy<bool> ethUseStaticIp            {false,                                  DoReset,   {},                         "ethUseStatIp"        };
    ConfigWrapperLegacy<ip_address_t> ethStaticIp       {ip_address_t{},                         DoReset,   {},                         "ethStaticIp"         };
    ConfigWrapperLegacy<ip_address_t> ethStaticSubnet   {ip_address_t{},                         DoReset,   {},                         "ethStaticSub"        };
    ConfigWrapperLegacy<ip_address_t> ethStaticGateway  {ip_address_t{},                         DoReset,   {},                         "ethStaticGw"         };
    ConfigWrapperLegacy<bool> ethUseStaticDns           {false,                                  DoReset,   {},                         "ethUseStatDns"       };
    ConfigWrapperLegacy<ip_address_t> ethStaticDns0     {ip_address_t{},                         DoReset,   {},                         "ethStaticDns0"       };
    ConfigWrapperLegacy<ip_address_t> ethStaticDns1     {ip_address_t{},                         DoReset,   {},                         "ethStaticDns1"       };
    ConfigWrapperLegacy<ip_address_t> ethStaticDns2     {ip_address_t{},                         DoReset,   {},                         "ethStaticDns2"       };
#endif

    ConfigWrapperLegacy<bool>        wifiStaEnabled     {true,                                  DoReset,   {},                            "wifiStaEnabled"      };
    std::array<WiFiConfig, 10> wifi_configs {
        WiFiConfig {"wifi_ssid0", "wifi_key0", "wifi_usestatic0", "wifi_static_ip0", "wifi_stati_sub0", "wifi_stat_gate0", "wifi_usestadns0", "wifi_stat_dnsA0", "wifi_stat_dnsB0", "wifi_stat_dnsC0"},
        WiFiConfig {"wifi_ssid1", "wifi_key1", "wifi_usestatic1", "wifi_static_ip1", "wifi_stati_sub1", "wifi_stat_gate1", "wifi_usestadns1", "wifi_stat_dnsA1", "wifi_stat_dnsB1", "wifi_stat_dnsC1"},
        WiFiConfig {"wifi_ssid2", "wifi_key2", "wifi_usestatic2", "wifi_static_ip2", "wifi_stati_sub2", "wifi_stat_gate2", "wifi_usestadns2", "wifi_stat_dnsA2", "wifi_stat_dnsB2", "wifi_stat_dnsC2"},
        WiFiConfig {"wifi_ssid3", "wifi_key3", "wifi_usestatic3", "wifi_static_ip3", "wifi_stati_sub3", "wifi_stat_gate3", "wifi_usestadns3", "wifi_stat_dnsA3", "wifi_stat_dnsB3", "wifi_stat_dnsC3"},
        WiFiConfig {"wifi_ssid4", "wifi_key4", "wifi_usestatic4", "wifi_static_ip4", "wifi_stati_sub4", "wifi_stat_gate4", "wifi_usestadns4", "wifi_stat_dnsA4", "wifi_stat_dnsB4", "wifi_stat_dnsC4"},
        WiFiConfig {"wifi_ssid5", "wifi_key5", "wifi_usestatic5", "wifi_static_ip5", "wifi_stati_sub5", "wifi_stat_gate5", "wifi_usestadns5", "wifi_stat_dnsA5", "wifi_stat_dnsB5", "wifi_stat_dnsC5"},
        WiFiConfig {"wifi_ssid6", "wifi_key6", "wifi_usestatic6", "wifi_static_ip6", "wifi_stati_sub6", "wifi_stat_gate6", "wifi_usestadns6", "wifi_stat_dnsA6", "wifi_stat_dnsB6", "wifi_stat_dnsC6"},
        WiFiConfig {"wifi_ssid7", "wifi_key7", "wifi_usestatic7", "wifi_static_ip7", "wifi_stati_sub7", "wifi_stat_gate7", "wifi_usestadns7", "wifi_stat_dnsA7", "wifi_stat_dnsB7", "wifi_stat_dnsC7"},
        WiFiConfig {"wifi_ssid8", "wifi_key8", "wifi_usestatic8", "wifi_static_ip8", "wifi_stati_sub8", "wifi_stat_gate8", "wifi_usestadns8", "wifi_stat_dnsA8", "wifi_stat_dnsB8", "wifi_stat_dnsC8"},
        WiFiConfig {"wifi_ssid9", "wifi_key9", "wifi_usestatic9", "wifi_static_ip9", "wifi_stati_sub9", "wifi_stat_gate9", "wifi_usestadns9", "wifi_stat_dnsA9", "wifi_stat_dnsB9", "wifi_stat_dnsC9"}
    };
    ConfigWrapperLegacy<int8_t>      wifiStaMinRssi     {-90,                                    DoReset,   {},                           "wifiStaMinRssi"      };

    ConfigWrapperLegacy<bool>        wifiApEnabled      {true,                                   DoReset,   {},                           "wifiApEnabled"       };
    ConfigWrapperLegacy<std::string> wifiApName         {defaultHostname,                        DoReset,   StringMinMaxSize<4, 32>,      "wifiApName"          };
    ConfigWrapperLegacy<std::string> wifiApKey          {"Passwort_123",                         DoReset,   StringOr<StringEmpty, StringMinMaxSize<8, 64>>, "wifiApKey" };
    ConfigWrapperLegacy<wifi_stack::ip_address_t> wifiApIp{wifi_stack::ip_address_t{10, 0, 0, 1},DoReset,   {},                           "wifiApIp"            };
    ConfigWrapperLegacy<wifi_stack::ip_address_t> wifiApMask{wifi_stack::ip_address_t{255, 255, 255, 0},DoReset, {},                      "wifiApMask"          };
    ConfigWrapperLegacy<uint8_t>     wifiApChannel      {1,                                      DoReset,   MinMaxValue<uint8_t, 1, 14>,  "wifiApChannel"       };
    ConfigWrapperLegacy<wifi_auth_mode_t> wifiApAuthmode{WIFI_AUTH_WPA2_PSK,                     DoReset,   {},                           "wifiApAuthmode"      };

    ConfigWrapperLegacy<bool>     timeServerEnabled     {true,                                   DoReset,   {},                           "timeServerEnabl"     };
    ConfigWrapperLegacy<std::string>   timeServer       {"europe.pool.ntp.org",                  DoReset,   StringMaxSize<64>,            "timeServer"          };
    ConfigWrapperLegacy<sntp_sync_mode_t> timeSyncMode  {SNTP_SYNC_MODE_IMMED,                   DoReset,   {},                           "timeSyncMode"        };
    ConfigWrapperLegacy<espchrono::milliseconds32> timeSyncInterval{espchrono::milliseconds32{CONFIG_LWIP_SNTP_UPDATE_DELAY}, DoReset, MinTimeSyncInterval, "timeSyncInterva" };
    ConfigWrapperLegacy<espchrono::minutes32> timezoneOffset{espchrono::minutes32{60},           DoReset,   {},                           "timezoneOffset"      }; // MinMaxValue<minutes32, -1440m, 1440m>
    ConfigWrapperLegacy<espchrono::DayLightSavingMode>timeDst{espchrono::DayLightSavingMode::EuropeanSummerTime, DoReset, {},             "time_dst"            };

    ConfigWrapperLegacy<std::string> otaUrl             {std::string{},                          DoReset,   StringOr<StringEmpty, StringValidUrl>, "otaUrl"   };

    template<typename T>
    void callForEveryConfig(T &&callable)
    {
#define REGISTER_CONFIG(name) \
        if (callable(name)) return;

        REGISTER_CONFIG(baseMacAddressOverride)
        REGISTER_CONFIG(hostname)

#ifdef CONFIG_ETH_ENABLED
        REGISTER_CONFIG(ethEnabled)
        REGISTER_CONFIG(ethUseStaticIp)
        REGISTER_CONFIG(ethStaticIp)
        REGISTER_CONFIG(ethStaticSubnet)
        REGISTER_CONFIG(ethStaticGateway)
        REGISTER_CONFIG(ethUseStaticDns)
        REGISTER_CONFIG(ethStaticDns0)
        REGISTER_CONFIG(ethStaticDns1)
        REGISTER_CONFIG(ethStaticDns2)
#endif

        REGISTER_CONFIG(wifiStaEnabled)

        for (auto &entry : wifi_configs)
        {
            REGISTER_CONFIG(entry.ssid)
            REGISTER_CONFIG(entry.key)
            REGISTER_CONFIG(entry.useStaticIp)
            REGISTER_CONFIG(entry.staticIp)
            REGISTER_CONFIG(entry.staticSubnet)
            REGISTER_CONFIG(entry.staticGateway)
            REGISTER_CONFIG(entry.useStaticDns)
            REGISTER_CONFIG(entry.staticDns0)
            REGISTER_CONFIG(entry.staticDns1)
            REGISTER_CONFIG(entry.staticDns2)
        }

        REGISTER_CONFIG(wifiStaMinRssi)

        REGISTER_CONFIG(wifiApEnabled)
        REGISTER_CONFIG(wifiApName)
        REGISTER_CONFIG(wifiApKey)
        REGISTER_CONFIG(wifiApIp)
        REGISTER_CONFIG(wifiApMask)
        REGISTER_CONFIG(wifiApChannel)
        REGISTER_CONFIG(wifiApAuthmode)

        REGISTER_CONFIG(timeServerEnabled)
        REGISTER_CONFIG(timeServer)
        REGISTER_CONFIG(timeSyncMode)
        REGISTER_CONFIG(timeSyncInterval)
        REGISTER_CONFIG(timezoneOffset)
        REGISTER_CONFIG(timeDst)

        REGISTER_CONFIG(otaUrl)

#undef REGISTER_API_VALUE
    }
};

extern ConfigManager<ConfigContainer> configs;
