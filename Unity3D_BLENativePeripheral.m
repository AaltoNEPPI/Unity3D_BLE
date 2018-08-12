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
    NSLog(@"BLENativePeripheral init: %@", self);
    if (self) {
        [cbperipheral retain];
        self->cbperipheral = cbperipheral;
	self->service = NULL;
	self->cbservice = NULL;
	self->characteristics = [[NSMutableArray<CBUUID *> alloc] init];
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
                NSLog(@"Discovering characteristics %@", characteristics);
		[cbperipheral discoverCharacteristics: characteristics
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
    for (CBCharacteristic *secha in service.characteristics) {
        for (CBUUID *mychauuid in self->characteristics) {
            if ([mychauuid isEqual: secha.UUID]) {
                NSLog(@"Subscribing to characteristic %@", secha);
		[cbperipheral setNotifyValue: YES forCharacteristic: secha];		            }
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
    NSLog(@"Characteristic %@ new value %@", characteristic, data);
    // parse the data as needed
}

@end

/*
 * Unity3D plain C interface 
 */

BLENativePeripheral *BLENativeCreatePeripheral(void *cbp)
{
    CBPeripheral *cbperipheral = (CBPeripheral *)cbp;
    
    if (cbperipheral.delegate) {
        BLENativePeripheral *this = (BLENativePeripheral *)cbperipheral.delegate;
        [this retain];
        return this;
    } else {
        BLENativePeripheral *this = [[BLENativePeripheral alloc]
                                        initWith: cbperipheral];
        cbperipheral.delegate = this;
        return this;
    }
}

void BLENativePeriperalRelease(BLENativePeripheral *this)
{
    this->cbperipheral.delegate = NULL;
    this->cbperipheral = NULL;
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
    BLENativePeripheral *this, char *servicestring)
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

    /*
     * If there is already a CoreBluetooth peripheral,
     * initiate service discovery if needed.
     */
    [this tryLocateMyServiceWithDiscovery: YES];
}

void BLENativePeripheralAddCharacteristic(
    BLENativePeripheral *this, char *chastring)
{
    NSLog(@"Adding to peripheral %@ characteristic: %s", this, chastring);

    NSString *nscha = [NSString stringWithCString: chastring
                                         encoding: NSUTF8StringEncoding];

    CBUUID *cha = [CBUUID UUIDWithString: nscha];
    [this->characteristics addObject: cha];

    if (NULL != this->cbperipheral && NULL != this->cbservice) {
        if (this->cbperipheral.state == CBPeripheralStateConnected) {
            NSLog(@"Subscribing peripheral %@ to characteristic %@",
                  this, cha);
            [this->cbperipheral
              discoverCharacteristics: this->characteristics
              forService: this->cbservice];
        }
    }
}

void BLENativePeripheralRemoveCharacteristic(
    BLENativePeripheral *this, char *chastrinfg)
{
    // XXX TBD
}
