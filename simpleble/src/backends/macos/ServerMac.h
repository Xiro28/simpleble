#pragma once

#include "ServerBase.h"

namespace simpleble {

class ServerMac : public ServerBase {
public:
    ServerMac();
    ~ServerMac() override;

    void start_advertising(const std::string& name, const std::string& service_uuid) override;
    void add_characteristic(const std::string& service_uuid, const std::string& char_uuid, bool can_read, bool can_write, bool can_notify) override;
    
    void set_on_ready(std::function<void()> callback) override;
    void set_on_read(const std::string& characteristic, std::function<std::vector<uint8_t>(const std::string&)> callback) override;
    void set_on_write(const std::string& characteristic, std::function<void(const std::vector<uint8_t>&, const std::string&)> callback) override;

    void notify(const std::string& char_uuid, const std::vector<uint8_t>& data, const std::string& target_id = "")  override;

    std::vector<uint8_t> handle_read(const std::string& char_uuid, const std::string& central_id);
    void handle_write(const std::string& char_uuid, const std::vector<uint8_t>& data, const std::string& central_id);

    std::function<void()> on_ready_callback_;

private:

    void* opaque_internal_; 

    std::map<std::string, std::function<std::vector<uint8_t>(const std::string&)>> read_callbacks_;
    std::map<std::string, std::function<void(const std::vector<uint8_t>&, const std::string&)>> write_callbacks_;
};

} 