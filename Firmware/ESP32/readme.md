# Wireshark and ESP32 setup
How to setup Wireshark to work with any ESP32

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
 * Open esp-idf (Power Shell or CMD) go to Firmware/ESP32
 * Setup device using `idf.py set-target esp32` (or `idf.py set-target esp32s3` etc.)
 * Compile firmware using `idf.py build` 
 * Upload firmware using `idf.py -p COMn flash` where COMn is debug UART of your ESP32 device
 * Start Wireshark using `wireshark -k -i TCP@127.0.0.1:19000` for Datagram (PDU) tracing or `wireshark -k -i TCP@127.0.0.1:19001` for SocketCAN tracing. Replace `127.0.0.1` with IP address of ESP32 device.
 * Copy scripts into Wireshark LUA script folder `Help -> About -> Folders -> Personal Lua Plugins`
 * If you want to further develop those scripts, use something like `mklink /J "C:\Path\To\AppData\Roaming\Wireshark\plugins" "D:\Git\Monitor\Plugins"`
 * You can also load coloring rules via `View -> Coloring Rules -> Import`
