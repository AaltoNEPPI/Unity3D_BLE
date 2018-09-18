/*
 *  Unity3D_BLE — A Unity3D desktop plugin for BlueTooth Low Energy
 *
 *  Created by Pekka Nikander <pekka.nikander@iki.fi> on 08/08/2018.
 *  Placed in public domain.
 */

#import "Unity3D_BLENativePeripheral.h"
#import <CoreBluetooth/CoreBluetooth.h>

#include <assert.h>
#include <stdio.h>

@interface BLENativePeripheral () <CBPeripheralDelegate> 
@end

@implementation BLENativePeripheral 

- (id)initWith: (CBPeripheral *)cbperipheral
{
    self = [super init];
    NSLog(@"BLENativePeripheral init: %@ id: %@", self,
          [[cbperipheral identifier] UUIDString]);
    if (self) {
        self->cbperipheral = cbperipheral;
        [cbperipheral retain];
        self->createCount = 0;
        self->service = NULL;
        self->cs_context = NULL;
        self->cbservice = NULL;
        self->cs_connected = NO;
        self->characteristics
            = [NSMutableDictionary dictionaryWithCapacity: 1];
        [self->characteristics retain];
    }
    return self;
}

- (void)dealloc
{
    NSLog(@"BLENativePeripheral dealloc: %@", self);
    if (self->cbperipheral) {
       [self->cbperipheral release];
        self->cbperipheral = NULL;
    }
    if (self->service) {
       [self->service release];
        self->service = NULL;
    }
    self->cs_context = NULL;
    if (self->cbservice) {
       [self->cbservice release];
        self->cbservice = NULL;
    }
    if (self->characteristics) {
       [self->characteristics release];
        self->characteristics = NULL;
    }
    [super dealloc];
}

- (void)tryLocateMyServiceWithDiscovery: (bool) withDiscovery
{
    NSLog(@"Trying to locate my service");
    if (NULL == self->cbperipheral) {
        NSLog(@"No CBPeripheral yet, giving up");
        return;
    }
    NSArray<CBService *> *services = [self->cbperipheral services];
    for (CBService *cbservice in services) {
        NSLog(@"Considering service %@", service);
        if ([self->service isEqual: cbservice.UUID]) {
            NSLog(@"Adapting service %@", cbservice);

            if (self->cbservice) {
               [self->cbservice release];
                self->cbservice = NULL;
            }

            [cbservice retain]; // Released in SetService or dealloc
            self->cbservice = cbservice;

            if (characteristics) {
                NSArray<CBUUID *> *uuids = [characteristics allKeys];
                NSLog(@"Discovering characteristics %@", uuids);
                [cbperipheral discoverCharacteristics: uuids
                                           forService: cbservice];
            }
        }
        return;
    }

    if (withDiscovery) {
        /* Not found, kick a discovery if connected. */
        if (self->cbperipheral.state == CBPeripheralStateConnected) {
            NSLog(@"Discovering %@ for service %@", self, self->service);
            [self->cbperipheral discoverServices: @[self->service]];
        }
    }
}

- (void)     peripheral:(CBPeripheral *)cbperipheral
    didDiscoverServices:(NSError      *)error
{
    assert(cbperipheral == self->cbperipheral);
    [self tryLocateMyServiceWithDiscovery: NO];
}

- (void)   peripheral:(CBPeripheral         *)cbperipheral
    didModifyServices:(NSArray<CBService *> *)invalidatedServices
{
    assert(cbperipheral == self->cbperipheral);
    NSLog(@"Discovering service %@ for peripheral %@", service, cbperipheral);
    if (service) {
        [cbperipheral discoverServices: @[service]];
    }
}

- (void)                      peripheral:(CBPeripheral *)cbperipheral
    didDiscoverCharacteristicsForService:(CBService    *)service
                                   error:(NSError      *)error
{
    NSLog(@"Discovered characteristics");
    assert(cbperipheral == self->cbperipheral);
    if (error) {
        NSLog(@"Error discovering characteristics: %@", error);
        return;
    }
    for (CBCharacteristic *sercha in service.characteristics) {
        for (CBUUID *mychauuid in [self->characteristics allKeys]) {
            if ([mychauuid isEqual: sercha.UUID]) {
                NSLog(@"Subscribing(1) to characteristic %@", sercha);
                [cbperipheral setNotifyValue: YES forCharacteristic: sercha];
            }
        }
    }
}

- (void)                         peripheral:(CBPeripheral *    )cbperipheral
didUpdateNotificationStateForCharacteristic:(CBCharacteristic *)characteristic
                                      error:(NSError *         )error
{
    if (error) {
        NSLog(@"Error changing notification state: %@", error);
        return;
    }
    NSLog(@"Notification state changed for %@", characteristic);
}

- (void)             peripheral:(CBPeripheral *    )cbperipheral
didUpdateValueForCharacteristic:(CBCharacteristic *)characteristic
                          error:(NSError *         )error {

    NSData *data = characteristic.value;

    CBUUID *UUID = [characteristic UUID];
    NSValue *nscallback = characteristics[UUID];
    if (nscallback) {
        const BLENativeCharacteristicUpdatedCallback callback
           = (BLENativeCharacteristicUpdatedCallback)
                 [nscallback pointerValue];
        if (callback) {
            const char *uuid = [[UUID UUIDString] UTF8String];
            char *uuid2 = strdup(uuid);
            NSLog(@"callback=%p, context=%p, uuid=%s", callback, cs_context, uuid2);
            callback(cs_context, uuid2, [data bytes]);
            free(uuid2);
        }
    }
}

@end

/*
 * Unity3D plain C interface 
 */

BLENativePeripheral *BLENativeCreatePeripheral(void *cbp)
{
    CBPeripheral *cbperipheral = (CBPeripheral *)cbp;
    [cbperipheral retain];

    if (cbperipheral.delegate) {
        BLENativePeripheral *this = (BLENativePeripheral *)cbperipheral.delegate;
        [this retain];
        NSLog(@"BLENativeCreatePeripheral: Already found: %p", this);
        assert(this->createCount > 0);
        this->createCount++;
        return this;
    } else {
        BLENativePeripheral *this = [[BLENativePeripheral alloc]
                                        initWith: cbperipheral];
        assert(this->createCount == 0);
        this->createCount++;
        cbperipheral.delegate = this;
        NSLog(@"BLENativeCreatePeripheral: Created: %p", this);
        return this;
    }
}

void BLENativePeripheralRelease(BLENativePeripheral *this)
{
    NSLog(@"BLENativePeripheralRelease: %p", this);
    assert(this->cbperipheral.delegate == this || this->cbperipheral.delegate == nil);
    assert(this->createCount > 0);
    this->createCount--;
    if (0 == this->createCount) {
        assert (this->cbperipheral.delegate);
        this->cbperipheral.delegate = nil;
    }

    [this release]; // For alloc or retain in BLECreatePeripheral
}

void BLENativePeripheralGetIdentifier(
    BLENativePeripheral *this, char *identifier, int len)
{
    NSString *UUID;
    if (this->cbperipheral) {
        UUID = [[this->cbperipheral identifier] UUIDString];
    } else {
        UUID = @"";
    }
    [UUID getCString: identifier /* [out] */
           maxLength: len
            encoding: NSASCIIStringEncoding];
}

void BLENativePeripheralGetName(
    BLENativePeripheral *this, char *name, int len)
{
    NSString *nsname;
    if (this->cbperipheral) {
        nsname = [this->cbperipheral name];
    } else {
        nsname = @"";
    }
    [nsname getCString: name /* [out] */
             maxLength: len
              encoding: NSASCIIStringEncoding];
}

/**
 * XXX
 *
 */
void BLENativePeripheralSetService(
    BLENativePeripheral *this, const char *servicestring, const void *cs_context)
{
    NSLog(@"Configuring peripheral %@ service: %s", this, servicestring);

    NSString *nsservice =
        [NSString stringWithCString: servicestring
                           encoding: NSUTF8StringEncoding];

    if (this->cbservice) {
        [this->cbservice release]; // Retained in tryDiscoveryMyServices
        this->cbservice = NULL;
    }

    if (this->service) {
        [this->service release];
        this->service = NULL;
    }

    this->service = [CBUUID UUIDWithString: nsservice];
    [this->service retain]; // Released above in if or in dealloc

    this->cs_context = cs_context;

    /*
     * If there is already a CoreBluetooth peripheral,
     * initiate service discovery if needed.
     */
    [this tryLocateMyServiceWithDiscovery: YES];
}

void BLENativePeripheralAddCharacteristic(
    BLENativePeripheral *this,
    const char *uuidstring,
    BLENativeCharacteristicUpdatedCallback callback)
{
    assert(this->characteristics);

    NSString *nsstring = [NSString stringWithCString: uuidstring
                                         encoding: NSUTF8StringEncoding];

    CBUUID *uuid = [CBUUID UUIDWithString: nsstring];
    if (!uuid) {
        NSLog(@"Malformed UUID %@", nsstring);
        return;
    }

    NSValue *object = [NSValue valueWithPointer: callback];
    [uuid retain];
    NSLog(@"Adding characteristic %s to peripheral %@", uuidstring, this);
    NSLog(@"callback=%p", callback);
    [this->characteristics setObject: object forKey: uuid];
    NSLog(@"Checking subscription");
    [uuid release];

    if (NULL != this->cbperipheral && NULL != this->cbservice) {
        if (this->cbperipheral.state == CBPeripheralStateConnected) {
            NSLog(@"Subscribing(2) peripheral %@ to characteristic %@",
                  this, uuid);
            NSArray<CBUUID *> *uuids = [this->characteristics allKeys];
            [this->cbperipheral
                discoverCharacteristics: uuids
                             forService: this->cbservice];
        }
    }
}

void BLENativePeripheralRemoveCharacteristic(
    BLENativePeripheral *this, const char *chastrinfg)
{
    // XXX TBD
}
