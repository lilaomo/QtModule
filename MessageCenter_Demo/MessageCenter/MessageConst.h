#ifndef MESSAGECONST_H
#define MESSAGECONST_H

#include <QObject>

namespace lilaomo {

struct Message {
    Message();
    Message(const Message& other);
    Message(Message&& other);
    virtual ~Message();

    void operator=(const Message& other);
    void operator=(Message&& other);

    QObject* Sender() const;

private:
    class MessageImpl;
    using ImplType = std::unique_ptr<MessageImpl>;

    ImplType impl_;
};

}

#endif // MESSAGECONST_H
