#include <binder/IServiceManager.h>
#include <binder/IBinder.h>
#include <binder/Parcel.h>
#include <utils/String16.h>
#include <iostream>
#include <iomanip>

using namespace android;

void hexdump(const void* data, size_t size) {
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < size; i++) {
        std::cout << std::setw(2) << std::setfill('0') << std::hex << (int)bytes[i] << " ";
        if ((i + 1) % 16 == 0) std::cout << std::endl;
    }
    if (size % 16 != 0) std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    // Get the device_policy service binder
    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> dpm = sm->getService(String16("device_policy"));

    Parcel data, reply;

    data.writeInterfaceToken(String16("android.app.admin.IDevicePolicyManager"));

    // Family Link profile owner
    std::string pkg = "com.google.android.gms.supervision";
    std::string cls = "com.google.android.gms.kids.account.receiver.ProfileOwnerReceiver";
	
    // writing ComponentName parcel
    data.writeInt32(1);
    data.writeUtf8AsUtf16(pkg);
    data.writeUtf8AsUtf16(cls);

    std::cout << "Sending Parcel data (size = " << data.dataSize() << " bytes):" << std::endl;
    hexdump(data.data(), data.dataSize());

    // Transaction code 91 was ClearProfileOwner for my device
    dpm->transact(91, data, &reply);

    std::cout << "Transaction successful, reply size: " << reply.dataSize() << std::endl;
    hexdump(reply.data(), reply.dataSize());

    return 0;
}
