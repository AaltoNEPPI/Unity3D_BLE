/*
 *  Unity3D_BLE — A Unity3D desktop plugin for BlueTooth Low Energy
 *
 *  Created by Pekka Nikander <pekka.nikander@iki.fi> on 08/08/2018.
 *  Placed in public domain.
 */

#include "Unity3D_BLENativePeripheral.h"

BLENativePeripheral *BLENativeCreatePeripheral(void *cbperipheral)
{
    return (void*)0;
}

void BLENativePeripheralGetIdentifier(BLENativePeripheral *p, char *identifier, int len)
{
}

void BLENativePeripheralGetName(BLENativePeripheral *p, char *name, int len)
{
}

void BLENativePeripheralSetService(BLENativePeripheral *p, char *service)
{
}

void BLENativePeripheralAddCharacteristic(
    BLENativePeripheral *p, char *characteristic)
{
}

void BLENativePeripheralRemoveCharacteristic(
    BLENativePeripheral *p, char *characteristic)
{
}

void BLENativePeripheralRelease(BLENativePeripheral *p)
{
}
