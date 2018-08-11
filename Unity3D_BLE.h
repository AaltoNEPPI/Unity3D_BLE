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

typedef CBPeripheral BLEPeripheral;
typedef NSDictionary BLEAdvertisementData;

typedef void BLEConnection;
typedef void *BLECharacteristic;

typedef void BLEScanDeviceFoundCallback(void *cs_context, BLEPeripheral *p, BLEAdvertisementData *add, long int RSSI);
typedef void BLESubscribeDataCallback(void *cs_context, void *dataTBD);

void BLEInitLog(void);
BLE *BLECreateContext(void);
void BLEInitialise(BLE *this, void *cs_context);
void BLEDeInitialise(BLE *this);
void BLEScanStart(BLE *this, char *serviceUUID, BLEScanDeviceFoundCallback *callback);
void BLEScanStop(BLE *this);
BLEPeripheral *BLECreatePeripheral(BLEPeripheral *p);
BLEConnection *BLEConnect(BLE *this, BLEPeripheral *p);
void BLEDisconnect(BLE *this, BLEConnection *connection);
void BLEDisconnectAll(BLE *this);
void BLECharacteristicRead(BLEConnection *connection, BLECharacteristic c);
void BLECharacteristicWrite(BLEConnection *connection, BLECharacteristic c, void *value);
void BLECharacteristicSubscribe(BLEConnection *connection, BLECharacteristic c, BLESubscribeDataCallback *callback);
void BLECharacteristicUnsubscribe(BLEConnection *connection, BLECharacteristic c);

void BLEPeripheralGetIdentifier(BLEPeripheral *peripheral, char *identifier, int len);
void BLEPeripheralGetName(BLEPeripheral *peripheral, char *name, int len);
void BLEPeripheralRelease(BLEPeripheral *peripheral);

#endif /* Unity3D_BLE_h */
