# Wireshark Traffic Monitor
This repository is group of tools for pre-processing, logging and aggreagtion of traffic from CAN (and PDUs like ISO15765-2, VW TP2.0) and KLINE (ISO9141, ISO14230) into Wireshark.

## Use Case
 * **Aggregate CAN traffic** using SocketCAN link layer
 * **Aggregate ISO9141 traffic via IP RAW link layer** Simple postprocessing via LUA dissectors possible.
 * **Aggregate ISO15765 traffic via IP RAW link layer** Simple postprocessing via LUA dissectors possible.
 * **Aggregate FlexRay traffic**

## How it looks
Socket CAN read directly from remote target:  
![WS Socket CAN](/Resources/Wireshark_SocketCAN_Example.png)

Parsed KW1281 / KWP2000 (ISO14230) traffic on ME7 ECU  
![WS Socket CAN](/Resources/Wireshark_KWP2000_Example.png)

Parsed UDS traffic on MDG1 ECU  
![WS Socket CAN](/Resources/Wireshark_UDS_Example.png)

## Protocol
I can trace `CAN, DoIP and FlexRay` using proper Wireshark link layer, however **I can't** trace `VWTP20, ISO9141, KW1281 and ISO15765*` For those protocols I have created dummy IPV4 header and then put those packets at a top of it. This simplifies subsequent parsing.

**ISO15765** There is ISO15765 protocol in Wireshark, but it is mostly useless. It is not grouping multiple packets into one: i.e. you will have 20 CAN messages of conseq. frames. Dissector will just mark them, but not merge them into one line. Second big problem is inability to see errors: i.e. You have SEQ on conseq frame 1,2,4 with mising 3. That is obvious error. But not for this dissector. This dissector just don't care. Previous errors also screws up follow up frames (makring). That's the reason why I have created pre-processing on dedicated hardware.

### TCP protocol
[There is not much information in official wireshark guide on TCP sockets](https://wiki.wireshark.org/CaptureSetup/Pipes) just only this example:  
```
wireshark -k -i TCP@127.0.0.1:19000
```
Using NamedPipes code I was able to figure out what kind of bytes Wireshark expects so I can port this knowledge on Wireshark Traffic Monitor tools

TCP server is waiting on connection from Wireshark. When Wireshark opens TCP connection, server will send start packet:
```
D4-C3-B2-A1-02-00-04-00-00-00-00-00-00-00-00-00-FF-FF-00-00-65-00-00-00

Where:
    D4-C3-B2-A1 = Magic number: A1B2C3D4
    02-00 = Version Major: 2
    04-00 = Version Minor: 4
    00-00-00-00 = Thiszone: 0
    00-00-00-00 = Sigfigs: 0
    FF-FF-00-00 = Snaplen: 65535
    65-00-00-00 = Link layer ID: 101 for IP RAW packets or 227 for SocketCAN packets
```
Except Link Layer ID, everything is a constant. It is important to point out that you **CAN NOT** mix SocketCAN data and IP RAW data after you specify link layer in first packet.

### Packet header
When we are sending a packet, first we will send a packet header. Then we will send body of packet (depends on Link Layer)
```
00-00-00-00 00-00-00-00-10-00-00-00-10-00-00-00

Where:
    00-00-00-00 = Time Stamp seconds.
    00-00-00-00 = Time Stamp micro seconds
    10-00-00-00 = Size of packet saved in a file = 16 bytes of body
    10-00-00-00 = Actual size of packet = 16 bytes of body
```

### CAN Packet
If we are using Link Layer for Socket CAN, then we will be sending always 16 bytes of data (in packet header).
```
00-00-07-EF-08-00-00-00-01-02-03-04-05-06-07-08

Where:
    00-00-07-EF = CAN ID (See BE!)
    08-00-00-00 = DLC
    01-02-03-04-05-06-07-08 = CAN Data
```
### Datagram Packet
If we are using Link Layer for RAW packets, then we will send IPV4 header (20 bytes) followed by our datagram (N bytes). Then into packet header we will write 20+N bytes to be expected.
```
45-00-00-16-00-02-40-00-80-94-00-00-C0-A8-00-01-00-00-07-E0-3E-00

Where:
    45 = Version 4, 5 words (20 bytes)
    00 = Differential services.
    00-16 = Size of IP datagram + our datagram = 22 bytes
    00-02 = Identification or sequence. Increment here some number with every packet sent
    40-00 = Don't fragemnt
    80 = TTL
    94 = ISO15765 frame. I am coding protocol as 0x90 + Enum. See below
    00-00 = Header checksum
    C0-A8-00-01 = Source IP address. Not used for my purpose.
    00-00-07-E0 = Destination IP address. Used for Identification field of datagram.
    3E-00 = Data of datagram.
```
Coding of protocol is used by Wireshark to apply dissectors, like TCP, UDP, ... Using byte 0x90 means that protocol at this moment is undefined for Wireshark and I can specify my own. Current coding of protocols is as 0x90 + Protocol Constant:
```
ISO14230 = 1,
KW1281 = 2,
VWTP20 = 3,
ISO15765 = 4,
...
Debug = 0x6A,    //For Debug information (i.e. SWO output)
Warning = 0x6B,
Error = 0x6C,
```
During preprocesing of data in remote devices, we can encounter an Error or a Warning. From this reason there is a simple mechanism to show this error to user via `ip.proto == 0xFC` for an Error or `ip.proto == 0xFB` for a Warning. Preprocessing device will provide ASCII string describing the error, which will be shown in `Info` column of Wireshark with Error or Warning coloring rules.

### Example of whole communication
```
Start packet: D4-C3-B2-A1-02-00-04-00-00-00-00-00-00-00-00-00-FF-FF-00-00-E3-00-00-00

Data packet header: 00-00-00-00-00-00-00-00-10-00-00-00-10-00-00-00
Data packet: 00-00-07-EF-08-00-00-00-01-02-03-04-05-06-07-08

Data packet header: 00-00-00-00-00-00-00-00-10-00-00-00-10-00-00-00
Data packet: 00-00-07-EF-08-00-00-00-01-02-03-04-05-06-07-08
```