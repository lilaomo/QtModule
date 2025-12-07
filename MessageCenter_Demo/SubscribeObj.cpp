#include "SubscribeObj.h"
#include <QDebug>

SubscribeObj::SubscribeObj(QObject *parent)
    : QObject{parent}
{}

void SubscribeObj::NormalFunc(const PositionChange& pos)
{
    qDebug() << "normal: " << pos.x << pos.y;
}
