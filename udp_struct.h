#ifndef UDP_STRUCT_H
#define UDP_STRUCT_H

#include <QDataStream>

struct stream
{
    double lx = 0.0;
    double ly = 0.0;
    double rx = 0.0;
    double ry = 0.0;

    uint8_t x = 0;
    uint8_t y = 0;
    uint8_t a = 0;
    uint8_t b = 0;
    uint8_t l1 = 0;
    uint8_t l2 = 0;
    uint8_t r1 = 0;
    uint8_t r2 = 0;
    stream() {}

    void serialization(QDataStream &buf);
};

#endif // UDP_STRUCT_H
