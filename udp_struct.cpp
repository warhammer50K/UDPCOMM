#include "udp_struct.h"

void stream::serialization(QDataStream &buf)
{
    buf << lx;
    buf << ly;
    buf << rx;
    buf << ry;

    buf << x;
    buf << y;
    buf << a;
    buf << b;
    buf << l1;
    buf << l2;
    buf << r1;
    buf << r2;
}
