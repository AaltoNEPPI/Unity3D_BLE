/*
 *  Unity3D_BLE — A Unity3D desktop plugin for BlueTooth Low Energy
 *
 *  Created by Pekka Nikander <pekka.nikander@iki.fi> on 08/08/2018.
 *  Placed in public domain.
 */

#include <assert.h>
#include <stdio.h>

#include "Unity3D_BLENativeManager.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>

typedef enum {
    INACTIVE,
    SCANNING,
} ThreadState;

struct NativeManager {
    void *cs_context;
    _Atomic ThreadState state;
};

void BLENativeInitLog(void)
{
    // Nothing to be done here under Linux?
}

BLENativeManager *BLENativeCreateManager(void)
{
    BLENativeManager *this = malloc(sizeof(BLENativeManager));
    memset(this, 0, sizeof(BLENativeManager));
    return this;
}

void BLENativeInitialise(BLENativeManager *this, void *cs_context)
{
    this->cs_context = cs_context;
}

void BLENativeDeInitialise(BLENativeManager *this)
{
    this->cs_context = NULL;
    free(this);
}

void BLENativeScanStart(
    BLENativeManager *this, char *serviceUUID,
    BLENativeScanDeviceFoundCallback *callback)
{
    /* See https://electronics.stackexchange.com/questions/
                                 82098/ble-scan-interval-and-window
     */
}

static void ScanContinue(BLENativeManager *this)
{
}

void BLENativeScanStop(BLENativeManager *this)
{
}

NativeConnection *BLENativeConnect(
    BLENativeManager *this, BLENativePeripheral *p)
{
    return (void*)0;
}

void BLENativeDisconnect(
    BLENativeManager *this, NativeConnection *c)
{
}

void BLENativeDisconnectAll(BLENativeManager *this)
{
}

/**
 * XXX
 *
 * Continuously called by a dedicated C# thread.
 */
void BLENativeLinuxHelper(BLENativeManager *this)
{
    const int retval = sd_bus_process(this->bus, XXX);
    if (retval < 0) {
	perror("Unity3D_BLENativeManager: sd_bus_process");
	return;
    }
    if (retval == 0) {
	sd_bus_wait(this->bus, XXX);
    }

    /* Return back to C#, to be called again */
}

