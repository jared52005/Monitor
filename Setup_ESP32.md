# Wireshark and ESP32 setup
How to setup Wireshark to work with monitors.

 * Go into Firmware/ESP32
 * Create `wifi_creds.h` and add there your credentials
```
#define EXAMPLE_ESP_WIFI_SSID      "your-ssid"
#define EXAMPLE_ESP_WIFI_PASS      "your-password"
```
 * Go into `CanIf.c` and select GPIO where you have connected CAN transciever
```
#define TX_GPIO_NUM                     (GPIO_NUM_5)
#define RX_GPIO_NUM                     (GPIO_NUM_4)
```
 * Compile and upload firmware in Firmware/ESP32 using `esp-idf`. Any ESP32 device should be working
 * Start Wireshark using `wireshark -k -i TCP@127.0.0.1:19000` for Datagram tracing or `wireshark -k -i TCP@127.0.0.1:19001` for SocketCAN tracing. Obviously instead of `127.0.0.1` you will use IP address of used monitor.
 * Copy scripts into Wireshark LUA script folder `Help -> About -> Folders -> Personal Lua Plugins`
 * If you want to further develop those scripts, use something like `mklink /J "C:\Path\To\AppData\Roaming\Wireshark\plugins" "D:\Git\Monitor\Plugins"`
 * You can also load coloring rules via `View -> Coloring Rules -> Import`
