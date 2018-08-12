/*
 *  Unity3D_BLE — A Unity3D desktop plugin for BlueTooth Low Energy
 *
 *  Created by Pekka Nikander <pekka.nikander@iki.fi> on 08/08/2018.
 *  Placed in public domain.
 */

#ifndef Unity3D_BLENativeManager_h
#define Unity3D_BLENativeManager_h

#import <Foundation/Foundation.h>
#import "Unity3D_BLENativePeripheral.h"

@interface BLENativeManager : NSObject
@end

typedef NSDictionary BLENativeAdvertisementData;

typedef BLENativePeripheral BLENativeConnection;
typedef void *BLENativeCharacteristic;

typedef void BLENativeScanDeviceFoundCallback(
    void *cs_context,
    CBPeripheral *cbp,
    BLENativeAdvertisementData *add/*XXX*/,
    long int RSSI);

typedef void BLENativeSubscribeDataCallback(
    void *cs_context,
    void *dataTBD/*XXX*/);

void BLENativeInitLog(void);

BLENativeManager *BLENativeCreateManager(void);

void BLENativeInitialise  (BLENativeManager *this, void *cs_context);
void BLENativeDeInitialise(BLENativeManager *this);

void BLENativeScanStart   (BLENativeManager *this,
			   char *serviceUUID,
			   BLENativeScanDeviceFoundCallback *bcallback);

void BLENativeScanStop    (BLENativeManager *this);

BLENativeConnection *
     BLENativeConnect      (BLENativeManager *this, BLENativePeripheral *p);
void BLENativeDisconnect   (BLENativeManager *this, BLENativeConnection *c);
void BLENativeDisconnectAll(BLENativeManager *this);

void BLENativeCharacteristicRead       (BLENativeConnection *c,
					BLENativeCharacteristic ch);
void BLENativeCharacteristicWrite      (BLENativeConnection *c,
					BLENativeCharacteristic ch, void *value);
void BLENativeCharacteristicSubscribe  (BLENativeConnection *c,
					BLENativeCharacteristic ch,
					BLENativeSubscribeDataCallback *callback);
void BLENativeCharacteristicUnsubscribe(BLENativeConnection *c,
					BLENativeCharacteristic ch);


#endif /* Unity3D_BLENativeManager_h */
