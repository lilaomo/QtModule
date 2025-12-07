#include <deque>
#include <mutex>

#include "MessageCenter.h"

namespace lilaomo {

class MessageCenter::MessageCenterImpl {
public:
    struct SubscriberInfo;

    using Callback = std::function<void(const std::any&)>;
    using Subscribers = std::deque<SubscriberInfo>;
    using SubscType = std::map<std::type_index, Subscribers>;

    struct SubscriberInfo {
        void* receiver;
        uint64_t func;
        Callback cb;
    };

    void Distribute(const std::type_index& type, std::any&& msg) {
        std::unique_lock<std::mutex> ulock(mtx_);
        auto iter = subsc_.find(type);
        if (iter == subsc_.end())
            return;

        for (const auto& subs : iter->second) {
            subs.cb(msg);
        }
    }

    void Register(bool unique, const std::type_index& type, SubscriberInfo&& info) {
        std::unique_lock<std::mutex> ulock(mtx_);
        if (unique && Exist(type, info))
            return;

        subsc_[type].emplace_back(std::move(info));
    }

private:
    bool Exist(const std::type_index& type, const SubscriberInfo& info) {
        auto iter = subsc_.find(type);
        if (iter == subsc_.end())
            return false;

        for (const auto& subs : iter->second) {
            if (info.receiver == subs.receiver && info.func == subs.func)
                return true;
        }
        return false;
    }

private:
    std::mutex mtx_;
    SubscType subsc_;
};



void MessageCenter::Distribute(std::type_index type, std::any&& msg) {
    impl_->Distribute(type, std::move(msg));
}



void MessageCenter::Register(bool unique, std::type_index type, std::function<void(std::any)>&& cb, uint64_t func, void* obj) {
    MessageCenterImpl::SubscriberInfo info {
        obj,
        func,
        std::move(cb)
    };
    impl_->Register(unique, type, std::move(info));
}



MessageCenter::MessageCenter()
    : impl_(std::make_unique<MessageCenterImpl>())
{

}



MessageCenter::~MessageCenter() {

}

}
