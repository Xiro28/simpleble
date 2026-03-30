#import "ServerMac.h"

#import <Foundation/Foundation.h>
#import <CoreBluetooth/CoreBluetooth.h>
#include <iostream>

using namespace simpleble;

@interface SimpleBLEServerDelegate : NSObject <CBPeripheralManagerDelegate>
@property (nonatomic, assign) ServerMac* cpp_server; 
@property (nonatomic, strong) CBPeripheralManager* manager; 
@property (nonatomic, strong) NSMutableDictionary<NSString*, CBCentral*>* known_centrals;
@property (nonatomic, strong) NSMutableDictionary<NSString*, CBMutableCharacteristic*>* known_characteristics;
@end

@implementation SimpleBLEServerDelegate
- (instancetype)init {
    self = [super init];
    if (self) {
        _known_centrals = [[NSMutableDictionary alloc] init];
        _known_characteristics = [[NSMutableDictionary alloc] init];
    }
    return self;
}

- (void)peripheralManagerDidUpdateState:(CBPeripheralManager *)peripheral {
    if (peripheral.state == CBManagerStatePoweredOn) {
        std::cout << "[macOS] BLE Hardware is ON and ready to host." << std::endl;
        if (self.cpp_server && self.cpp_server->on_ready_callback_) {
            self.cpp_server->on_ready_callback_();
        }
    } else {
        std::cout << "[macOS] BLE Hardware state changed: " << peripheral.state << std::endl;
    }
}

- (void)peripheralManagerDidStartAdvertising:(CBPeripheralManager *)peripheral error:(NSError *)error {
    if (error) {
        std::cerr << "[macOS] Advertising error: " << [[error localizedDescription] UTF8String] << std::endl;
    } else {
        std::cout << "[macOS] Broadcasting into the air successfully!" << std::endl;
    }
}

- (void)peripheralManager:(CBPeripheralManager *)peripheral didReceiveReadRequest:(CBATTRequest *)request {
    std::string uuid_str = [[request.characteristic.UUID UUIDString] UTF8String];
    
    std::string central_id = [[request.central.identifier UUIDString] UTF8String];
    
    if (self.cpp_server) {
        std::vector<uint8_t> cpp_data = self.cpp_server->handle_read(uuid_str, central_id);
        request.value = [NSData dataWithBytes:cpp_data.data() length:cpp_data.size()];
        [peripheral respondToRequest:request withResult:CBATTErrorSuccess];
    } else {
        [peripheral respondToRequest:request withResult:CBATTErrorUnlikelyError];
    }
}

- (void)peripheralManager:(CBPeripheralManager *)peripheral didReceiveWriteRequests:(NSArray<CBATTRequest *> *)requests {
    for (CBATTRequest *request in requests) {
        std::string uuid_str = [[request.characteristic.UUID UUIDString] UTF8String];
        
        std::string central_id = [[request.central.identifier UUIDString] UTF8String];
        
        const uint8_t* bytes = (const uint8_t*)[request.value bytes];
        size_t length = [request.value length];
        std::vector<uint8_t> cpp_data(bytes, bytes + length);
        
        if (self.cpp_server) {
            self.cpp_server->handle_write(uuid_str, cpp_data, central_id);
        }
        [peripheral respondToRequest:request withResult:CBATTErrorSuccess];
    }
}

- (void)peripheralManager:(CBPeripheralManager *)peripheral central:(CBCentral *)central didSubscribeToCharacteristic:(CBCharacteristic *)characteristic {
    NSString* central_id = [central.identifier UUIDString];
    self.known_centrals[central_id] = central; // Save the phone in memory!
    std::cout << "[macOS] Device " << [central_id UTF8String] << " subscribed!" << std::endl;
}
@end


ServerMac::ServerMac() {
    SimpleBLEServerDelegate* delegate = [[SimpleBLEServerDelegate alloc] init];
    delegate.cpp_server = this;

    dispatch_queue_t ble_queue = dispatch_queue_create("com.daas.ble", DISPATCH_QUEUE_SERIAL);
    delegate.manager = [[CBPeripheralManager alloc] initWithDelegate:delegate queue:ble_queue];
    
    opaque_internal_ = (__bridge_retained void*)delegate;
}

ServerMac::~ServerMac() {
    SimpleBLEServerDelegate* delegate = (__bridge_transfer SimpleBLEServerDelegate*)opaque_internal_;
    
    if (delegate.manager.isAdvertising) {
        [delegate.manager stopAdvertising];
    }
    delegate.manager.delegate = nil;
    delegate = nil;
}

void ServerMac::start_advertising(const std::string& name, const std::string& service_uuid) {
    SimpleBLEServerDelegate* delegate = (__bridge SimpleBLEServerDelegate*)opaque_internal_;
    
    if (delegate.manager.state != CBManagerStatePoweredOn) {
        std::cerr << "[macOS] Cannot advertise yet. Wait for Bluetooth to power on!" << std::endl;
        return;
    }
    
    NSString* apple_name = [NSString stringWithUTF8String:name.c_str()];
    CBUUID* apple_uuid = [CBUUID UUIDWithString:[NSString stringWithUTF8String:service_uuid.c_str()]];
    
    NSDictionary *adv_data = @{ 
        CBAdvertisementDataLocalNameKey: apple_name,
        CBAdvertisementDataServiceUUIDsKey: @[apple_uuid] 
    };
    
    [delegate.manager startAdvertising:adv_data];
}

void ServerMac::add_characteristic(const std::string& service_uuid, const std::string& char_uuid, bool can_read, bool can_write, bool can_notify) {
    SimpleBLEServerDelegate* delegate = (__bridge SimpleBLEServerDelegate*)opaque_internal_;
    
    CBUUID* s_uuid = [CBUUID UUIDWithString:[NSString stringWithUTF8String:service_uuid.c_str()]];
    CBUUID* c_uuid = [CBUUID UUIDWithString:[NSString stringWithUTF8String:char_uuid.c_str()]];

    CBCharacteristicProperties props = 0;
    CBAttributePermissions perms = 0;

    if (can_read) {
        props |= CBCharacteristicPropertyRead;
        perms |= CBAttributePermissionsReadable;
    }
    if (can_write) {
        props |= (CBCharacteristicPropertyWrite | CBCharacteristicPropertyWriteWithoutResponse);
        perms |= CBAttributePermissionsWriteable;
    }

    if (can_notify) {
        props |= CBCharacteristicPropertyNotify;
    }

    CBMutableCharacteristic* apple_char = [[CBMutableCharacteristic alloc] initWithType:c_uuid properties:props value:nil permissions:perms];
    NSString* ns_char_uuid = [NSString stringWithUTF8String:char_uuid.c_str()];
    delegate.known_characteristics[ns_char_uuid] = apple_char;

    CBMutableService* apple_service = [[CBMutableService alloc] initWithType:s_uuid primary:YES];
    apple_service.characteristics = @[apple_char];

    [delegate.manager addService:apple_service];
}

void ServerMac::notify(const std::string& char_uuid, const std::vector<uint8_t>& data, const std::string& target_id) {
    SimpleBLEServerDelegate* delegate = (__bridge SimpleBLEServerDelegate*)opaque_internal_;
    

    NSString* ns_char_uuid = [NSString stringWithUTF8String:char_uuid.c_str()];
    CBMutableCharacteristic* target_char = delegate.known_characteristics[ns_char_uuid];
    

    if (target_char) {
        NSData* ns_data = [NSData dataWithBytes:data.data() length:data.size()];
        NSArray<CBCentral*>* target_array = nil; 
        
        if (!target_id.empty()) {
            NSString* ns_id = [NSString stringWithUTF8String:target_id.c_str()];
            CBCentral* specific_phone = delegate.known_centrals[ns_id];
            
            if (specific_phone) {
                target_array = @[specific_phone]; 
            } else {
                std::cerr << "[macOS] Error: Tried to message an unknown device!\nBroadcast fallback" << std::endl;
            }
        }
        
        [delegate.manager updateValue:ns_data forCharacteristic:target_char onSubscribedCentrals:target_array];
    } else {
        std::cerr << "[macOS] Error: Cannot notify. Characteristic " << char_uuid << " not found!" << std::endl;
    }
}

void ServerMac::set_on_read(const std::string& char_uuid, std::function<std::vector<uint8_t>(const std::string&)> callback) {
    read_callbacks_[char_uuid] = callback;
}

void ServerMac::set_on_write(const std::string& char_uuid, std::function<void(const std::vector<uint8_t>&, const std::string&)> callback) {
    write_callbacks_[char_uuid] = callback;
}

void ServerMac::set_on_ready(std::function<void()> callback) {
    on_ready_callback_ = callback;
}

std::vector<uint8_t> ServerMac::handle_read(const std::string& char_uuid, const std::string& central_id) {
    auto it = read_callbacks_.find(char_uuid);
    if (it != read_callbacks_.end() && it->second) {
        return it->second(central_id);
    }
    return std::vector<uint8_t>();
}

void ServerMac::handle_write(const std::string& char_uuid, const std::vector<uint8_t>& data, const std::string& central_id) {
    auto it = write_callbacks_.find(char_uuid);
    if (it != write_callbacks_.end() && it->second) {
        it->second(data, central_id);
    }
}