/*
 *  Unity3D_BLE — A Unity3D desktop plugin for BlueTooth Low Energy
 *
 *  Created by Pekka Nikander <pekka.nikander@iki.fi> on 08/08/2018.
 *  Placed in public domain.
 */

#import "Unity3D_BLE.h"
#import <CoreBluetooth/CoreBluetooth.h>

#include <stdio.h>

@interface BLE () <CBCentralManagerDelegate, CBPeripheralDelegate>
@end

@implementation BLE {
    @public void *cs_context;
    @public CBCentralManager *manager;
    @public void (*deviceFoundCallback)(void *, BLEPeripheral *, BLEAdvertisementData *, long int);
}

- (id)init
{
    self = [super init];
    NSLog(@"init: %@", self);
    return self;
}

- (void)centralManager:(CBCentralManager *)manager
    didDiscoverPeripheral:(CBPeripheral *)peripheral
    advertisementData:(NSDictionary *)advertisementData
    RSSI:(NSNumber *)RSSI
{
    NSLog(@"Scanning discovered peripheral: self=%ld cs_context=%ld name=%@ RSSI=%@",
          (long)(void*)self, (long)cs_context, peripheral.name, RSSI);
    if (self->deviceFoundCallback) {
        self->deviceFoundCallback(cs_context, peripheral, advertisementData, [RSSI integerValue]);
    }
}


- (void)centralManagerDidUpdateState:(nonnull CBCentralManager *)manager {
    NSLog(@"centralManagerDidUpdateState: %ld", manager.state);
}

@end



/* Unity3D plain C API */

void BLEInitLog(void) {
    /* Redirect NSlog to a known good file */
    char filename[PATH_MAX];
    sprintf(filename, "%s/%s", getenv("HOME"), "Library/Logs/Unity/Plugin.log");
    freopen(filename, "a+", stderr);
    NSLog(@"Log initialised.");
}

BLE *BLECreateContext(void) {
    BLE *this = [[BLE alloc] init];

    return this;
}

void BLEInitialise(BLE *this, void *context)
{
    NSLog(@"Unity3D_BLE: Initialise");
    this->cs_context = context;
    this->manager = [[CBCentralManager alloc] initWithDelegate:this queue:nil];
}

void BLEDeInitialise(BLE *this)
{
    if (this->deviceFoundCallback) {
        BLEScanStop(this);
    }
    
    NSLog(@"Unity3D_BLE: DeInitialise");
    if (this->manager) {
	[this->manager release];
	this->manager = NULL;
    }
}

void BLEScanStart(BLE *this, char *serviceUUIDstring, BLEScanDeviceFoundCallback *callback)
{
    this->deviceFoundCallback = callback;
    NSDictionary *options = @{ CBCentralManagerScanOptionAllowDuplicatesKey: @YES };
    NSArray<CBUUID*> *serviceUUIDs = nil;
    if ((NULL != serviceUUIDstring) && (0 != strlen(serviceUUIDstring))) {
        CBUUID *serviceUUID =
            [CBUUID UUIDWithString:
                [NSString stringWithCString:serviceUUIDstring encoding:NSUTF8StringEncoding]];
        serviceUUIDs = @[serviceUUID];
    }
    NSLog(@"Starting scanning for %@.", serviceUUIDs);
    [this->manager scanForPeripheralsWithServices: serviceUUIDs options:options];
}

void BLEScanStop(BLE *this)
{
    NSLog(@"Stopping scanning.");
    [this->manager stopScan];
    this->deviceFoundCallback = NULL;
    NSLog(@"Stopping scanning...done.");
}

BLEConnection BLEConnect(BLEPeripheral *p) {
    return (BLEConnection)0;
}

void BLEDisconnect(BLEConnection *connection) {
}

void BLEDisconnectAll(BLE *this) {
}

void BLECharacteristicRead(BLEConnection connection, BLECharacteristic c) {
}

void BLECharacteristicWrite(BLEConnection connection, BLECharacteristic c, void *value) {
}

void BLECharacteristicSubscribe(BLEConnection connection, BLECharacteristic c , BLESubscribeDataCallback *callback) {
}

void BLECharacteristicUnsubscribe(BLEConnection connection, BLECharacteristic c) {
}


