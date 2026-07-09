# UDPCOMM

A single-header, dependency-free C++11 template class for exchanging plain structs between processes or machines over UDP. Originally written for joystick → mobile robot teleoperation.

Define the struct you want to send and the struct you want to receive, instantiate `UDPCOMM<SendStruct, RecvStruct>`, and it handles serialization, framing, socket binding, and a background receive thread for you.

```cpp
#include "udpcomm.hpp"

struct JoyCmd   { double lx, ly, rx, ry; uint8_t buttons; };
struct RobotFb  { uint8_t state; };

// bind to the current Wi-Fi interface, listen on port 3333
UDPCOMM<JoyCmd, RobotFb> comm(NetInterface::Wifi, 3333);
// ...or bind to an explicit address:
// UDPCOMM<JoyCmd, RobotFb> comm("127.0.0.1", 3333);

// register one or more destinations
comm.add_send_address("192.168.0.18", 2222);

// send
JoyCmd cmd;
cmd.lx = 0.5;
comm.write_dataGram(cmd);

// receive (latest value, filled by the background thread)
RobotFb fb = comm.get_received_struct();
```

## Features

- **Single header, zero dependencies** — just `udpcomm.hpp`; C++11 standard library and POSIX sockets only.
- **Typed struct exchange** — send/receive types are template parameters; payloads are `memcpy`-serialized with a 2-byte head (`0xFF 0xFD`) / tail (`0xFB 0xFA`) frame so partial or foreign datagrams are rejected.
- **Automatic interface binding** — pass `NetInterface::Wifi`, `Ethernet`, `Loopback`, or `Any` and the class finds and binds the matching local address, or pass an explicit IP string.
- **Multiple destinations** — register any number of `(ip, port)` targets with `add_send_address()`; one `write_dataGram()` fans out to all of them.
- **Peer auto-registration** — anyone who sends you a datagram is added as a send target automatically, so the "server" side can reply without knowing the peer's address in advance (see `examples/receiver.cpp`).
- **Background receive thread** — datagrams are received and parsed on a dedicated thread; `get_received_struct()` returns the latest value without blocking your UI/control loop.
- **Optional nibble-split encoding** — compile with `-DUSE_BYTE_SPLIT` to split each byte into two half-bytes for links/parsers that can't handle arbitrary binary (e.g. some embedded UART-to-UDP bridges).

## Requirements

- C++11, pthreads
- Linux (POSIX sockets, `getifaddrs`, `/sys/class/net` for Wi-Fi detection)

## Using it in your project

Copy `udpcomm.hpp` into your project and link pthreads (`-pthread`). That's it.

## Examples

`examples/` contains a joystick/robot pair demonstrating bidirectional exchange over loopback:

```bash
cd examples
make
./receiver &   # robot side: replies via peer auto-registration
./sender       # joystick side: sends JoyCmd, prints RobotFb
```

## Caveats

Structs are sent as raw memory, so both ends must agree on the exact layout:

- Use trivially copyable structs (no pointers, `std::string`, virtual members, …).
- Compile both peers with the same packing/alignment; consider `#pragma pack` or fixed-width members.
- Endianness is not converted — fine between x86/ARM Linux machines in practice, but keep it in mind for exotic targets.
- One datagram per struct: keep structs under the path MTU (~1400 bytes) to avoid IP fragmentation.
- `get_received_struct()` has latest-value semantics — it is a state mirror, not a message queue.

## License

[MIT](LICENSE)
