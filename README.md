# Reliable UDP

Implementing reliable protocol of the udp to transfer a video file.

## Description

The protocol is made reliable using the following techniques:
* Sequence numbers
* Retransmission (selective repeat)
* Window size of 5-10 UDP segments (stop n wait)
* Re ordering on receiver side

## Getting Started

### Dependencies

* C++
* Linux

### Installing

* Clone the repo
```
git clone https://github.com/imranzaheer612/reliableUDP.git
```
* change dir to reliableUDP then specify the port number and the video file you want to use in **client.c and server.c** 
```
#define VIDEO_FILE "testFiles/outUDP.mp4"
#define PORT 8080
```  

### Executing program

* Start the server first
```
gcc server.c -o server -lpthread
./server
```
* Then start the client
```
gcc client.c -o client -lpthread
./client
```
## Help

If you run the code directly via IDE(vscode) there could be chance the code is not compiled keeping in view the pthread library. You can compile it manually as the above given command by adding **-lpthread** in the commands.


## License

This project is licensed under the [MIT] License - see the LICENSE.md file for details

## Acknowledgments

This repo helped alot.
* [implementing reliable udp](https://github.com/EeshaArif/Reliable-File-Transfer-UDP-Without-Timeout.git)
