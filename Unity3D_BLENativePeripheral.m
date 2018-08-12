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
    if (self->characteristics) {
       [self->characteristics release];
	self->characteristics = NULL;
    }
    [super dealloc];
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

- (bool)locateMyCharacteristicsWithinServices: (NSArray<CBService *> *)services
{
    for (CBService *service in services) {
        NSLog(@"Considering service %@", service);
        if (service.UUID == service.UUID) {
            [cbperipheral discoverCharacteristics: characteristics
				       forService: service];
        }
        return true;
    }
    return false;
}

- (void)     peripheral:(CBPeripheral *)cbperipheral
    didDiscoverServices:(NSError      *)error
{
    assert(cbperipheral == self->cbperipheral);
    (void)[self locateMyCharacteristicsWithinServices: cbperipheral.services];
}

- (void)                      peripheral:(CBPeripheral *)cbperipheral
    didDiscoverCharacteristicsForService:(CBService    *)service
				   error:(NSError      *)error
{
    assert(cbperipheral == self->cbperipheral);
    for (CBCharacteristic *characteristic in service.characteristics) {
        NSLog(@"Discovered characteristic %@", characteristic);
    }
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

    if (this->service) {
	[this->service release];
	this->service = NULL;
    }

    this->service = [CBUUID UUIDWithString: nsservice];
    [this->service retain]; // Released above or in dealloc

    /*
     * If there is already a CoreBluetooth peripheral,
     * initiate service discovery if needed.
     */
    if (NULL != this->cbperipheral) {
	// First try to find within already found services; if fail, then scan
	if ([this locateMyCharacteristicsWithinServices:
		      [this->cbperipheral services]])
	    return;
		  
        if (this->cbperipheral.state == CBPeripheralStateConnected) {
            NSLog(@"Discovering peripheral %@ for service %@", this, this->service);
            [this->cbperipheral discoverServices: @[this->service]];
        }
    }
}

void BLENativePeripheralSetCharacteristic( /*XXX*/
    BLENativePeripheral *p, char *c)
{
}
    

