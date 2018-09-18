/*
 *  Unity3D_BLE — A Unity3D desktop plugin for BlueTooth Low Energy
 *
 *  Created by Pekka Nikander <pekka.nikander@iki.fi> on 08/08/2018.
 *  Placed in public domain.
 */

#ifndef Unity3D_BLENativePeripheral_h
#define Unity3D_BLENativePeripheral_h

typedef void (*BLENativeCharacteristicUpdatedCallback)(
    const void *cs_context,
    const char *uuid,
    const void *data);

#ifdef __APPLE__

#import <CoreBluetooth/CoreBluetooth.h>

@interface BLENativePeripheral : NSObject {
@public
    CBPeripheral *cbperipheral;
    int createCount;
    BLENativeCharacteristicUpdatedCallback subscribeDataCallback;
    CBUUID *service; // XXX Only one service?
    const void *cs_context;
    CBService *cbservice;
    BOOL cs_connected;
    NSMutableDictionary<CBUUID *, NSValue *> *characteristics;
}
- (void)tryLocateMyServiceWithDiscovery: (bool) withDiscovery;
@end

#endif

#ifdef __linux__

typedef struct characteristics {
    char *uuid;
    char *path;
    BLENativeCharacteristicUpdatedCallback callback;
    UT_hash_handle hh;
} BLENativeCharacteristic;

typedef struct NativePeripheral {
    struct NativeManager *manager;
    char *path;
    char *address;
    char *name;
    int rssi;
    char *service_uuid;
    char *service_path;
    const void *cs_context;
    int num_characteristics;
    BLENativeCharacteristic *characteristics;
    UT_hash_handle hh;
    int references;
} BLENativePeripheral;

BLENativePeripheral *BLENativeCreatePeripheralInternal(
    struct NativeManager *this, const char *path, const char *address, int rssi);

void BLENativePeripheralAddServicePath(
    const char *device_path, const char *uuid, const char *path);
void BLENativePeripheralAddCharacteristicPath(
    const char *service_path, const char *uuid, const char *path);

/* XXX: Fix module boundary violation */
void BLENativeGetManagedObjects(struct NativeManager *this);

#endif

#ifdef __TEST__

typedef void BLENativePeripheral;

#endif

BLENativePeripheral *BLENativeCreatePeripheral(void *native_peripheral);

void BLENativePeripheralGetIdentifier    (BLENativePeripheral *p, char *identifier, int len);
void BLENativePeripheralGetName          (BLENativePeripheral *p, char *name, int len);
void BLENativePeripheralSetService       (BLENativePeripheral *p,
					  const char *service,
					  const void *cs_context);

void BLENativePeripheralAddCharacteristic(BLENativePeripheral *p,
                                          const char *characteristic,
                                          BLENativeCharacteristicUpdatedCallback callback);

void BLENativePeripheralRemoveCharacteristic(BLENativePeripheral *p,
                                             const char *characteristic);

void BLENativePeripheralRelease          (BLENativePeripheral *p);

#ifdef __linux__

// XXX Fix module boundary violation
void BLENativeSubscribeToCharacteristic(
    struct NativeManager *this, const char *path, BLENativePeripheral *peri);

void BLENativeNotifyCharacteristic(
    const char *characteristic_path, const void *valuep);

#endif

#endif /* Unity3D_BLENativePeripheral_h */
