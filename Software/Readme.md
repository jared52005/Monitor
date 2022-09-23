# Wireshark Traffic Monitor Software
Purpose of this software solution is to transfer data from USB connected CAN, FlexRay or UART devices to TCP so it is possible to log those data via Wireshark

## WTM.KLine
**Hardware:** You can use any USB to UART converter which is able to set any UART baudrate. I have found some older devices which were able to set only predefined baudrates like 9600, 19200 etc.  

I have good experience with FT232RL connected together with MC33660 or L9613

**Software** 
 * Open `WiresharkTrafficMon.sln` and compile the solution. 
 * If you are getting errors in WTM.PCAN or WTM.XL projects regarding missing binaries, unload those projects from the solution
 * Go into `Software\WTM.Kline\bin\Debug`
 * Run the monitor using `WTM.KLine.exe -c COMn -b 10400` where COMn is COM port where you have connected USB UART conveter and 10400 is baudrate used
 * Exit the application by pressing `Esc` key
 

