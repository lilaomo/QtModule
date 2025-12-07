#ifndef SUBSCRIBEOBJ_H
#define SUBSCRIBEOBJ_H

#include <QObject>
#include "MessageCenter/MessageConst.h"

struct PositionChange : public lilaomo::Message {
    int x;
    int y;
};

class SubscribeObj : public QObject
{
    Q_OBJECT
public:
    explicit SubscribeObj(QObject *parent = nullptr);

    void NormalFunc(const PositionChange& pos);
};

#endif // SUBSCRIBEOBJ_H
