/*
 *  Unity3D_BLE — A Unity3D desktop plugin for BlueTooth Low Energy
 *
 *  Created by Pekka Nikander <pekka.nikander@iki.fi> on 08/08/2018.
 *  Placed in public domain.
 */

#include <assert.h>
#include <stdio.h>

#include "uthash.h"
#include "Unity3D_BLENativePeripheral.h"

BLENativePeripheral *peripherals = NULL;
BLENativeCharacteristic *characteristics = NULL;

BLENativePeripheral *BLENativeCreatePeripheralInternal(
    struct NativeManager *manager, const char *path, const char *address, int rssi)
{
    BLENativePeripheral *this = NULL;

    HASH_FIND_STR(peripherals, path, this);
    if (!this) {
        this = malloc(sizeof(BLENativePeripheral));
        assert(this);
        memset(this, 0, sizeof(*this));
        /* Redundant... */
        this->manager = manager;
        this->path = strdup(path);
        this->address = strdup(address);
        this->name = NULL;
        this->service_uuid = NULL;
        this->service_path = NULL;
        this->cs_context = NULL;
        this->num_characteristics = 0;
        this->characteristics = NULL;
        HASH_ADD_STR(peripherals, path, this);
        // fprintf(stderr, "Constructing NativePeripheral %p\n", this);
    }
    assert(this);
    assert(0 == strcmp(this->path, path));
    assert(0 == strcasecmp(this->address, address));
    this->rssi = rssi;
    return this;
}

/**
 * XXX
 *
 * Called by the C# runtime system to "convert" the type from
 * an IntPtr to a SafeHandle subclass.
 */
BLENativePeripheral *BLENativeCreatePeripheral(void *p)
{
    BLENativePeripheral *this = p;

    // fprintf(stderr, "Retaining NativePeripheral %p\n", this);
    this->references++;
    return this;
}

void BLENativePeripheralRelease(BLENativePeripheral *this)
{
    assert(this);
    if (--(this->references) > 0)
        return;

    // fprintf(stderr, "Releasing NativePeripheral %p\n", this);
    HASH_DEL(peripherals, this);
    this->manager = NULL;
    if (this->path) {
        free(this->path);
        this->path = NULL;
    }
    if (this->address) {
        free(this->address);
        this->address = NULL;
    }
    if (this->name) {
        free(this->name);
        this->name = NULL;
    }
    if (this->service_uuid) {
        free(this->service_uuid);
        this->service_uuid = NULL;
    }
    if (this->service_path) {
        free(this->service_path);
        this->service_path = NULL;
    }
    this->cs_context = NULL;
    if (this->num_characteristics) {
        assert(this->characteristics);
        for (int i = 0; i < this->num_characteristics; i++) {
            BLENativeCharacteristic *c = &this->characteristics[i];
            if (c->uuid) {
                free(c->uuid);
                c->uuid = NULL;
            }
            if (c->path) {
                free(c->path);
                c->path = NULL;
            }
        }
        this->num_characteristics = 0;
        free(this->characteristics);
        this->characteristics = NULL;
    }
    assert(NULL == this->characteristics);
    free(this);
}

void BLENativePeripheralGetIdentifier(BLENativePeripheral *this, char *identifier, int len)
{
    strncpy(identifier, this->address, len);
}

void BLENativePeripheralGetName(BLENativePeripheral *this, char *name, int len)
{
    strncpy(name, this->name? this->name: "", len);
}

void BLENativePeripheralSetService(
    BLENativePeripheral *this, const char *service, const void *cs_context)
{
    if (this->service_uuid) {
        if (!strcasecmp(this->service_uuid, service))
            return;
        free(this->service_uuid);
        this->service_uuid = NULL;
    }
    assert(NULL == this->service_uuid);
    this->service_uuid = strdup(service);

    this->cs_context = cs_context;

    if (this->path) {
        BLENativeGetManagedObjects(this->manager); /* XXX Add filter? */
    }
}

void BLENativePeripheralAddServicePath(
    const char *device_path, const char *uuid, const char *path)
{
    BLENativePeripheral *this;

    HASH_FIND_STR(peripherals, device_path, this);
    if (!this) {
        fprintf(stderr, "AddServicePath: device not found: %s\n", device_path);
        return;
    }
    if (!this->service_uuid) {
        fprintf(stderr, "AddServicePath: device has no service: %s\n", device_path);
        return;
    }
    if (0 != strcasecmp(this->service_uuid, uuid)) {
        fprintf(stderr, "AddServicePath: uuid mismatch: %s\n", this->service_uuid);
        return;
    }

    if (this->service_path) {
        free(this->service_path);
        this->service_path = NULL;
    }
    this->service_path = strdup(path);
}

void BLENativePeripheralAddCharacteristic(
    BLENativePeripheral *this,
    const char *characteristic,
    BLENativeCharacteristicUpdatedCallback callback)
{
    for (int i = 0; i < this->num_characteristics; i++) {
        BLENativeCharacteristic *c = &this->characteristics[i];
        if (NULL == c->uuid)
            continue;
        if (!strcasecmp(c->uuid, characteristic))
            return;
    }
    BLENativeCharacteristic *new_c
        = realloc(this->characteristics, (this->num_characteristics+1) * sizeof(BLENativeCharacteristic));
    assert(new_c);
    memset(new_c + this->num_characteristics, 0, sizeof(new_c[this->num_characteristics]));
    new_c[this->num_characteristics].uuid = strdup(characteristic);
    new_c[this->num_characteristics].path = NULL;
    new_c[this->num_characteristics].callback = callback;
    this->num_characteristics++;
    this->characteristics = new_c;

    if (this->path) {
        BLENativeGetManagedObjects(this->manager);  /* XXX Add filter? */
    }
}

void BLENativePeripheralAddCharacteristicPath(
    const char *service_path, const char *uuid, const char *path)
{
    BLENativePeripheral *this;

    char *device_path = strdup(service_path);
    char *device_path_end = strrchr(device_path, '/');
    assert(device_path_end);
    *device_path_end = '\0';

    HASH_FIND_STR(peripherals, device_path, this);
    free(device_path);

    if (!this || !this->service_path)
        return;

    if (0 != strcmp(this->service_path, service_path))
        return;

    fprintf(stderr, "AddCharacteristicPath: considering %s, n=%d\n", uuid,
        this->num_characteristics);

    for (int i = 0; i < this->num_characteristics; i++) {
        BLENativeCharacteristic *c = &this->characteristics[i];

        assert(NULL != c->uuid);
        if (0 != strcasecmp(c->uuid, uuid)) {
            fprintf(stderr, "AddCharacteristicPath: mismatch %d %s\n", i, c->uuid);
            continue;
        }

        fprintf(stderr, "AddChacteristicPath: found %s\n", uuid);

        if (NULL != c->path) {
            if (0 == strcasecmp(c->path, path)) {
                fprintf(stderr, "AddCharacteristicPath: already subscribed %s\n", uuid);
                return;
            }
            free(c->path);
            c->path = NULL;
        }

        c->path = strdup(path);

        HASH_ADD_STR(characteristics, path, c);

        fprintf(stderr, "AddCharacteristicPath: subscribing to %s\n", uuid);
        BLENativeSubscribeToCharacteristic(this->manager, c->path, this);

        return;
    }
}

void BLENativeNotifyCharacteristic(
    const char *characteristic_path,
    const void *valuep)
{
    BLENativeCharacteristic *c;

    HASH_FIND_STR(characteristics, characteristic_path, c);

    fprintf(stderr, "%s: path=%s, uuid=%s\n", __func__, characteristic_path, c->uuid);

    if (c->callback) {
        c->callback(XXX->cs_context, c->uuid, valuep);
    }
}

void BLENativePeripheralRemoveCharacteristic(
    BLENativePeripheral *this, const char *characteristic)
{
    for (int i = 0; i < this->num_characteristics; i++) {
        BLENativeCharacteristic *c = &this->characteristics[i];

        if (NULL == c->uuid)
            continue;
        if (0 != strcasecmp(c->uuid, characteristic))
            continue;

        assert(c->uuid);
        free(c->uuid);

        if (c->path) {
            HASH_DEL(characteristics, c);
            free(c->path);
            c->path = NULL;
        }

        // XXX: Is this correct?
        memmove(c+1, c, sizeof(c[0])*(this->num_characteristics - (c - this->characteristics)));
        this->num_characteristics--;
        return;
    }
}
