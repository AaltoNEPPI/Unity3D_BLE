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
    BLENativeScanDeviceFoundCallback deviceFoundCallback;
};

# define CHECK_RETURN(r) if ((r) < 0) { \
        fprintf(stderr, "Unity3D_BLE: %s: parse failed: %d (L%d)\n", \
                __func__, (r), __LINE__);                            \
        return r; \
    }

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

        // fprintf(stderr, "Calling deviceFoundCallback for %s\n", address);

        this->deviceFoundCallback(
            this->cs_context,
            peri,
            NULL,
            rssi);
    } else {
        fprintf(stderr, "NULL deviceFoundCallback for %s\n", address);
    }
}

typedef struct {
    const char *type;
    const char *name;
    int         name_len;
    void       *valuep; // Must correspond to the type
} DBUSProperty;

/**
 * Test if the type is suitable for sd_bus_message_read_array
 *
 * Copied from systemd/src/libsystemd/sd-bus/bus-type.c, where
 * this is but not public.  Silly.  XXX FIXME.
 */
static int bus_type_is_trivial(char c) {
        static const char valid[] = {
                SD_BUS_TYPE_BYTE,
                SD_BUS_TYPE_BOOLEAN,
                SD_BUS_TYPE_INT16,
                SD_BUS_TYPE_UINT16,
                SD_BUS_TYPE_INT32,
                SD_BUS_TYPE_UINT32,
                SD_BUS_TYPE_INT64,
                SD_BUS_TYPE_UINT64,
                SD_BUS_TYPE_DOUBLE
        };

        return !!memchr(valid, c, sizeof(valid));
}

/**
 * Read properties from a dictionary of {name,value} pairs,
 * where the values are variadic but know by the property name
 */
static int readProperties(
    sd_bus_message *m, const DBUSProperty *properties, const size_t count)
{
    int r;

    r = sd_bus_message_enter_container(m, 'a', "{sv}");
    CHECK_RETURN(r);
    {
        while ((r = sd_bus_message_enter_container(m, 'e', "sv")) > 0) {
            const char *property;

            r = sd_bus_message_read_basic(m, 's', &property);
            CHECK_RETURN(r);

            char type;
            const char *contents;
            r = sd_bus_message_peek_type(m, &type, &contents);
            CHECK_RETURN(r);

            if ('v' != type) {
                fprintf(stderr, "Unity3D_BLE: %s: parse failed: %d (L%d)\n",
                        __func__, (r), __LINE__);       \
                return -EBADMSG;
            }

            r = sd_bus_message_enter_container(m, 'v', contents);
            CHECK_RETURN(r);

            int i;
            for (i = 0; i < count; i++) {
                const char type = contents[0];

                if (type != properties[i].type[0])
                    continue;
                if (strncmp(property, properties[i].name, properties[i].name_len))
                    continue;

                switch (type) {
                case SD_BUS_TYPE_ARRAY:
                    if (bus_type_is_trivial(contents[1])) {
                        size_t size; // XXX
                        r = sd_bus_message_read_array(m, contents[1],
                                                      properties[i].valuep,
                                                      &size);
                        break;
                    }
                    /* FALLTHROUGH */
                case SD_BUS_TYPE_VARIANT:
                case SD_BUS_TYPE_STRUCT_BEGIN:
                case SD_BUS_TYPE_DICT_ENTRY_BEGIN:
                    // Handle other containers
                    fprintf(stderr,
                            "        Not implemented yet: Complex property values\n");
                    r = sd_bus_message_skip(m, contents);
                    break;
                default:
                    // Handle basic types
                    r = sd_bus_message_read_basic(m, type, properties[i].valuep);
                }
                CHECK_RETURN(r);

                switch (type) {
                case 's':
                case 'o':
                    fprintf(stderr, "      %s=%s\n", properties[i].name,
                            *(char **)properties[i].valuep);
                    break;
                case 'n':
                    fprintf(stderr, "      %s=%d\n", properties[i].name,
                            *(int *)properties[i].valuep);
                    break;
                default:
                    fprintf(stderr, "      %s=???\n", properties[i].name);
                    break;
                }
                break;
            }
            /* If the caller is not interested, just skip */
            if (count == i) {
                r = sd_bus_message_skip(m, contents);
                CHECK_RETURN(r);
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

    return 1;
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

    const DBUSProperty properties[] = {
        { "s",  BLUEZ_DEVICE_ADDRESS,  sizeof(BLUEZ_DEVICE_ADDRESS),  &address, },
        { "n",  BLUEZ_DEVICE_RSSI,     sizeof(BLUEZ_DEVICE_RSSI),     &rssi,    },
        { "as", BLUEZ_DEVICE_SERVICES, sizeof(BLUEZ_DEVICE_SERVICES), &UUIDs,   },
    };
    const int properties_count = sizeof(properties)/sizeof(properties[0]);

    r = readProperties(m, properties, properties_count);
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
    const char *service;

    const DBUSProperty properties[] = {
        { "o", BLUEZ_SERVICE_DEVICE, sizeof(BLUEZ_SERVICE_DEVICE), &device,  },
        { "s", BLUEZ_SERVICE_UUID,   sizeof(BLUEZ_SERVICE_UUID),   &service, },
    };
    const size_t properties_count = sizeof(properties)/sizeof(properties[0]);

    r = readProperties(m, properties, properties_count);
    CHECK_RETURN(r);

    BLENativePeripheralAddServicePath(device, service, path);
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
    const char *uuid;
    const char *service; /* DBUS service path */

    const DBUSProperty properties[] = {
        { "s", BLUEZ_CHARACT_UUID,    sizeof(BLUEZ_CHARACT_UUID),    &uuid,    },
        { "o", BLUEZ_CHARACT_SERVICE, sizeof(BLUEZ_CHARACT_SERVICE), &service, },
    };
    const size_t properties_count = sizeof(properties)/sizeof(properties[0]);

    r = readProperties(m, properties, properties_count);
    CHECK_RETURN(r);

    BLENativePeripheralAddCharacteristicPath(service, uuid, path);
    return 1;
}

static int parseObjectMaybeAdd(
    BLENativeManager *this, sd_bus_message *m)
{
    int r;
    const char *path;

    sd_bus_message_read_basic(m, 'o', &path);

    // fprintf(stderr, "Object %s\n", path);

    r = sd_bus_message_enter_container(m, 'a', "{sa{sv}}");
    CHECK_RETURN(r);
    {

        while ((r = sd_bus_message_enter_container(m, 'e', "sa{sv}")) > 0) {
            const char *interface;

            sd_bus_message_read_basic(m, 's', &interface);
            // fprintf(stderr, "  Interface %s\n", interface);

            if (!strncmp(BLUEZ_INTERFACE_DEVICE,
                         interface,
                         sizeof(BLUEZ_INTERFACE_DEVICE))) {
                fprintf(stderr, "    Considering as a device.\n");
                addDevice(this, path, m);
            } else
            if (!strncmp(BLUEZ_INTERFACE_SERVICE,
                         interface,
                         sizeof(BLUEZ_INTERFACE_SERVICE))) {
                fprintf(stderr, "    Considering as a service.\n");
                addService(this, path, m);
            } else
            if (!strncmp(BLUEZ_INTERFACE_CHARACTERISTIC,
                         interface,
                         sizeof(BLUEZ_INTERFACE_CHARACTERISTIC))) {
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

        r = parseObjectMaybeAdd(this, reply);
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

    const int r = parseObjectMaybeAdd(this, reply);
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
        fprintf(stderr, "Unity3D_BLE: %s: sd_bus_message_new_method_call: %d",
                __func__, r);
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
    default: break;
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
    BLENativeManager *this, const char *serviceUUID,
    BLENativeScanDeviceFoundCallback callback)
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

    /*
     * Not that on dual-mode devices "Connect" will connect
     * to either EDR or LE, and one apparently cannot easily
     * control which.  I tried to google how to handle this,
     * but (as is too often for Linux) couldn't figure out.
     *
     * I think this is a bug in the Bluez DBUS API.  Maybe
     * "ConnectProfile" could be used, but I couldn't figure
     * out how to use that, either.
     *
     * Hence, this may fail on dual mode devices, such as iPhone.
     */
    const int retval = sd_bus_message_new_method_call(
        this->bus,
        &m,
        "org.bluez",
        p->path,
        "org.bluez.Device1",
        "Connect");
    if (retval < 0) {
        fprintf(stderr, "Unity3D_BLE: %s: sd_bus_message_new_method_call", __func__);
        return p;
    }

    sd_bus_call_async(this->bus, NULL, m, connectCallback, p, 0);
    sd_bus_message_unref(m);

    return p;
}

static int disconnectCallback(
    sd_bus_message *reply, void *userdata, sd_bus_error *error)
{
    BLENativePeripheral *this = userdata;

    fprintf(stderr, "Unity3D_BLE: %s: Disconnected from %s\n",
            __func__, this->address);

    return 1;
}

void BLENativeDisconnect(
    BLENativeManager *this, NativeConnection *c)
{
    BLENativePeripheral *p = c;
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

    sd_bus_call_async(this->bus, NULL, m, disconnectCallback, p, 0);
    sd_bus_message_unref(m);
}

void BLENativeDisconnectAll(BLENativeManager *this)
{
    // TBD
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
        // fprintf(stderr, "DBUS: no message\n");
    }

    if (retval == 0) {
        fprintf(stderr, "Unity3D_BLE: BLENativeLinuxHelper: entering sd_bus_wait\n");
        sd_bus_wait(this->bus, -1);
    }

    /* Return back to C#, to be called again */
}

/**
 * Handle characteristic notifications
 */
static int characteristicNotifyCallback(
    sd_bus_message *reply, void *userdata, sd_bus_error *error)
{
    const char *characteristic_path = userdata;

    int r;
    const char *interface_name = NULL;

    void *value = (void *)0xdeadbeef; // XXX fixme

    const DBUSProperty properties[] = {
        { "ab", BLUEZ_CHARACT_VALUE, sizeof(BLUEZ_CHARACT_VALUE), &value },
    };
    const size_t properties_count = sizeof(properties)/sizeof(properties[0]);

    // "sa{sv}as"
    r = sd_bus_message_read_basic(reply, 's', &interface_name);
    CHECK_RETURN(r);

    r = readProperties(reply, properties, properties_count);
    CHECK_RETURN(r);

    // XXX: Currently we ignore invalidated properties
    r = sd_bus_message_skip(reply, "as");
    CHECK_RETURN(r);

    if (value == (void *)0xdeadbeef) // XXX fixme
        return 1;

    fprintf(stderr, "Unity3D_BLE: %s: interface %s updated: %p\n", __func__, interface_name, value);

    BLENativeNotifyCharacteristic(characteristic_path, value);

    return 1;
}

static int characteristicNotifyInstallCallback(
    sd_bus_message *reply, void *userdata, sd_bus_error *error)
{
    // XXX Not much to be done here?
    fprintf(stderr, "Unity3D_BLE: %s.\n", __func__);
    return 1;
}

static int characteristicNotifyStartCallback(
    sd_bus_message *reply, void *userdata, sd_bus_error *error)
{
    // XXX Not much to be done here?
    fprintf(stderr, "Unity3D_BLE: %s.\n", __func__);
    return 1;
}

void BLENativeSubscribeToCharacteristic(
    BLENativeManager *this, const char *path, BLENativePeripheral *peri)
{
    int r;
    sd_bus_message *m = NULL;

    fprintf(stderr, "Unity3D_BLE: %s: path=%s\n", __func__, path);

    // Subscribe to new notifications
    r = sd_bus_match_signal_async(
        this->bus,
        NULL,
        "org.bluez",
        path,
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        characteristicNotifyCallback,
        characteristicNotifyInstallCallback,
        (char *)/*XXX*/path);
    if (r < 0) {
        fprintf(stderr, "Unity3D_BLE: %s: sd_bus_match_signal_async: %d",
                __func__, r);
        return;
    }
    const int retval = sd_bus_message_new_method_call(
        this->bus,
        &m,
        "org.bluez",
        path,
        "org.bluez.GattCharacteristic1",
        "StartNotify");
    if (retval < 0) {
        fprintf(stderr, "Unity3D_BLE: %s: sd_bus_message_new_method_call",
                __func__);
        return;
    }

    sd_bus_call_async(this->bus, NULL, m,
                      characteristicNotifyStartCallback, peri, 0);
    sd_bus_message_unref(m);
}
