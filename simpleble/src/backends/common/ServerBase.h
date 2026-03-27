#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>

namespace simpleble {

class ServerBase {
public:
    virtual ~ServerBase() = default;

    virtual void start_advertising(const std::string& name, const std::string& service_uuid) = 0;
    virtual void add_characteristic(const std::string& service_uuid, const std::string& char_uuid, bool can_read, bool can_write) = 0;
    
    virtual void set_on_read(const std::string& char_uuid, std::function<std::vector<uint8_t>()> callback) = 0;
    virtual void set_on_write(const std::string& char_uuid, std::function<void(const std::vector<uint8_t>&)> callback) = 0;
};

} 