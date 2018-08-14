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

static int getManagedObjectsCallback(
    sd_bus_message *reply, void *userdata, sd_bus_error *error)
{
#   define CHECK_RETURN(r) if (r < 0) { \
	fprintf(stderr, "Unity3D_BLE: %s: parse failed: %d\n", __func__, r); \
    }
    assert(reply);
    fprintf(stderr, "Unity3D_BLE: %s\n", __func__);

    const int rerr = sd_bus_message_get_errno(reply);
    if (rerr > 0) {
	fprintf(stderr, "Unity3D_BLE: %s: error=%d: %s\n",
		__func__, rerr,	sd_bus_message_get_error(reply)->name);
	return -rerr;
    }

    /* Parse objects from the message: type = a{oa{sa{sv}}} */
    int r = sd_bus_message_enter_container(reply, 'a', "{oa{sa{sv}}}");
    CHECK_RETURN(r);
    {

	while ((r = sd_bus_message_enter_container(reply, 'e', "oa{sa{sv}}")) > 0) {
	    const char *path;

	    sd_bus_message_read_basic(reply, 'o', &path);

	    fprintf(stderr, "Object %s\n", path);

	    r = sd_bus_message_enter_container(reply, 'a', "{sa{sv}}");
	    CHECK_RETURN(r);
	    {

		while ((r = sd_bus_message_enter_container(reply, 'e', "sa{sv}")) > 0) {
		    const char *interface;

		    sd_bus_message_read_basic(reply, 's', &interface);
		    fprintf(stderr, "  Interface %s\n", interface);

		    r = sd_bus_message_enter_container(reply, 'a', "{sv}");
		    CHECK_RETURN(r);
		    {

			while ((r = sd_bus_message_enter_container(reply, 'e', "sv")) > 0) {
			    const char *property;

			    sd_bus_message_read_basic(reply, 's', &property);

			    /* XXX: Handle the variant contents */
			    r = sd_bus_message_skip(reply, "v");

			    r = sd_bus_message_exit_container(reply); // 'e', "sv"
			    CHECK_RETURN(r);
			}
			CHECK_RETURN(r);

		    }
		    r = sd_bus_message_exit_container(reply); // 'a', "{sv}"
		    CHECK_RETURN(r);

		    r = sd_bus_message_exit_container(reply); // 'e', "sa{sv}"
		    CHECK_RETURN(r);
		}
		CHECK_RETURN(r);

	    }
	    r = sd_bus_message_exit_container(reply); // 'a', "{sa{sv}}"
	    CHECK_RETURN(r);

	    r = sd_bus_message_exit_container(reply); // 'e', "oa{sa{sv}}"
	    CHECK_RETURN(r);
	}
	CHECK_RETURN(r);

    }
    r = sd_bus_message_exit_container(reply); // 'a', "{oa{sa{sv}}}"
    CHECK_RETURN(r);

    return 1;
}
void BLENativeInitialise(BLENativeManager *this, void *cs_context)
{
    fprintf(stderr, "Unity3D_BLE: %s.\n", __func__);
    if (this->cs_context) {
	fprintf(stderr, "Unity3D_BLE: %s: Already initialised.", __func__);
	return;
    }
    this->cs_context = cs_context;
    assert(NULL == this->bus);
    const int retval = sd_bus_default_system(&this->bus);
    if (retval < 0) {
	fprintf(stderr, "Unity3D_BLE: %s: sd_bus_default_system: %d", __func__, retval);
	BLENativeDeInitialise(this);
	return;
    }

    // Subscribe to new proxy objects
    sd_bus_message *m = NULL;
    const int retval_msg = sd_bus_message_new_method_call(
	this->bus,
	&m,
	"org.bluez",
	"/",
	"org.freedesktop.DBus.ObjectManager",
	"GetManagedObjects");
    if (retval_msg < 0) {
	fprintf(stderr, "Unity3D_BLE: %s: sd_bus_message_new_method_call: %d", __func__, retval_msg);
	return;
    }

    sd_bus_call_async(this->bus, NULL, m, getManagedObjectsCallback, NULL, 0);
    sd_bus_message_unref(m);
}

void BLENativeDeInitialise(BLENativeManager *this)
{
    fprintf(stderr, "Unity3D_BLE: %s.\n", __func__);

    this->cs_context = NULL;
    if (this->bus) {
	sd_bus_unref(this->bus);
	this->bus = NULL;
    }
    free(this);
}

static int startDiscoveryCallback(
    sd_bus_message *reply, void *userdata, sd_bus_error *error)
{
    fprintf(stderr, "Unity3D_BLE: %s.\n", __func__);

    assert(reply);

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
	"/org/bluez/hci0",
	"org.bluez.Adapter1",
	"StartDiscovery");
    if (retval < 0) {
	perror("Unity3D_BLE: BLENativeScanStart: sd_bus_message_new_method_call");
	return;
    }

    sd_bus_call_async(this->bus, NULL, m, startDiscoveryCallback, NULL, 0);
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

