/*
 *  Unity3D_BLE — A Unity3D desktop plugin for BlueTooth Low Energy
 *
 *  Created by Pekka Nikander <pekka.nikander@iki.fi> on 08/08/2018.
 *  Placed in public domain.
 */

#ifndef Unity3D_BLENativeManager_h
#define Unity3D_BLENativeManager_h

#ifdef __APPLE__

#import <Foundation/Foundation.h> // Needed for CBPeripheral
#import "Unity3D_BLENativePeripheral.h"

@interface BLENativeManager : NSObject
@end

typedef BLENativePeripheral NativeConnection;

typedef NSDictionary NativeAdvertisementData;
typedef CBPeripheral NativePeripheral;

#endif

#ifdef __linux__

#include <uthash.h>
#include "Unity3D_BLENativePeripheral.h"

typedef struct NativeManager BLENativeManager;

typedef void NativeConnection;
typedef void NativeAdvertisementData;
typedef struct NativePeripheral NativePeripheral;

/* XXX: Convert into a data structure */
#define BLUEZ_INTERFACE_DEVICE         "org.bluez.Device1"
#define BLUEZ_INTERFACE_SERVICE        "org.bluez.GattService1"
#define BLUEZ_INTERFACE_CHARACTERISTIC "org.bluez.GattCharacteristic1"

/* XXX: Convert into a data structure */
#define BLUEZ_DEVICE_ADDRESS  "Address"
#define BLUEZ_DEVICE_RSSI     "RSSI"
#define BLUEZ_DEVICE_SERVICES "UUIDs"

#define BLUEZ_SERVICE_UUID    "UUID"
#define BLUEZ_SERVICE_DEVICE  "Device"

#define BLUEZ_CHARACT_UUID    "UUID"
#define BLUEZ_CHARACT_SERVICE "Service"
#define BLUEZ_CHARACT_VALUE   "Value"

void BLENativeLinuxHelper  (BLENativeManager *this); // Only in Linux

#endif

#ifdef __TEST__

typedef void *BLENativeManager;
typedef void *NativeConnection;
typedef void *NativeAdvertisementData;
typedef void NativePeripheral;
typedef void BLENativePeripheral; // XXX Module violation?

#endif

typedef void (*BLENativeScanDeviceFoundCallback)(
    void *cs_context,
    NativePeripheral *cbp,
    NativeAdvertisementData *add/*XXX*/,
    long int RSSI);

void BLENativeInitLog(void);

/**
 * Returns a native (Mac OS X or Linux) binary object.
 *
 * This function is called by the C# level to create a native level
 * binary object, referenced from the C# level.  The same reference
 * is passed back as `this` int he subsequent calls.
 */
BLENativeManager *BLENativeCreateManager(void);

void BLENativeInitialise   (BLENativeManager *this, void *cs_context);
void BLENativeDeInitialise (BLENativeManager *this);

void BLENativeScanStart    (BLENativeManager *this,
                            const char *serviceUUID,
                            BLENativeScanDeviceFoundCallback callback);

void BLENativeScanStop     (BLENativeManager *this);

NativeConnection *
     BLENativeConnect      (BLENativeManager *this, BLENativePeripheral *p);
void BLENativeDisconnect   (BLENativeManager *this, NativeConnection *c);
void BLENativeDisconnectAll(BLENativeManager *this);

#endif /* Unity3D_BLENativeManager_h */
