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

void Server::start_advertising(const std::string& name, const std::string& service_uuid) {
    if (internal_) internal_->start_advertising(name, service_uuid);
}

void Server::add_characteristic(const std::string& service_uuid, const std::string& char_uuid, bool can_read, bool can_write) {
    if (internal_) internal_->add_characteristic(service_uuid, char_uuid, can_read, can_write);
}

void Server::set_on_read(const std::string& char_uuid, std::function<std::vector<uint8_t>()> callback) {
    if (internal_) internal_->set_on_read(char_uuid, callback);
}

void Server::set_on_write(const std::string& char_uuid, std::function<void(const std::vector<uint8_t>&)> callback) {
    if (internal_) internal_->set_on_write(char_uuid, callback);
}