/*
 *  Unity3D_BLE — A Unity3D desktop plugin for BlueTooth Low Energy
 *
 *  Created by Pekka Nikander <pekka.nikander@iki.fi> on 08/08/2018.
 *  Placed in public domain.
 */

#ifndef Unity3D_BLE_h
#define Unity3D_BLE_h

#include <CoreBluetooth/CoreBluetooth.h>

@interface BLE : NSObject
@end

typedef CBPeripheral *BLEPeripheral;
typedef NSDictionary *BLEAdvertisementData;

typedef void *BLEConnection;
typedef void *BLECharacteristic;

typedef void (*BLEScanDeviceFoundCallback)(BLEPeripheral p, BLEAdvertisementData add, long int RSSI);
typedef void (*BLESubscribeDataCallback)(void *context, void *dataTBD);

void BLEInitialise(void); // Only central supported for now
void BLEDeInitialise(void);
void BLEScan(char *serviceUUID, BLEScanDeviceFoundCallback callback);
void BLEScanStop(void);
BLEConnection BLEConnect(BLEPeripheral p);
void BLEDisconnect(BLEConnection connection);
void BLEDisconnectAll(void);
void BLECharacteristicRead(BLEConnection connection, BLECharacteristic c);
void BLECharacteristicWrite(BLEConnection connection, BLECharacteristic c, void *value);
void BLECharacteristicSubscribe(BLEConnection connection, BLECharacteristic c, BLESubscribeDataCallback *callback);
void BLECharacteristicUnsubscribe(BLEConnection connection, BLECharacteristic c);

#endif /* Unity3D_BLE_h */
