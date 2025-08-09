#include <binder/IServiceManager.h>
#include <binder/IBinder.h>
#include <binder/Parcel.h>
#include <utils/String16.h>
#include <iostream>
#include <iomanip>
#include <android/log.h>

using namespace android;

#undef LOG_TAG
#define LOG_TAG "Zygotroller"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

void hexdump(const void* data, size_t size) {
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < size; i++) {
        std::cout << std::setw(2) << std::setfill('0') << std::hex << (int)bytes[i] << " ";
        if ((i + 1) % 16 == 0) std::cout << std::endl;
    }
    if (size % 16 != 0) std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    LOGI("Attempting to remove Family Link");

    // Get the device_policy service binder
    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> dpm = sm->getService(String16("device_policy"));

    // We have to remove policies first because only the profile owner can clear them
    std::cout << "Removing policies..." << std::endl;

    // Family Link profile owner
    std::string pkg = "com.google.android.gms.supervision";
    std::string cls = "com.google.android.gms.kids.account.receiver.ProfileOwnerReceiver";

    // this is one of the only ways I could get the removal of these restrictions to work,
    // using a vector caused an unknown symbol error so we're going with this.
    auto removeRestriction = [&](const std::string& policy) {
        Parcel data2, reply2;
        data2.writeInterfaceToken(String16("android.app.admin.IDevicePolicyManager"));
        
        // writing ComponentName parcel
        data2.writeInt32(1);
        data2.writeUtf8AsUtf16(pkg);
        data2.writeUtf8AsUtf16(cls);

        // Restriction name
        data2.writeUtf8AsUtf16(policy);
        data2.writeInt32(0); // Policy disabled (set to false)

        data2.writeInt32(1); // Whatever "parent" means. (set to true)

        std::cout << "Sending Parcel to remove " << policy << " (size = " << data2.dataSize() << " bytes):" << std::endl;
        hexdump(data2.data(), data2.dataSize());

		// Transaction code 132 was setUserRestriction for my device
        dpm->transact(132, data2, &reply2);

        std::cout << "Transaction successful, (size = " << reply2.dataSize() << " bytes):" << std::endl;
        hexdump(reply2.data(), reply2.dataSize());
    };

    removeRestriction("no_factory_reset");
	removeRestriction("no_install_unknown_sources");
	removeRestriction("no_add_user");
    removeRestriction("no_config_location");
    removeRestriction("no_add_clone_profile");
    removeRestriction("no_safe_boot");
    removeRestriction("no_config_credentials");
    removeRestriction("no_config_date_time");

    // Now we're actually clearing the profile owner
    Parcel data, reply;

    data.writeInterfaceToken(String16("android.app.admin.IDevicePolicyManager"));
	
    // writing ComponentName parcel
    data.writeInt32(1);
    data.writeUtf8AsUtf16(pkg);
    data.writeUtf8AsUtf16(cls);

    std::cout << "Sending Parcel data (size = " << data.dataSize() << " bytes):" << std::endl;
    hexdump(data.data(), data.dataSize());

    // Transaction code 91 was ClearProfileOwner for my device
    dpm->transact(91, data, &reply);

    std::cout << "Transaction successful, (size = " << reply.dataSize() << " bytes):" << std::endl;
    hexdump(reply.data(), reply.dataSize());

    LOGI("I've finished execution! :D");

    return 0;
}
