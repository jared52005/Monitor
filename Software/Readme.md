# Wireshark Traffic Monitor Software
Purpose of this software solution is to transfer data from USB connected CAN, FlexRay or UART devices to TCP so it is possible to log those data via Wireshark

## WTM.KLine
**Hardware:** You can use any USB to UART converter which is able to set any UART baudrate. I have found some older devices which were able to set only predefined baudrates like 9600, 19200 etc.  

I have good experience with FT232RL connected together with MC33660 or L9613

This software can also follow changes in baudrate from application layer (i.e. 10400 -> 52800) and rebaud the interface accordingly.

**Software** 
 * Open `WiresharkTrafficMon.sln` and compile the solution. 
 * Go into `Software\WTM.Kline\bin\Debug`
 * Run the monitor using `WTM.KLine.exe -c COMn -b 10400` where COMn is COM port where you have connected USB UART conveter and 10400 is baudrate used
 * Exit the application by pressing `Esc` key
 
## WTM.XL
**Hardware:** VN7610, VN7640 or similar device with support of FlexRay (optional, if you want to log only CAN bus). Does not need to have a License for Vector products installed in the device.  

XL API is described [here](https://cdn.vector.com/cms/content/products/XL_Driver_Library/Docs/XL_Driver_Library_Manual_EN.pdf)

## WTM.PCan
**Hardware:** PeakCAN USB or compatible.

**Software** 
 * Open `WiresharkTrafficMon.sln` and compile the solution. 
 * Go into `Software\WTM.Pcan\bin\Debug`
 * Run the monitor using `WTM.Pcan.exe -b 500000 -f "D:\Path\To\CanIds_Example.xml"` where 500000 is baudrate and `CanIds_Example` is an optional file describing how CAN IDs should be processed
 * Exit the application by pressing `Esc` key
