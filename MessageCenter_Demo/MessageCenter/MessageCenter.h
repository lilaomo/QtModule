#ifndef MESSAGECENTER_H
#define MESSAGECENTER_H

#define LLMMSG lilaomo::MessageCenter::Instance()

#include <typeindex>
#include <any>

#include "MessageConst.h"

#define BASE_OF(Msg) std::enable_if_t<std::is_base_of<Message, Msg>::value>

namespace lilaomo {

class MessageCenter : public QObject
{
    Q_OBJECT
    class MessageCenterImpl;
    using ImplType = std::unique_ptr<MessageCenterImpl>;

public:
    static MessageCenter* Instance() {
        static MessageCenter _this;
        return &_this;
    }

    template<typename MsgType>
    BASE_OF(MsgType) Publish(MsgType&& msg) {
        std::any _msg = std::forward<MsgType>(msg);
        Distribute(typeid(MsgType), std::move(_msg));
    }

    template<typename MsgType, typename RetType, typename ClassType>
    BASE_OF(MsgType) Subscribe(RetType (ClassType::* func)(MsgType), ClassType* obj) {
        Register(false, typeid(MsgType), [func, obj](const std::any& msg) {
            const MsgType& _msg = std::any_cast<const MsgType&>(msg);
            (obj->*func)(_msg);
        }, reinterpret_cast<uint64_t>(func), static_cast<void*>(obj));
    }

    template<typename MsgType, typename Function>
    BASE_OF(MsgType) Subscribe(Function&& func) {
        Register(false, typeid(MsgType), [func](const std::any& msg) {
            const MsgType& _msg = std::any_cast<const MsgType&>(msg);
            func(_msg);
        }, reinterpret_cast<uint64_t>(func), nullptr);
    }

    template<typename MsgType, typename RetType, typename ClassType>
    BASE_OF(MsgType) SubscribeUnique(RetType (ClassType::* func)(MsgType), ClassType* obj) {
        Register(true, typeid(MsgType), [func, obj](const std::any& msg) {
            MsgType _msg = std::any_cast<MsgType>(msg);
            (obj->*func)(_msg);
        }, reinterpret_cast<uint64_t>(func), static_cast<void*>(obj));
    }

private:
    void Distribute(std::type_index type, std::any&& msg);
    void Register(bool unique, std::type_index type, std::function<void(std::any)>&& cb, uint64_t func, void* obj);

    explicit MessageCenter();
    ~MessageCenter();

private:
    ImplType impl_;
};

}

#endif // MESSAGECENTER_H
