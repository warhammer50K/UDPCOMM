#include "udp_struct.h"

QDataStream &operator<<(QDataStream &out, const STREAM &_stream)
{
    out  << _stream.lx;
    out  << _stream.ly;
    out  << _stream.rx;
    out  << _stream.ry;

    out  << _stream.x;
    out  << _stream.y;
    out  << _stream.a;
    out  << _stream.b;
    out  << _stream.l1;
    out  << _stream.l2;
    out  << _stream.r1;
    out  << _stream.r2;

    return out;
}

QDataStream &operator>>(QDataStream &in, STREAM &_stream)
{
    uint8_t dummy;

    in  >> dummy;
    in  >> dummy;

    in  >> _stream.lx;
    in  >> _stream.ly;
    in  >> _stream.rx;
    in  >> _stream.ry;

    in  >> _stream.x;
    in  >> _stream.y;
    in  >> _stream.a;
    in  >> _stream.b;
    in  >> _stream.l1;
    in  >> _stream.l2;
    in  >> _stream.r1;
    in  >> _stream.r2;

    in  >> dummy;
    in  >> dummy;

    return in;
}

