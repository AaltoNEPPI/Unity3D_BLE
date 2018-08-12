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

#include "Unity3D_BLENativePeripheral.h"

typedef struct {
    int dummy; // XXX
} BLENativeManager;

typedef void NativeConnection;
typedef void NativeAdvertisementData;
typedef void NativePeripheral;

#endif

typedef void BLENativeScanDeviceFoundCallback(
    void *cs_context,
    NativePeripheral *cbp,
    NativeAdvertisementData *add/*XXX*/,
    long int RSSI);

typedef void BLENativeSubscribeDataCallback(
    void *cs_context,
    void *dataTBD/*XXX*/);


void BLENativeInitLog(void);

BLENativeManager *BLENativeCreateManager(void);

void BLENativeInitialise   (BLENativeManager *this, void *cs_context);
void BLENativeDeInitialise (BLENativeManager *this);

void BLENativeScanStart    (BLENativeManager *this,
			    char *serviceUUID,
			    BLENativeScanDeviceFoundCallback *callback);

void BLENativeScanStop     (BLENativeManager *this);

NativeConnection *
     BLENativeConnect      (BLENativeManager *this, BLENativePeripheral *p);
void BLENativeDisconnect   (BLENativeManager *this, NativeConnection *c);
void BLENativeDisconnectAll(BLENativeManager *this);

#endif /* Unity3D_BLENativeManager_h */
