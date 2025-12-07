#include <QVariant>
#include "MessageConst.h"

namespace lilaomo {

class Message::MessageImpl {
public:
    QObject* Sender() const {
        return sender_;
    }

private:
    QObject* sender_;
};



Message::Message()
    : impl_(std::make_unique<MessageImpl>())
{

}



Message::Message(const Message &other)
    : impl_(std::make_unique<MessageImpl>(*other.impl_.get()))
{

}



Message::Message(Message &&other)
    : impl_(std::move(other.impl_))
{

}



Message::~Message()
{

}



void Message::operator=(const Message &other) {
    impl_.reset();
    impl_ = std::make_unique<MessageImpl>(*other.impl_.get());
}



void Message::operator=(Message&& other) {
    impl_.reset();
    impl_ = std::move(other.impl_);
}



QObject* Message::Sender() const {
    return impl_->Sender();
}

}
