#pragma once

#include <simpleble/export.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace simpleble {

class ServerBase; 

class SIMPLEBLE_EXPORT Server {
public:
    Server();
    ~Server();

    void start_advertising(const std::string& name, const std::string& service_uuid);
    void add_characteristic(const std::string& service_uuid, const std::string& char_uuid, bool can_read, bool can_write, bool can_notify);
    
    void set_on_ready(std::function<void()> callback);
    void set_on_read(const std::string& characteristic, std::function<std::vector<uint8_t>(const std::string&)> callback);
    void set_on_write(const std::string& characteristic, std::function<void(const std::vector<uint8_t>&, const std::string&)> callback);

    void notify(const std::string& char_uuid, const std::vector<uint8_t>& data, const std::string& target_id = "");

private:
    std::shared_ptr<ServerBase> internal_;
};

} 