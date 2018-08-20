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

BLENativePeripheral *BLENativeCreatePeripheralInternal(
    struct NativeManager *manager, const char *path, const char *address, int rssi)
{
    BLENativePeripheral *this = malloc(sizeof(BLENativePeripheral));
    assert(this);
    this->manager = manager;
    this->path = strdup(path);
    this->address = strdup(address);
    this->name = NULL;
    this->rssi = rssi;
    this->service_uuid = NULL;
    this->service_path = NULL;
    this->num_characteristics = 0;
    this->characteristics = NULL;
    HASH_ADD_STR(peripherals, path, this);
    fprintf(stderr, "Constructing NativePeripheral %p\n", this);
    return this;
}

/**
 * XXX
 *
 * Called by the C# runtime system to "convert" the type from
 * an IntPtr to a SafeHandle subclass.
 */
BLENativePeripheral *BLENativeCreatePeripheral(void *this)
{
    fprintf(stderr, "Retaining NativePeripheral %p\n", this);
    return this;
}

void BLENativePeripheralRelease(BLENativePeripheral *this)
{
    assert(this);
    fprintf(stderr, "Releasing NativePeripheral %p\n", this);
    HASH_DEL(peripherals, this);
    this->manager = NULL;
    if (this->path) {
	fprintf(stderr, "Releasing path %s\n", this->path);
        free(this->path);
        this->path = NULL;
    }
    if (this->address) {
	fprintf(stderr, "Releasing address %s\n", this->address);
        free(this->address);
        this->address = NULL;
    }
    if (this->name) {
	fprintf(stderr, "Releasing name %s\n", this->name);
        free(this->name);
        this->name = NULL;
    }
    if (this->service_uuid) {
	fprintf(stderr, "Releasing service_uuid %s\n", this->service_uuid);
        free(this->service_uuid);
        this->service_uuid = NULL;
    }
    if (this->service_path) {
	fprintf(stderr, "Releasing service_path %s\n", this->service_path);
        free(this->service_path);
        this->service_path = NULL;
    }
    if (this->num_characteristics) {
        assert(this->characteristics);
        for (int i = 0; i < this->num_characteristics; i++) {
            BLENativeCharacteristic *c = &this->characteristics[i];
            if (c->uuid) {
		fprintf(stderr, "Releasing characteristic uuid %s\n", c->uuid);
                free(c->uuid);
                c->uuid = NULL;
            }
            if (c->path) {
		fprintf(stderr, "Releasing characteristic path %s\n", c->path);
                free(c->path);
                c->path = NULL;
            }
        }
        this->num_characteristics = 0;
	fprintf(stderr, "Releasing characteristics %p\n", this->characteristics);
        free(this->characteristics);
        this->characteristics = NULL;
    }
    assert(NULL == this->characteristics);
    free(this);
    fprintf(stderr, "Releasing NativePeripheral %p....done.\n", this);
}

void BLENativePeripheralGetIdentifier(BLENativePeripheral *this, char *identifier, int len)
{
    strncpy(identifier, this->address, len);
}

void BLENativePeripheralGetName(BLENativePeripheral *this, char *name, int len)
{
    strncpy(name, this->name? this->name: "", len);
}

void BLENativePeripheralSetService(BLENativePeripheral *this, const char *service)
{
    if (this->service_uuid) {
        if (!strcmp(this->service_uuid, service))
            return;
        free(this->service_uuid);
        this->service_uuid = NULL;
    }
    assert(NULL == this->service_uuid);
    this->service_uuid = strdup(service);

    if (this->path) {
        BLENativeGetManagedObjects(this->manager); /* XXX Add filter? */
    }
}

void BLENativePeripheralAddServicePath(
    const char *device_path, const char *uuid, const char *path)
{
    BLENativePeripheral *this;

    HASH_FIND_STR(peripherals, device_path, this);
    if (!this || this->service_uuid)
        return;
    if (0 != strcmp(this->service_uuid, uuid))
        return;

    if (this->service_path) {
        free(this->service_path);
        this->service_path = NULL;
    }
    this->service_path = strdup(path);
}

void BLENativePeripheralAddCharacteristic(
    BLENativePeripheral *this, const char *characteristic)
{
    for (int i = 0; i < this->num_characteristics; i++) {
        BLENativeCharacteristic *c = &this->characteristics[i];
        if (NULL == c->uuid)
            continue;
        if (!strcmp(c->uuid, characteristic))
            return;
    }
    BLENativeCharacteristic *new_c
        = realloc(this->characteristics, (this->num_characteristics+1) * sizeof(BLENativeCharacteristic));
    assert(new_c);
    new_c[this->num_characteristics++].uuid = strdup(characteristic);
    new_c[this->num_characteristics++].path = NULL;
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

    for (int i = 0; i < this->num_characteristics; i++) {
        BLENativeCharacteristic *c = &this->characteristics[i];

        if (NULL != c->uuid)
            continue;
        if (0 != strcmp(c->uuid, uuid))
            continue;

        if (NULL != c->path) {
            free(c->path);
            c->path = NULL;
        }

        c->path = strdup(path);

	BLENativeSubscribeToCharacteristic(this->manager, path, this);

        return;
    }
}

void BLENativePeripheralRemoveCharacteristic(
    BLENativePeripheral *this, const char *characteristic)
{
    for (int i = 0; i < this->num_characteristics; i++) {
        BLENativeCharacteristic *c = &this->characteristics[i];

        if (NULL == c->uuid)
            continue;
        if (0 != strcmp(c->uuid, characteristic))
            continue;

        // XXX: Is this correct?
        memmove(c+1, c, sizeof(c[0])*(this->num_characteristics - (c - this->characteristics)));
        this->num_characteristics--;
        return;
    }
}
