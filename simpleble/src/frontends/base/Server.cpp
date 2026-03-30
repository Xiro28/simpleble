#include <simpleble/Server.h>

#include "ServerBase.h"


#if SIMPLEBLE_BACKEND_MACOS
#include "backends/macos/ServerMac.h"
#endif

#if SIMPLEBLE_BACKEND_ANDROID
#include "backends/macos/ServerAndroid.h"
#endif

using namespace simpleble;

Server::Server() {
#if SIMPLEBLE_BACKEND_MACOS
    internal_ = std::make_shared<ServerMac>();
#elif SIMPLEBLE_BACKEND_ANDROID
    internal_ = std::make_shared<ServerAndroid>();
#else
    internal_ = nullptr; 
#endif
}

Server::~Server() = default;

void Server::set_on_ready(std::function<void()> callback) {
    if (internal_) internal_->set_on_ready(callback);
}

void Server::start_advertising(const std::string& name, const std::string& service_uuid) {
    if (internal_) internal_->start_advertising(name, service_uuid);
}

void Server::add_characteristic(const std::string& service_uuid, const std::string& char_uuid, bool can_read, bool can_write, bool can_notify) {
    if (internal_) internal_->add_characteristic(service_uuid, char_uuid, can_read, can_write, can_notify);
}

void Server::set_on_read(const std::string& characteristic, std::function<std::vector<uint8_t>(const std::string&)> callback) {
    internal_->set_on_read(characteristic, callback);
}
void Server::set_on_write(const std::string& characteristic, std::function<void(const std::vector<uint8_t>&, const std::string&)> callback) {
    internal_->set_on_write(characteristic, callback);
}

void Server::notify(const std::string& char_uuid, const std::vector<uint8_t>& data, const std::string& target_id) {
    if (internal_) internal_->notify(char_uuid, data, target_id);
}