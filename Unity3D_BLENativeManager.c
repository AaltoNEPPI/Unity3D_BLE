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

#include <systemd/sd-bus.h>

typedef enum {
    INACTIVE,
    SCANNING,
} ThreadState;

struct NativeManager {
    void *cs_context;
    sd_bus *bus;
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
    fprintf(stderr, "Unity3D_BLE: BLENativeInitialise.\n");
    if (this->cs_context) {
	fprintf(stderr, "Unity3D_BLE: BLENativeInitialise: Already initialised.");
	return;
    }
    this->cs_context = cs_context;
    assert(NULL == this->bus);
    const int retval = sd_bus_default_system(&this->bus);
    if (retval < 0) {
	perror("Unity3D_BLE: BLENativeInitialise: sd_bus_default_system");
	BLENativeDeInitialise(this);
	return;
    }
}

void BLENativeDeInitialise(BLENativeManager *this)
{
    fprintf(stderr, "Unity3D_BLE: BLENativeDeInitialise.\n");
    this->cs_context = NULL;
    if (this->bus) {
	sd_bus_unref(this->bus);
	this->bus = NULL;
    }
    free(this);
}

static int scanStartCallback(
    sd_bus_message *reply, void *userdata, sd_bus_error *error)
{
    assert(reply);

    fprintf(stderr, "Unity3D_BLE: scanStartCallback.\n");

    return 1;
}

void BLENativeScanStart(
    BLENativeManager *this, char *serviceUUID,
    BLENativeScanDeviceFoundCallback *callback)
{
    /* See https://electronics.stackexchange.com/questions/
                                 82098/ble-scan-interval-and-window
     */
    sd_bus_message *m = NULL;

    const int retval = sd_bus_message_new_method_call(
	this->bus,
	&m,
	"org.bluez",
	"/org/bluez/hc0",
	"org.bluez.Adapter1",
	"StartDiscovery");
    if (retval < 0) {
	perror("Unity3D_BLE: BLENativeScanStart: sd_bus_message_new_method_call");
	return;
    }

    sd_bus_call_async(this->bus, NULL, m, scanStartCallback, NULL, 0);
    sd_bus_message_unref(m);
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
    const int retval = sd_bus_process(this->bus, NULL/*XXX*/);
    if (retval < 0) {
	perror("Unity3D_BLENativeManager: sd_bus_process");
	return;
    }
    if (retval == 0) {
	fprintf(stderr, "Unity3D_BLE: BLENativeLinuxHelper: entering sd_bus_wait\n");
	sd_bus_wait(this->bus, -1);
    }

    /* Return back to C#, to be called again */
}

