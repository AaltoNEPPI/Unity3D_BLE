#include <stdio.h>
#include <assert.h>

#ifdef __APPLE__

#import <Foundation/Foundation.h> // Needed for CBPeripheral

static void BLENativeLinuxHelper(void *this) {
    SInt32    result = CFRunLoopRunInMode(kCFRunLoopDefaultMode, 10, YES);
    switch (result) {
    case kCFRunLoopRunStopped:
    case kCFRunLoopRunFinished:
        fprintf(stderr, "Stopping run loop\n");
        exit(0);
    }
}
const char serviceUUID[] = "";
#undef __APPLE__
#undef __linux__
#else
#undef __linux__
const char serviceUUID[] = "B131ABDC-7195-142B-E012-0808817F198D";
#endif

#define __TEST__

#include "Unity3D_BLENativeManager.h"
#include "Unity3D_BLENativePeripheral.h"

const char deviceAddr[]  = "00:7E:6B:5F:95:30";
const char charactUUID[] = "B131BBD0-7195-142B-E012-0808817F198D";

NativeConnection *connection = NULL;

void notifyCallback(const char *uuid, const void *dp)
{
    int v = *(unsigned char *)dp;
    fprintf(stderr, "%s: %s, %d (%p)\n", __func__, uuid, v, dp);
}

void scanCallback(
    void *cs_context,
    NativePeripheral *cbp,
    NativeAdvertisementData *add/*XXX*/,
    long int RSSI)
{
    BLENativeManager *this = cs_context;
    char id[32] = { '\0' };

    BLENativePeripheral *p = BLENativeCreatePeripheral(cbp);
    BLENativePeripheralGetIdentifier(p, id, sizeof(id));

    fprintf(stderr, "device: %s\n", id);

    if (strncmp(deviceAddr, id, sizeof(id))) {
	BLENativePeripheralRelease(p);
	return;
    }

    BLENativePeripheralSetService(p, serviceUUID);
    BLENativePeripheralAddCharacteristic(p, charactUUID, notifyCallback);

    connection = BLENativeConnect(this, p);
}


int main(int ac, const char **av)
{
    BLENativeManager *this = BLENativeCreateManager();

    BLENativeInitialise(this, this);
    BLENativeScanStart(this, serviceUUID, scanCallback);

    for (;;) {
	BLENativeLinuxHelper(this);
    }
    /* NOTREACHED */
}
