// Joystick side: sends JoyCmd to the receiver, prints RobotFb replies.
//   ./sender

#include "../udpcomm.hpp"
#include "messages.h"

#include <chrono>
#include <cstdio>

int main()
{
    UDPCOMM<JoyCmd, RobotFb> comm("127.0.0.1", 3333);
    if (!comm.is_bound())
    {
        return 1;
    }

    comm.add_send_address("127.0.0.1", 2222);

    for (int i = 0; i < 10; ++i)
    {
        JoyCmd cmd;
        cmd.lx = 0.1 * i;
        cmd.buttons = (uint8_t)i;
        comm.write_dataGram(cmd);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        RobotFb fb = comm.get_received_struct();
        printf("[sender] sent lx=%.1f, received state=%d battery=%.1f\n", cmd.lx, fb.state, fb.battery);
    }

    return 0;
}
