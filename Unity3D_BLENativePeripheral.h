/*
 *  Unity3D_BLE — A Unity3D desktop plugin for BlueTooth Low Energy
 *
 *  Created by Pekka Nikander <pekka.nikander@iki.fi> on 08/08/2018.
 *  Placed in public domain.
 */

#ifndef Unity3D_BLENativePeripheral_h
#define Unity3D_BLENativePeripheral_h

#ifdef __APPLE__

#import <CoreBluetooth/CoreBluetooth.h>

@interface BLENativePeripheral : NSObject {
@public
    CBPeripheral *cbperipheral;
    void (*subscribeDataCallback)(void *, void */*XXX*/);
    CBUUID *service; // XXX Only one service?
    CBService *cbservice;
    NSMutableArray<CBUUID *> *characteristics;
}
@end

#endif

#ifdef __linux__

typedef struct characteristics {
    char *uuid;
    char *path;
} BLENativeCharacteristic;

typedef struct NativePeripheral {
    struct NativeManager *manager;
    char *path;
    char *name;
    int rssi;
    char *service_uuid;
    char *service_path;
    int num_characteristics;
    BLENativeCharacteristic *characteristics;
    UT_hash_handle hh;
} BLENativePeripheral;

BLENativePeripheral *BLENativeCreatePeripheralInternal(
    struct NativeManager *this, const char *path, const char *address, int rssi);

void BLENativePeripheralAddServicePath(
    const char *device_path, const char *uuid, const char *path);
void BLENativePeripheralAddCharacteristicPath(
    const char *service_path, const char *uuid, const char *path);

#endif

BLENativePeripheral *BLENativeCreatePeripheral(void *native_peripheral);

void BLENativePeripheralGetIdentifier    (BLENativePeripheral *p, char *identifier, int len);
void BLENativePeripheralGetName          (BLENativePeripheral *p, char *name, int len);
void BLENativePeripheralSetService       (BLENativePeripheral *p, const char *service);

void BLENativePeripheralAddCharacteristic(BLENativePeripheral *p,
					  const char *characteristic);

void BLENativePeripheralRemoveCharacteristic(BLENativePeripheral *p,
					     const char *characteristic);

void BLENativePeripheralRelease          (BLENativePeripheral *p);

#endif /* Unity3D_BLENativePeripheral_h */
