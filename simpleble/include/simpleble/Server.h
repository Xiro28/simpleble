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
    void add_characteristic(const std::string& service_uuid, const std::string& char_uuid, bool can_read, bool can_write);
    
    void set_on_read(const std::string& char_uuid, std::function<std::vector<uint8_t>()> callback);
    void set_on_write(const std::string& char_uuid, std::function<void(const std::vector<uint8_t>&)> callback);

private:
    std::shared_ptr<ServerBase> internal_;
};

} 