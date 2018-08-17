/*
 *  Unity3D_BLE — A Unity3D desktop plugin for BlueTooth Low Energy
 *
 *  Created by Pekka Nikander <pekka.nikander@iki.fi> on 08/08/2018.
 *  Placed in public domain.
 */

#include <assert.h>
#include <stdio.h>
#include <errno.h>

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
    /*_Atomic XXX*/ ThreadState state;
    void (*deviceFoundCallback)(
        void *, NativePeripheral *, NativeAdvertisementData */*XXX*/, long int);
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

static void addPeripheral(
    BLENativeManager *this, const char *path, const char *address, int rssi)
{
    if (this->deviceFoundCallback) {
        BLENativePeripheral *peri =
            BLENativeCreatePeripheralInternal(this, path, address, rssi);

	fprintf(stderr, "Calling deviceFoundCallback for %s\n", address);

        this->deviceFoundCallback(
            this->cs_context,
            peri,
            NULL,
            rssi);
    }
}

# define CHECK_RETURN(r) if ((r) < 0) { \
        fprintf(stderr, "Unity3D_BLE: %s: parse failed: %d (L%d)\n", __func__, (r), __LINE__); \
        return r; \
    }

/**
 * XXX
 *
 * @param m A message pointing to a dictionary of methods and properties
 */
static int addDevice(BLENativeManager *this, const char *path, sd_bus_message *m)
{
    int r;
    const char *address;
    int16_t rssi;
    const char **UUIDs;
    size_t size;

    r = sd_bus_message_enter_container(m, 'a', "{sv}");
    CHECK_RETURN(r);
    {

        while ((r = sd_bus_message_enter_container(m, 'e', "sv")) > 0) {
            const char *property;

            sd_bus_message_read_basic(m, 's', &property);

            char type;
            const char *contents;
            r = sd_bus_message_peek_type(m, &type, &contents);
            CHECK_RETURN(r);

            if ('v' != type) {
                fprintf(stderr, "Unity3D_BLE: %s: parse failed: %d (L%d)\n", __func__, (r), __LINE__); \
                return -EBADMSG;
            }

            r = sd_bus_message_enter_container(m, 'v', contents);
            CHECK_RETURN(r);
            {
                if (contents[0] == 's' && !strncmp(BLUEZ_DEVICE_ADDRESS, property, sizeof(BLUEZ_DEVICE_ADDRESS))) {
                    r = sd_bus_message_read_basic(m, contents[0], &address);
                    CHECK_RETURN(r);
                    fprintf(stderr, "      Address=%s\n", address);
                } else
                if (contents[0] == 'n' && !strncmp(BLUEZ_DEVICE_RSSI, property, sizeof(BLUEZ_DEVICE_RSSI))) {
		    r = sd_bus_message_read_basic(m, contents[0], &rssi);
		    CHECK_RETURN(r);
		    fprintf(stderr, "      RSSI=%d\n", rssi);
#if 0
		} else
 	        // Something below is not correct, causes a core dump
                if (contents[0] == 'a' && !strncmp(BLUEZ_DEVICE_SERVICES, property, sizeof(BLUEZ_DEVICE_SERVICES))) {
		    if ('s' != contents[1]) {
			fprintf(stderr, "Unity3D_BLE: %s: parse failed: non-string UUID\n", __func__);
			return r;
		    }
		    // XXX: Is the following correct?
		    r = sd_bus_message_read_array(m, 's', (const void **)UUIDs, &size);
		    for (size_t i = 0; i < size; i++) {
			fprintf(stderr, "      UUID[%ld]=%s\n", i, UUIDs[i]);
		    }
#endif
		} else {
		    r = sd_bus_message_skip(m, contents);
		}
            }
            r = sd_bus_message_exit_container(m); // 'v', contents
            CHECK_RETURN(r);

            r = sd_bus_message_exit_container(m); // 'e', "sv"
            CHECK_RETURN(r);
        }
        CHECK_RETURN(r);
    }
    r = sd_bus_message_exit_container(m); // 'a', "{sv}"
    CHECK_RETURN(r);

    addPeripheral(this, path, address, rssi);
    return 1;
}

/**
 * XXX
 *
 * @param m A message pointing to a dictionary of methods and properties
 */
static int addService(BLENativeManager *this, const char *path, sd_bus_message *m)
{
    int r;
    const char *device;
    const char *uuid;

    r = sd_bus_message_enter_container(m, 'a', "{sv}");
    CHECK_RETURN(r);
    {
        while ((r = sd_bus_message_enter_container(m, 'e', "sv")) > 0) {
            const char *property;

            sd_bus_message_read_basic(m, 's', &property);
            char type;
            const char *contents;
            r = sd_bus_message_peek_type(m, &type, &contents);
            CHECK_RETURN(r);

            if ('v' != type) {
                fprintf(stderr, "Unity3D_BLE: %s: parse failed: %d (L%d)\n", __func__, (r), __LINE__); \
                return -EBADMSG;
            }

            r = sd_bus_message_enter_container(m, 'v', contents);
            CHECK_RETURN(r);
            {
		if (contents[0] == 's'
		    && !strncmp(BLUEZ_SERVICE_UUID, property, sizeof(BLUEZ_SERVICE_UUID))) {

		    r = sd_bus_message_read_basic(m, contents[0], &uuid);
		    CHECK_RETURN(r);
		    fprintf(stderr, "      UUID=%s\n", uuid);

		} else
                if (contents[0] == 's'
		    && !strncmp(BLUEZ_SERVICE_DEVICE, property, sizeof(BLUEZ_SERVICE_DEVICE))) {

		    r = sd_bus_message_read_basic(m, contents[0], &device);
		    CHECK_RETURN(r);
		    fprintf(stderr, "      Device=%s\n", device);
		} else {
		    r = sd_bus_message_skip(m, contents);
		}
            }
            r = sd_bus_message_exit_container(m); // 'v', contents
            CHECK_RETURN(r);

            r = sd_bus_message_exit_container(m); // 'e', "sv"
            CHECK_RETURN(r);
        }
        CHECK_RETURN(r);

    }
    r = sd_bus_message_exit_container(m); // 'a', "{sv}"
    CHECK_RETURN(r);

    BLENativePeripheralAddServicePath(device, uuid, path);
    return 1;
}

/**
 * XXX
 *
 * @param m A message pointing to a dictionary of methods and properties
 */
static int addCharacteristic(BLENativeManager *this, const char *path, sd_bus_message *m)
{
    int r;
    const char *service; /* DBUS service path */
    const char *uuid;

    r = sd_bus_message_enter_container(m, 'a', "{sv}");
    CHECK_RETURN(r);
    {
        while ((r = sd_bus_message_enter_container(m, 'e', "sv")) > 0) {
            const char *property;

            sd_bus_message_read_basic(m, 's', &property);
            char type;
            const char *contents;
            r = sd_bus_message_peek_type(m, &type, &contents);
            CHECK_RETURN(r);

            if ('v' != type) {
                fprintf(stderr, "Unity3D_BLE: %s: parse failed: %d (L%d)\n", __func__, (r), __LINE__); \
                return -EBADMSG;
            }

            r = sd_bus_message_enter_container(m, 'v', contents);
            CHECK_RETURN(r);
            {
		if (contents[0] == 's'
		    && !strncmp(BLUEZ_CHARACTERISTIC_UUID, property, sizeof(BLUEZ_CHARACTERISTIC_UUID))) {

		    r = sd_bus_message_read_basic(m, contents[0], &uuid);
		    CHECK_RETURN(r);
		    fprintf(stderr, "      UUID=%s\n", uuid);
		} else
	        if (contents[0] == 's'
		    && !strncmp(BLUEZ_CHARACTERISTIC_SERVICE, property, sizeof(BLUEZ_CHARACTERISTIC_SERVICE))) {

		    r = sd_bus_message_read_basic(m, contents[0], &service);
		    CHECK_RETURN(r);
		    fprintf(stderr, "      Service=%s\n", service);
		} else {
		    r = sd_bus_message_skip(m, contents);
		}
            }
            r = sd_bus_message_exit_container(m); // 'v', contents
            CHECK_RETURN(r);

            r = sd_bus_message_exit_container(m); // 'e', "sv"
            CHECK_RETURN(r);
        }
        CHECK_RETURN(r);

    }
    r = sd_bus_message_exit_container(m); // 'a', "{sv}"
    CHECK_RETURN(r);

    BLENativePeripheralAddCharacteristicPath(service, uuid, path);
    return 1;
}

static int parseObjectAddDevice(
    BLENativeManager *this, sd_bus_message *m)
{
    int r;
    const char *path;

    sd_bus_message_read_basic(m, 'o', &path);

    fprintf(stderr, "Object %s\n", path);

    r = sd_bus_message_enter_container(m, 'a', "{sa{sv}}");
    CHECK_RETURN(r);
    {

        while ((r = sd_bus_message_enter_container(m, 'e', "sa{sv}")) > 0) {
            const char *interface;

            sd_bus_message_read_basic(m, 's', &interface);
            fprintf(stderr, "  Interface %s\n", interface);

            if (!strncmp(BLUEZ_INTERFACE_DEVICE, interface, sizeof(BLUEZ_INTERFACE_DEVICE))) {
                fprintf(stderr, "    Considering as a device.\n");
                addDevice(this, path, m);
            } else
            if (!strncmp(BLUEZ_INTERFACE_SERVICE, interface, sizeof(BLUEZ_INTERFACE_SERVICE))) {
                fprintf(stderr, "    Considering as a service.\n");
                addService(this, path, m);
            } else
            if (!strncmp(BLUEZ_INTERFACE_CHARACTERISTIC, interface, sizeof(BLUEZ_INTERFACE_CHARACTERISTIC))) {
                fprintf(stderr, "    Considering as a characteristic.\n");
                addCharacteristic(this, path, m);
            } else {
                sd_bus_message_skip(m, "a{sv}");
            }

            r = sd_bus_message_exit_container(m); // 'e', "sa{sv}"
            CHECK_RETURN(r);

        }
        CHECK_RETURN(r);

    }
    r = sd_bus_message_exit_container(m); // 'a', "{sa{sv}}"
    CHECK_RETURN(r);
    return 1;
}
# undef CHECK_RETURN

static int getManagedObjectsCallback(
    sd_bus_message *reply, void *userdata, sd_bus_error *error)
{
    BLENativeManager *this = userdata;
    int r;

    assert(reply);
    fprintf(stderr, "Unity3D_BLE: %s\n", __func__);

    r = sd_bus_message_get_errno(reply);
    if (r > 0) {
        fprintf(stderr, "Unity3D_BLE: %s: error=%d: %s\n",
                __func__, r,    sd_bus_message_get_error(reply)->name);
        return -r;
    }

    /* Parse objects from the message: type = a{oa{sa{sv}}} */
    r = sd_bus_message_enter_container(reply, 'a', "{oa{sa{sv}}}");
    if (r < 0) {
        fprintf(stderr, "Unity3D_BLE: %s: parse failed: %d\n", __func__, r);
        return r;
    }

    while ((r = sd_bus_message_enter_container(reply, 'e', "oa{sa{sv}}")) > 0) {

        r = parseObjectAddDevice(this, reply);
        if (r < 0) {
            return r;
        }

        r = sd_bus_message_exit_container(reply); // 'e', "oa{sa{sv}}"
        if (r < 0) {
            fprintf(stderr, "Unity3D_BLE: %s: parse failed: %d\n", __func__, r);
            return r;
        }
    }
    if (r < 0) {
        fprintf(stderr, "Unity3D_BLE: %s: parse failed: %d\n", __func__, r);
        return r;
    }

    r = sd_bus_message_exit_container(reply); // 'a', "{oa{sa{sv}}}"
    if (r < 0) {
        fprintf(stderr, "Unity3D_BLE: %s: parse failed: %d\n", __func__, r);
        return r;
    }

    return 1;
}

static int interfaceAddedCallback(
    sd_bus_message *reply, void *userdata, sd_bus_error *error)
{
    BLENativeManager *this = userdata;

    fprintf(stderr, "Unity3D_BLE: %s\n", __func__);

    const int rerr = sd_bus_message_get_errno(reply);
    if (rerr > 0) {
        fprintf(stderr, "Unity3D_BLE: %s: error=%d: %s\n",
                __func__, rerr, sd_bus_message_get_error(reply)->name);
        return -rerr;
    }

    const int r = parseObjectAddDevice(this, reply);
    if (r < 0) {
        return r;
    }
    return 1;
}

static int interfaceAddedInstallCallback(
    sd_bus_message *reply, void *userdata, sd_bus_error *error)
{
    fprintf(stderr, "Unity3D_BLE: %s\n", __func__);

    const int rerr = sd_bus_message_get_errno(reply);
    if (rerr > 0) {
        fprintf(stderr, "Unity3D_BLE: %s: error=%d: %s\n",
                __func__, rerr, sd_bus_message_get_error(reply)->name);
        return -rerr;
    }
    return 1;
}

/*
 * Ask for all existing objects, including already discovered devices
 */
void BLENativeGetManagedObjects(BLENativeManager *this)
{
    int r;
    sd_bus_message *m = NULL;
    r = sd_bus_message_new_method_call(
        this->bus,
        &m,
        "org.bluez",
        "/",
        "org.freedesktop.DBus.ObjectManager",
        "GetManagedObjects");
    if (r < 0) {
        fprintf(stderr, "Unity3D_BLE: %s: sd_bus_message_new_method_call: %d", __func__, r);
        return;
    }

    sd_bus_call_async(this->bus, NULL, m, getManagedObjectsCallback, this, 0);
    sd_bus_message_unref(m);
}

void BLENativeInitialise(BLENativeManager *this, void *cs_context)
{
    int r;

    fprintf(stderr, "Unity3D_BLE: %s.\n", __func__);
    if (this->cs_context) {
        fprintf(stderr, "Unity3D_BLE: %s: Already initialised.", __func__);
        return;
    }
    this->cs_context = cs_context;
    assert(NULL == this->bus);
    r = sd_bus_default_system(&this->bus);
    if (r < 0) {
        fprintf(stderr, "Unity3D_BLE: %s: sd_bus_default_system: %d", __func__, r);
        BLENativeDeInitialise(this);
        return;
    }

    // Subscribe to new devices
    r = sd_bus_match_signal_async(
        this->bus,
        NULL,
        "org.bluez",
        "/",
        "org.freedesktop.DBus.ObjectManager",
        "InterfacesAdded",
        interfaceAddedCallback,
        interfaceAddedInstallCallback,
        this);
    if (r < 0) {
        fprintf(stderr, "Unity3D_BLE: %s: sd_bus_match_signal_async: %d", __func__, r);
        return;
    }
}

void BLENativeDeInitialise(BLENativeManager *this)
{
    fprintf(stderr, "Unity3D_BLE: %s.\n", __func__);

    switch (this->state) {
        // XXX
    }

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

/*
  XXX See https://electronics.stackexchange.com/questions/
   82098/ble-scan-interval-and-window
*/

void BLENativeScanStart(
    BLENativeManager *this, char *serviceUUID,
    BLENativeScanDeviceFoundCallback *callback)
{
    sd_bus_message *m = NULL;

    this->deviceFoundCallback = callback;

    BLENativeGetManagedObjects(this);
    
    const int retval = sd_bus_message_new_method_call(
        this->bus,
        &m,
        "org.bluez",
        "/org/bluez/hci0",
        "org.bluez.Adapter1",
        "StartDiscovery");
    if (retval < 0) {
        fprintf(stderr, "Unity3D_BLE: %s: sd_bus_message_new_method_call", __func__);
        return;
    }

    sd_bus_call_async(this->bus, NULL, m, startDiscoveryCallback, this, 0);
    sd_bus_message_unref(m);
}

static int stopDiscoveryCallback(
    sd_bus_message *reply, void *userdata, sd_bus_error *error)
{
    BLENativeManager *this = userdata;

    fprintf(stderr, "Unity3D_BLE: %s.\n", __func__);

    this->deviceFoundCallback = NULL;

    assert(reply);

    return 1;
}
void BLENativeScanStop(BLENativeManager *this)
{
    sd_bus_message *m = NULL;

    const int retval = sd_bus_message_new_method_call(
        this->bus,
        &m,
        "org.bluez",
        "/org/bluez/hci0",
        "org.bluez.Adapter1",
        "StopDiscovery");
    if (retval < 0) {
        fprintf(stderr, "Unity3D_BLE: %s: sd_bus_message_new_method_call", __func__);
        return;
    }

    sd_bus_call_async(this->bus, NULL, m, stopDiscoveryCallback, this, 0);
    sd_bus_message_unref(m);
}

static int connectCallback(
    sd_bus_message *reply, void *userdata, sd_bus_error *error)
{
    BLENativePeripheral *this = userdata;

    fprintf(stderr, "Unity3D_BLE: %s: Connected to %s\n",
	    __func__, this->address);

    return 1;
}

NativeConnection *BLENativeConnect(
    BLENativeManager *this, BLENativePeripheral *p)
{
    sd_bus_message *m = NULL;

    const int retval = sd_bus_message_new_method_call(
        this->bus,
        &m,
        "org.bluez",
	p->path,
        "org.bluez.Device1",
        "ConnectProfile");
    // XXX: Add profile
    if (retval < 0) {
        fprintf(stderr, "Unity3D_BLE: %s: sd_bus_message_new_method_call", __func__);
        return p;
    }

    sd_bus_call_async(this->bus, NULL, m, connectCallback, p, 0);
    sd_bus_message_unref(m);

    return (void*)0;
}

void BLENativeDisconnect(
    BLENativeManager *this, NativeConnection *c)
{
    sd_bus_message *m = NULL;

    const int retval = sd_bus_message_new_method_call(
        this->bus,
        &m,
        "org.bluez",
	p->path,
        "org.bluez.Device1",
        "Disconnect");
    if (retval < 0) {
        fprintf(stderr, "Unity3D_BLE: %s: sd_bus_message_new_method_call", __func__);
	return;
    }

    sd_bus_call_async(this->bus, NULL, m, connectCallback, p, 0);
    sd_bus_message_unref(m);
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
    sd_bus_message *m = NULL;
    const int retval = sd_bus_process(this->bus, &m);
    if (retval < 0) {
        perror("Unity3D_BLENativeManager: sd_bus_process");
        return;
    }
    if (m) {
        const sd_bus_error *error = sd_bus_message_get_error(m);
        fprintf(stderr, "DBUS message: %s %s %s %s %s\n",
            sd_bus_message_get_path(m),
            sd_bus_message_get_interface(m),
            sd_bus_message_get_member(m),
            sd_bus_message_get_sender(m),
            error? error->name: "");
        sd_bus_message_unref(m);
    } else {
        fprintf(stderr, "DBUS: no message\n");
    }

    if (retval == 0) {
        fprintf(stderr, "Unity3D_BLE: BLENativeLinuxHelper: entering sd_bus_wait\n");
        sd_bus_wait(this->bus, -1);
    }

    /* Return back to C#, to be called again */
}

