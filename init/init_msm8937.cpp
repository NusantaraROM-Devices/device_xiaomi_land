/*
   Copyright (c) 2016, The CyanogenMod Project
   Copyright (c) 2017, The LineageOS Project

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cstdlib>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/sysinfo.h>

#include <android-base/file.h>
#include <android-base/properties.h>
#include <android-base/strings.h>

#include "property_service.h"
#include "vendor_init.h"

#include "init_msm8937.h"

using android::base::GetProperty;
using android::init::property_set;
using android::base::ReadFileToString;
using android::base::Trim;

char const *heapstartsize;
char const *heapgrowthlimit;
char const *heapsize;
char const *heapminfree;
char const *heapmaxfree;

__attribute__ ((weak))
void init_target_properties() {}

void check_device()
{
    struct sysinfo sys;

    sysinfo(&sys);

    // set different Davlik heap properties for 2 GB
    if (sys.totalram > 2048ull * 1024 * 1024) {
        heapgrowthlimit = "192m";
        heapsize = "512m";
        // from phone-xhdpi-4096-dalvik-heap.mk
        heaptargetutilization = "0.6";
        heapminfree = "8m";
        heapmaxfree = "16m";
    } else {
        // from go_defaults_common.prop
        heapgrowthlimit = "128m";
        heapsize = "256m";
        // from phone-xhdpi-2048-dalvik-heap.mk
        heaptargetutilization = "0.75";
        heapminfree = "512k";
        heapmaxfree = "8m";
}

    // set Go tweaks for LMK for 2/3 GB
    if (sys.totalram < 3072ull * 1024 * 1024) {
        property_set("ro.lmk.critical_upgrade", "true");
        property_set("ro.lmk.upgrade_pressure", "40");
        property_set("ro.lmk.downgrade_pressure", "60");
        property_set("ro.lmk.kill_heaviest_task", "false");
    }

    // set rest of Go tweaks for 2 GB
    if (sys.totalram < 2048ull * 1024 * 1024) {
        // set lowram options and enable traced by default
        property_set("ro.config.low_ram", "true");
        property_set("persist.traced.enable", "true");
        property_set("ro.statsd.enable", "true");
        // set threshold to filter unused apps
        property_set("pm.dexopt.downgrade_after_inactive_days", "10");
        // set the compiler filter for shared apks to quicken
        property_set("pm.dexopt.shared", "quicken");
    }
}

void set_zram_size(void)
{
    FILE *f = fopen("/sys/block/zram0/disksize", "wb");
    int MB = 1024 * 1024;
    std::string zram_disksize;
    struct sysinfo si;

    // Check if zram exist
    if (f == NULL) {
        return;
    }

    // Initialize system info
    sysinfo(&si);

    // Set zram disksize (divide RAM size by 3)
    zram_disksize = std::to_string(si.totalram / MB / 3);

    // Write disksize to sysfs
    fprintf(f, "%sM", zram_disksize.c_str());

    // Close opened file
    fclose(f);
}

void vendor_load_properties()
{
    check_device();
    set_zram_size();

    property_set("dalvik.vm.heapstartsize", heapstartsize);
    property_set("dalvik.vm.heapgrowthlimit", heapgrowthlimit);
    property_set("dalvik.vm.heapsize", heapsize);
    property_set("dalvik.vm.heaptargetutilization", heaptargetutilization);
    property_set("dalvik.vm.heapminfree", heapminfree);
    property_set("dalvik.vm.heapmaxfree", heapmaxfree);

    init_target_properties();
}
