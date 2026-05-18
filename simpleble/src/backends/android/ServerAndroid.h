#pragma once

#include "ServerBase.h"
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <jni.h>

namespace simpleble {

class ServerAndroid : public ServerBase {
public:
    ServerAndroid();
    ~ServerAndroid() override;

    void start_advertising(const std::string& name, const std::string& service_uuid) override;
    void add_characteristic(const std::string& service_uuid, const std::string& char_uuid, bool can_read, bool can_write) override;

    void set_on_read(const std::string& char_uuid, std::function<std::vector<uint8_t>()> callback) override;
    void set_on_write(const std::string& char_uuid, std::function<void(const std::vector<uint8_t>&)> callback) override;

    void set_on_disconnect(std::function<void(const std::string&)> callback) override;
    void disconnect(const std::string& char_uuid) override;

    void notify(const std::string& char_uuid, const std::vector<uint8_t>& data, const std::string& target_id = "") override;

    std::vector<uint8_t> handle_read(const std::string& char_uuid);
    void handle_write(const std::string& char_uuid, const std::vector<uint8_t>& data);

private:
    jobject java_server_; 
};

} 