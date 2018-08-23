#include <stdio.h>
#include <assert.h>

#include "Unity3D_BLENativeManager.h"
#include "Unity3D_BLENativePeripheral.h"

#define CS_CONTEXT ((void *)0xcafebabe)

const char deviceAddr[]  = "00:7E:6B:5F:95:30";
const char serviceUUID[] = "b131abdc-7195-142b-e012-0808817f198d";

NativeConnection *connection = NULL;

void scanCallback(
    void *cs_context,
    NativePeripheral *cbp,
    NativeAdvertisementData *add/*XXX*/,
    long int RSSI)
{
    char name[32];

    assert(cs_context == CS_CONTEXT);

    BLENativePeripheral *p = BLENativeCreatePeripheral(cbp);
    BLENativePeripheralGetName(p, name, sizeof(name));

    fprintf(stderr, "device: %s", name);

    if (strncmp(deviceAddr, name, sizeof(name)))
	return;

    // connection = BLENativeConnect(this, p);
}


int main(int ac, const char **av)
{
    BLENativeManager *this = BLENativeCreateManager();

    BLENativeInitialise(this, CS_CONTEXT);
    BLENativeScanStart(this, serviceUUID, scanCallback);

    for (;;) {
	BLENativeLinuxHelper(this);
    }
    /* NOTREACHED */
}
