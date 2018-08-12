/*
 *  Unity3D_BLE — A Unity3D desktop plugin for BlueTooth Low Energy
 *
 *  Created by Pekka Nikander <pekka.nikander@iki.fi> on 08/08/2018.
 *  Placed in public domain.
 */

#ifndef Unity3D_BLENativePeripheral_h
#define Unity3D_BLENativePeripheral_h

#import <CoreBluetooth/CoreBluetooth.h>

@interface BLENativePeripheral : NSObject {
@public
    CBPeripheral *cbperipheral;
    void (*subscribeDataCallback)(void *, void */*XXX*/);
    CBUUID *service; // XXX Only one service?
    NSArray<CBUUID *> *characteristics;
}
@end

BLENativePeripheral *BLENativeCreatePeripheral(void *cbperipheral);

void BLENativePeripheralGetIdentifier    (BLENativePeripheral *p, char *identifier, int len);
void BLENativePeripheralGetName          (BLENativePeripheral *p, char *name, int len);
void BLENativePeripheralSetService       (BLENativePeripheral *p, char *service);
void BLENativePeripheralSetCharacteristic(BLENativePeripheral *p,
					  char *characteristic/*XXX*/); 

void BLENativePeripheralRelease          (BLENativePeripheral *p);

#endif /* Unity3D_BLENativePeripheral_h */
