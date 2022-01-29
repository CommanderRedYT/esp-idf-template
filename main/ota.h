#pragma once

// system includes
#include <string>
#include <string_view>
#include <map>

// 3rdparty lib includes
#include <tl/expected.hpp>

// forward declares
class EspAsyncOta;

extern EspAsyncOta &otaClient;

void ota_cloud_init();
void ota_cloud_update();

tl::expected<void, std::string> otaCloudTrigger(std::string_view url);
tl::expected<void, std::string> otaCloudAbort();
