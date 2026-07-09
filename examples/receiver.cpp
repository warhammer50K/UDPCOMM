// Robot side: receives JoyCmd and replies with RobotFb.
// No add_send_address() needed — the sender is auto-registered
// from the first datagram it sends us.
//   ./receiver

#include "../udpcomm.hpp"
#include "messages.h"

#include <chrono>
#include <cstdio>

int main()
{
    UDPCOMM<RobotFb, JoyCmd> comm("127.0.0.1", 2222);
    if (!comm.is_bound())
    {
        return 1;
    }

    for (int i = 0; i < 20; ++i)
    {
        JoyCmd cmd = comm.get_received_struct();
        printf("[receiver] lx=%.1f buttons=%d\n", cmd.lx, cmd.buttons);

        RobotFb fb;
        fb.state = 1;
        fb.battery = 87.5;
        comm.write_dataGram(fb);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return 0;
}
