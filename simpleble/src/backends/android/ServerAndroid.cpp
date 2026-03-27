#include <jni.h>
#include "ServerAndroid.h"
#include <android/log.h>

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "SimpleBLE_Server", __VA_ARGS__)

extern JNIEnv* get_env();

jobject get_android_context() {
    JNIEnv* env = get_env();
    
    jclass activity_thread_class = env->FindClass("android/app/ActivityThread");
    if (activity_thread_class == nullptr) return nullptr;
    
    jmethodID current_activity_thread_method = env->GetStaticMethodID(
        activity_thread_class, 
        "currentActivityThread", 
        "()Landroid/app/ActivityThread;"
    );
    
    jobject current_activity_thread = env->CallStaticObjectMethod(
        activity_thread_class, 
        current_activity_thread_method
    );
    
    jmethodID get_application_method = env->GetMethodID(
        activity_thread_class, 
        "getApplication", 
        "()Landroid/app/Application;"
    );
    
    jobject context = env->CallObjectMethod(
        current_activity_thread, 
        get_application_method
    );
    
    env->DeleteLocalRef(activity_thread_class);
    env->DeleteLocalRef(current_activity_thread);
    
    return context;
}

namespace simpleble {

ServerBase::ServerBase() {
    JNIEnv* env = get_env();
    
    jclass server_class = env->FindClass("org/simpleble/android/SimpleBleServer");
    if (server_class == nullptr) {
        LOGE("Could not find org.simpleble.android.SimpleBleServer!");
        return;
    }

    // 2. Find the constructor: (Context, long)
    jmethodID constructor = env->GetMethodID(server_class, "<init>", "(Landroid/content/Context;J)V");
    
    // 3. Instantiate the Kotlin object, passing 'this' as the native handle
    jobject local_java_server = env->NewObject(server_class, constructor, get_android_context(), reinterpret_cast<jlong>(this));
    
    // 4. Save a global reference so it survives
    java_server_ = env->NewGlobalRef(local_java_server);
    
    env->DeleteLocalRef(server_class);
    env->DeleteLocalRef(local_java_server);
}

ServerBase::~ServerBase() {
    if (java_server_) {
        JNIEnv* env = get_env();
        env->DeleteGlobalRef(java_server_);
    }
}

void ServerBase::start_advertising(const std::string& name, const std::string& service_uuid) {
    if (!java_server_) return;
    JNIEnv* env = get_env();

    jclass server_class = env->GetObjectClass(java_server_);
    jmethodID start_adv_id = env->GetMethodID(server_class, "startAdvertising", "(Ljava/lang/String;Ljava/lang/String;)V");

    jstring jname = env->NewStringUTF(name.c_str());
    jstring juuid = env->NewStringUTF(service_uuid.c_str());

    env->CallVoidMethod(java_server_, start_adv_id, jname, juuid);

    env->DeleteLocalRef(jname);
    env->DeleteLocalRef(juuid);
    env->DeleteLocalRef(server_class);
}

void ServerBase::add_characteristic(const std::string& service_uuid, const std::string& char_uuid, bool can_read, bool can_write) {
    if (!java_server_) return;
    JNIEnv* env = get_env();

    jclass server_class = env->GetObjectClass(java_server_);
    jmethodID add_char_id = env->GetMethodID(server_class, "addCharacteristic", "(Ljava/lang/String;Ljava/lang/String;ZZ)V");

    jstring j_service_uuid = env->NewStringUTF(service_uuid.c_str());
    jstring j_char_uuid = env->NewStringUTF(char_uuid.c_str());

    env->CallVoidMethod(java_server_, add_char_id, j_service_uuid, j_char_uuid, can_read, can_write);

    env->DeleteLocalRef(j_service_uuid);
    env->DeleteLocalRef(j_char_uuid);
    env->DeleteLocalRef(server_class);
}

void ServerBase::set_on_read(const std::string& char_uuid, std::function<std::vector<uint8_t>()> callback) {
    read_callbacks_[char_uuid] = callback;
}

void ServerBase::set_on_write(const std::string& char_uuid, std::function<void(const std::vector<uint8_t>&)> callback) {
    write_callbacks_[char_uuid] = callback;
}

std::vector<uint8_t> ServerBase::handle_read(const std::string& char_uuid) {
    auto it = read_callbacks_.find(char_uuid);
    if (it != read_callbacks_.end() && it->second) {
        return it->second();
    }
    LOGE("Unhandled read request for UUID: %s", char_uuid.c_str());
    return std::vector<uint8_t>();
}

void ServerBase::handle_write(const std::string& char_uuid, const std::vector<uint8_t>& data) {
    auto it = write_callbacks_.find(char_uuid);
    if (it != write_callbacks_.end() && it->second) {
        it->second(data);
    } else {
        LOGE("Unhandled write request for UUID: %s", char_uuid.c_str());
    }
}

} 