# UDPCOMM

A small header-only C++ template class for exchanging plain structs between processes or machines over UDP. Built on Qt, originally written for joystick → mobile robot teleoperation.

Define the struct you want to send and the struct you want to receive, instantiate `UDPCOMM<SendStruct, RecvStruct>`, and it handles serialization, framing, socket binding, and a background receive thread for you.

```cpp
#include "udpcomm.hpp"

struct JoyCmd   { double lx, ly, rx, ry; uint8_t buttons; };
struct RobotFb  { uint8_t state; };

// bind to the current Wi-Fi interface, listen on port 3333
UDPCOMM<JoyCmd, RobotFb> comm(QNetworkInterface::Wifi, 3333);

// register one or more destinations
comm.add_send_address(QHostAddress("192.168.0.18"), 2222);

// send
JoyCmd cmd;
cmd.lx = 0.5;
comm.write_dataGram(cmd);

// receive (filled by the background thread)
RobotFb fb = comm.get_received_struct();
```

## Features

- **Typed struct exchange** — send/receive types are template parameters; payloads are `memcpy`-serialized with a 2-byte head (`0xFF 0xFD`) / tail (`0xFB 0xFA`) frame so partial or foreign datagrams are rejected.
- **Automatic interface binding** — pass `QNetworkInterface::Wifi`, `Ethernet`, etc. and the class finds and binds the matching local address; no hardcoded IPs on the receiving side.
- **Multiple destinations** — register any number of `(ip, port)` targets with `add_send_address()`; one `write_dataGram()` fans out to all of them.
- **Background receive thread** — datagrams are drained on a dedicated thread into a TBB concurrent queue; `get_received_struct()` never blocks your UI/control loop.
- **Two socket backends** — Linux native sockets (default, `USE_LINUX_NATIVE_SOCKET`) or `QUdpSocket`, switchable with one `#define` in `udpcomm.hpp`.
- **Optional nibble-split encoding** — `USE_BYTE_SPLIT` splits each byte into two half-bytes for links/parsers that can't handle arbitrary binary (e.g. some embedded UART-to-UDP bridges).

## Requirements

- Qt 5 (`core`, `network`; the demo app additionally uses `widgets` and `gamepad`)
- Intel TBB (`-ltbb`)
- C++11, Linux (the default backend uses POSIX sockets)

## Using it in your project

The library itself is just the headers — copy these into your project:

```
udpcomm.hpp        # the UDPCOMM template class
udpsocket.h/.cpp   # Linux native socket backend (default)
myudpsocket.h/.cpp # QUdpSocket backend (only if you switch backends)
```

Then link TBB and Qt Network. `main.cpp` / `mainwindow.cpp` and the `.pro` file are a Qt Widgets demo that sends gamepad state (`STREAM` in `global_defines.h`) to a mobile robot and reads back its status (`MOBILE`).

## Caveats

Structs are sent as raw memory, so both ends must agree on the exact layout:

- Use trivially copyable structs (no pointers, `std::string`, virtual members, …).
- Compile both peers with the same packing/alignment; consider `#pragma pack` or fixed-width members.
- Endianness is not converted — fine between x86/ARM Linux machines in practice, but keep it in mind for exotic targets.
- One datagram per struct: keep structs under the path MTU (~1400 bytes) to avoid IP fragmentation.
