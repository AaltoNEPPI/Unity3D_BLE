/*
 *  Unity3D_BLE — A Unity3D desktop plugin for BlueTooth Low Energy
 *
 *  Created by Pekka Nikander <pekka.nikander@iki.fi> on 08/08/2018.
 *  Placed in public domain.
 */

#import "Unity3D_BLE.h"
#import <CoreBluetooth/CoreBluetooth.h>

#include <stdio.h>

static CBCentralManager *manager = NULL;
static BLEScanDeviceFoundCallback deviceFoundCallback = NULL;

@interface BLE () <CBCentralManagerDelegate, CBPeripheralDelegate>
@end

@implementation BLE {
}

- (void)centralManager:(CBCentralManager *)manager
    didDiscoverPeripheral:(CBPeripheral *)peripheral
    advertisementData:(NSDictionary *)advertisementData
    RSSI:(NSNumber *)RSSI
{
    if (deviceFoundCallback) {
        deviceFoundCallback(peripheral, advertisementData, [RSSI integerValue]);
    }
}


- (void)centralManagerDidUpdateState:(nonnull CBCentralManager *)central {
    // TBD
}

@end



/* Unity3D plain C API */

void BLEInitialise(void)
{
    if (!manager) {
        /* Redirect NSlog to a known good file */
        char filename[PATH_MAX];
        sprintf(filename, "%s/%s", getenv("HOME"), "Library/Logs/Unity/Plugin.log");
        freopen(filename, "a+", stderr);

        NSLog(@"Unity3D_BLE: Initialise");
        BLE *delegate = [[BLE alloc] init];
        manager = [[CBCentralManager alloc] initWithDelegate:delegate queue:nil];
    }
}

void BLEDeInitialise(void)
{
    if (manager) {
        NSLog(@"Unity3D_BLE: DeInitialise");
        [manager release];
        manager = NULL;
    }
}

void BLEScan(char *serviceUUIDstring, BLEScanDeviceFoundCallback callback)
{
    deviceFoundCallback = callback;
    NSDictionary *options = @{ CBCentralManagerScanOptionAllowDuplicatesKey: @YES };
    CBUUID *serviceUUID = [CBUUID UUIDWithString:
                           [NSString stringWithCString:serviceUUIDstring encoding:NSUTF8StringEncoding]];
    [manager scanForPeripheralsWithServices:@[serviceUUID] options:options];
}

void BLEScanStop(void)
{
    [manager stopScan];
    deviceFoundCallback = NULL;
}

BLEConnection BLEConnect(BLEPeripheral p) {
    return (BLEConnection)0;
}

void BLEDisconnect(BLEConnection connection) {
}

void BLEDisconnectAll(void) {
}

void BLECharacteristicRead(BLEConnection connection, BLECharacteristic c) {
}

void BLECharacteristicWrite(BLEConnection connection, BLECharacteristic c, void *value) {
}

void BLECharacteristicSubscribe(BLEConnection connection, BLECharacteristic c , BLESubscribeDataCallback *callback) {
}

void BLECharacteristicUnsubscribe(BLEConnection connection, BLECharacteristic c) {
}


