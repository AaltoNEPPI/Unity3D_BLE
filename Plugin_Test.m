#include <stdio.h>
#include <assert.h>

#ifdef __APPLE__

#import <Foundation/Foundation.h> // Needed for CBPeripheral

static void BLENativeLinuxHelper(void *this) {
    [[NSRunLoop currentRunLoop] run];
}
#endif

#undef __APPLE__
#undef __linux__
#define __TEST__

#include "Unity3D_BLENativeManager.h"
#include "Unity3D_BLENativePeripheral.h"

const char deviceAddr[]  = "00:7E:6B:5F:95:30";
const char serviceUUID[] = "b131abdc-7195-142b-e012-0808817f198d";
const char charactUUID[] = "b131bbd0-7195-142b-e012-0808817f198d";

NativeConnection *connection = NULL;

void notifyCallback(const char *uuid, const void *dp)
{
    int v = *(uint8_t *)dp;
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
