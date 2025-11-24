# ComnetsPBL_WSN
Instructions and codebase(s) for the Comnets3 PBL WSN project. 2025/2026

# make command line:
- `make BOARD="seeedstudio-xiao-nrf52840" RIOTBASE="$HOME/ComnetsPBL_WSN/RIOT" PORT="/dev/ttyACM0" UF2_SOFTDEV=DROP WERROR=0 flash term`
- This line should build your project and flash the binary to the board at 
- Where: 
- `RIOTBASE` should point to the RIOT directory we're using (the one in this repo)   
- `PORT` is the serial port you want to flash the firmware to
- If you add the `term` argument it will open serial comms to the board where you can run commands. If you do not have the `flash` argument and only the `term` argument, it will get you the shell without flashing the device.

# Task 1) Get familiar with this workflow
- Clone this repo: `git clone https://github.com/IsikcanYilmaz/ComnetsPBL_WSN` and go into its directory.
- Run this command to get the git submodules `git submodule update --recursive --init`. This will download the RIOT OS from a specific checkpoint that is known to work. The latest has some problems with our board.
- Go to the `gnrc_networking` directory in the root of this repo.
- Run the `make` command line noted above. Flash all of your boards.
- If you had added the `term` argument, it should take you to a shell with the device. 
- Open a shell to both of your devices. 
- Run `udp server start 1` on one device. This will be your `Receiver` node and the other device will be your `Transmitter` node.
- Run `ifconfig` on both. Here you should see the MAC and the (automatically assigned) IPv6 addresses. 
- Test for connectivity by pinging. From the shell of one of your devices do `ping <ip address>` where <ip address> is the IPv6 of your other device. The output should look like:
```
> ping fe80::aca5:e855:c919:2044
ping fe80::aca5:e855:c919:2044
12 bytes from fe80::aca5:e855:c919:2044%7: icmp_seq=0 ttl=64 rssi=-38 dBm time=5.609 ms
12 bytes from fe80::aca5:e855:c919:2044%7: icmp_seq=1 ttl=64 rssi=-38 dBm time=9.103 ms
12 bytes from fe80::aca5:e855:c919:2044%7: icmp_seq=2 ttl=64 rssi=-37 dBm time=7.502 ms

--- fe80::aca5:e855:c919:2044 PING statistics ---
3 packets transmitted, 3 packets received, 0% packet loss
round-trip min/avg/max = 5.609/7.404/9.103 ms

```
- Once you verify that the devices can talk to one another, run the following on your `Transmitter` node: `udp send <Receiver IP address> 1 aaaa`
- This will send the payload string `aaaa` to the IP address of the receiver, port number 1, where the UDP server should be listening on. Verify on the other device that our data came through. Should look like this:

Transmitter side:
```
> udp send fe80::aca5:e855:c919:2044 1 asd
Success: sent 3 byte(s) to [fe80::aca5:e855:c919:2044]:1
```
Receiver side:
```
> PKTDUMP: data received:
~~ SNIP  0 - size:   3 byte, type: NETTYPE_UNDEF (0)
00000000  61  73  64
~~ SNIP  1 - size:   8 byte, type: NETTYPE_UDP (4)
   src-port:     1  dst-port:     1
   length: 11  cksum: 0x9d32
~~ SNIP  2 - size:  40 byte, type: NETTYPE_IPV6 (2)
traffic class: 0x00 (ECN: 0x0, DSCP: 0x00)
flow label: 0x00000
length: 11  next header: 17  hop limit: 64
source address: fe80::ec0a:1a43:ebf7:2f8f
destination address: fe80::aca5:e855:c919:2044
~~ SNIP  3 - size:  24 byte, type: NETTYPE_NETIF (-1)
if_pid: 7  rssi: -40  lqi: 212
flags: 0x0
src_l2addr: EE:0A:1A:43:EB:F7:2F:8F
dst_l2addr: AE:A5:E8:55:C9:19:20:44
~~ PKT    -  4 snips, total size:  75 byte

```

# Task 2) One (or more) transmitter, one receiver
- Go into the folder `01_basic_wsn`. Build the application and flash all of your boards with it.
- This application introduces a custom shell command `wsn <sensor|root|start|stop|deinit>`. With it you can set one node as the `root` and others as the `sensor` nodes. 
- The root node will start a UDP listener on its port number 1, and let the other nodes' routing mechanisms know that it is the root node. 
- The sensor nodes will periodically send the root node a UDP packet.
- Once you flash all your boards, first, designate one as the root node and run `wsn root` in its shell. This will set this node as the `root`, start a UDP listener.
- Then, run `wsn sensor` in the shell of the other nodes which will act as your sensor nodes. This will have these nodes start periodically sending data to the `root` node. For now, let's send and receive data and learn how to parse the data.
- Study the code in `main.c`. In it you will see `WSN_NodeThread`, which is the function that our nodes will spend most of their time in. Running `wsn root` or `wsn sensor` starts a thread with this function, though it is dormant until the thread receives an Inter Process Communication message. We use these messages internally as "event triggers". This point isn't that important right now.
- Notice the function `PeriodicSensingTask()`. It gets called by the sensor nodes periodically and it is meant to do the sensing and the sending of sensor data to the root.
- Notice the function `PacketReceptionHandler()`. It gets called by the root node on every packet received. It goes through each layer of the packet (mac/ipv6/udp/payload).
- Modify `PeriodicSensingTask()` such that it sends a random uint16_t number that is between 0 and 1024.
- Modify `PacketReceptionHandler()` such that it parses this data, keeps an average of received values, and a tally of how many packets were received. Have the root node print all of these.
- This application will be the basis for the upcoming tasks.
- You are encouraged to dig into the documentation of RIOT for more info (links noted below). Please write to me if needed also.
- You are also encouraged to experiment. Go wild.
- (Note: the `reboot` command reboots the board.)

# Task 3) Sensor hardware
- GY-BMP280 Temperature/Pressure sensor datasheet: https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf
- In this task we will start using the temperature sensor module GY-BMP280. We will only do temperature readings for now. We will write a driver for this device to break out the functionality that we need.
- Wiring the sensor is easy: https://wiki.seeedstudio.com/XIAO_BLE/ you should see which pins of the board are the I2C pins; SDA is the data line, SCL is the clock line. Connect the SDA of the sensor to the Xiao's SDA and likewise the SCL pins. Connect the 3V3 and GND to the sensor's Vcc and GND.
- Go into the folder `02_sensing`. It's based on Task 2's code with 2 added files: `sensor.c` and `sensor.h`.
- This application introduces a custom shell command `sensor <id|readreg|readregs|writereg>`. With this you can interactively use the sensor module, send receive I2C data. 
- Fill in the macro definitions you see in `sensor.h`. You'll find the addresses of these registers in the manual.
- Fill in the functions `Sensor_EnableSampling`, `Sensor_LoadCalibrationData`, `Sensor_DoTemperatureReading`, `Sensor_GetChipId`. The Chip ID query will act as a test to see if I2C comms are working. 
- The reference manual should have every information you need. Dig into it.
- Once you implement the functionality and test that it's working, replace the placeholder functions from Task 01 with these.

# Experimental: 
- You will find the shell script `makescript.sh` in the gnrc_networking example directory. It's a script of mine to make the build process a bit easier but I havent tested it in this context. It should work by simply evoking it `bash makescript.sh` and if you supply a (one or more) serial port name it should flash it i.e. `bash makescript.sh /dev/ttyACM0`

# Links of interest:
- https://doc.riot-os.org/index.html 
- https://guide.riot-os.org/
- Seeedstudio-xiao-nrf52840 board page (pinout and other info here): https://wiki.seeedstudio.com/XIAO_BLE/
- The gnrc networking stack: https://doc.riot-os.org/group__net__gnrc.html
- GY-BMP280 Datasheet https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf
- I2C Basics https://www.circuitbasics.com/basics-of-the-i2c-communication-protocol/
