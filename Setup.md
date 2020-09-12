# Wireshark setup
How to setup Wireshark to work with monitors.

 * Prepare and start some monitor. Either hardware or software one.
 * Start Wireshark using `wireshark -k -i TCP@127.0.0.1:19000` for Datagram tracing or `wireshark -k -i TCP@127.0.0.1:19001` for SocketCAN tracing. Obviously instead of `127.0.0.1` you will use IP address of used monitor.
 * Copy scripts into Wireshark LUA script folder `Help -> About -> Folders -> Personal Lua Plugins`
 * If you want to further develop those scripts, use something like `mklink /J "C:\Path\To\AppData\Roaming\Wireshark\plugins" "D:\Git\Monitor\Plugins"`
 * You can also load coloring rules via `View -> Coloring Rules -> Import`
