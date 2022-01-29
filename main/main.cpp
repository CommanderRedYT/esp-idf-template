#include <esp_log.h>

namespace {
constexpr const char * const TAG = "MAIN";
} // namespace

extern "C" void app_main()
{
    ESP_LOGI(TAG, "hello world");
}
