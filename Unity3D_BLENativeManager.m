/*
 *  Unity3D_BLE — A Unity3D desktop plugin for BlueTooth Low Energy
 *
 *  Created by Pekka Nikander <pekka.nikander@iki.fi> on 08/08/2018.
 *  Placed in public domain.
 */

#import "Unity3D_BLENativeManager.h"
#import <CoreBluetooth/CoreBluetooth.h>

#include <assert.h>
#include <stdio.h>

@interface BLENativeManager () <CBCentralManagerDelegate> {
@public
    void *cs_context; // C# BLE context
    CBCentralManager *cbmanager;
    void (*deviceFoundCallback)(
        void *, CBPeripheral *, NativeAdvertisementData */*XXX*/, long int);
}
@end

@implementation BLENativeManager

- (id)init
{
    self = [super init];
    NSLog(@"BLENativeManager init: %@", self);
    return self;
}

- (id)initWithCSContext: (void*)context manager: (CBCentralManager *)cbmanager
{
    NSLog(@"Unity3D_BLE: Initialise");
    if (self) {
	self->cs_context = context;
	[cbmanager retain];
	self->cbmanager  = cbmanager;
    }
    return self;
}

- (void)dealloc
{
    NSLog(@"BLENativeManager dealloc: %@", self);
    if (self->cbmanager) {
       [self->cbmanager release];
	self->cbmanager = NULL;
    }
    self->cs_context = NULL;
    [super dealloc];
}

- (void)centralManagerDidUpdateState:(nonnull CBCentralManager *)cbmanager
{
    NSLog(@"centralManagerDidUpdateState: %ld", cbmanager.state);
    // XXX
}

- (void)   centralManager:(CBCentralManager *)cbmanager
    didDiscoverPeripheral:(CBPeripheral     *)cbperipheral
        advertisementData:(NSDictionary     *)advertisementData
                     RSSI:(NSNumber         *)RSSI
{
    NSLog(@"Discovered peripheral: cs_context=%ld name=%@ RSSI=%@",
          (long)cs_context, cbperipheral.name, RSSI);

    if (self->deviceFoundCallback) {
        self->deviceFoundCallback(
            cs_context,
            cbperipheral,
            advertisementData/*XXX*/,
            [RSSI integerValue]);
    }
}

- (void)  centralManager:(CBCentralManager *)cbmanager
    didConnectPeripheral:(CBPeripheral     *)cbperipheral
{
    NSLog(@"Connected to peripheral: %@", cbperipheral);
    /* 
     * We must trigger discovering for services from here,
     * since it is possible that the user has set the services
     * it is interested about already before connecting.
     */
    if (cbperipheral.delegate) {
	[cbperipheral.delegate peripheral: cbperipheral didModifyServices: @[]];
    }
}

@end

/*
 * BLE Unity3D plain C API 
 */

void BLENativeInitLog(void)
{
    /* Redirect NSlog to a known good file */
    char filename[PATH_MAX];
    sprintf(filename, "%s/%s", getenv("HOME"), "Library/Logs/Unity/Plugin.log");
    freopen(filename, "a+", stderr);
    NSLog(@"Log initialised.");
}

/**
 * Create a new instance of the native BLE manager.
 *
 * Called by the C# SafeHandle runtime, without any arguments.  Hence, we 
 * must initialise the member variables separately.
 */
BLENativeManager *BLENativeCreateManager(void)
{
    // XXX Test if refactoring init works
    BLENativeManager *this = [[BLENativeManager alloc] init];
    return this;
}

void BLENativeInitialise(BLENativeManager *this, void *context)
{
    NSLog(@"BLENativeInitialise");

    [this initWithCSContext: context
		    manager: [[CBCentralManager alloc]
				 initWithDelegate: this queue: nil]];
}

void BLENativeDeInitialise(BLENativeManager *this)
{
    NSLog(@"BLENativeDeInitialise");

    if (this->deviceFoundCallback) {
        BLENativeScanStop(this); // Clears deviceFoundCallback
    }
    
    // cbmanager released in dealloc 
    [this release]; // For alloc in BLECreateContext
}

void BLENativeScanStart(
    BLENativeManager *this,
    const char *serviceUUIDstring,
    BLENativeScanDeviceFoundCallback callback)
{
    this->deviceFoundCallback = callback;
    NSDictionary *options = @{ CBCentralManagerScanOptionAllowDuplicatesKey: @NO };
    NSArray<CBUUID*> *serviceUUIDs = nil;

    if ((NULL != serviceUUIDstring) && (0 != strlen(serviceUUIDstring))) {
        CBUUID *serviceUUID =
            [CBUUID UUIDWithString:
                [NSString stringWithCString:serviceUUIDstring
                                   encoding:NSUTF8StringEncoding]];
        serviceUUIDs = @[serviceUUID];
    }

    NSLog(@"BLENativeScanStart: for %@", serviceUUIDs);
    [this->cbmanager scanForPeripheralsWithServices: serviceUUIDs options: options];
}

void BLENativeScanStop(BLENativeManager *this)
{
    NSLog(@"BLENativeScanStop");
    this->deviceFoundCallback = NULL;
    [this->cbmanager stopScan];
}


NativeConnection *BLENativeConnect(
    BLENativeManager *this, BLENativePeripheral *p)
{
    [p retain];
    NSLog(@"Connecting to peripheral: %p name=%@", p->cbperipheral, p->cbperipheral.name);
    assert(p->cbperipheral);
    [this->cbmanager connectPeripheral: p->cbperipheral options:nil];
    [p tryLocateMyServiceWithDiscovery: NO];
    return p;
}

void BLENativeDisconnect(BLENativeManager *this, NativeConnection *connection)
{
    BLENativePeripheral *p = connection; // XXX
    NSLog(@"Disconnecting from peripheral: %p name=%@", p->cbperipheral, p->cbperipheral.name);
    assert(p->cbperipheral);
    [this->cbmanager cancelPeripheralConnection: p->cbperipheral];
    [p release];
}

void BLENativeDisconnectAll(BLENativeManager *this)
{
    // XXX TBD
}

#if UNITY_TEST_THREADING
/* XXX REMOVE */
void BLENativeLinuxHelper(BLENativeManager *this)
{
    NSLog(@"LinuxHelper: Sleeping");
    sleep(1);
}
#endif
