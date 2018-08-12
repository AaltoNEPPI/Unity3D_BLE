/*
 *  Unity3D_BLE — A Unity3D desktop plugin for BlueTooth Low Energy
 *
 *  Created by Pekka Nikander <pekka.nikander@iki.fi> on 08/08/2018.
 *  Placed in public domain.
 */

#include <assert.h>
#include <stdio.h>

#include "Unity3D_BLENativeManager.h"

void BLENativeInitLog(void)
{
}

BLENativeManager *BLENativeCreateManager(void)
{
    return (void*)0;
}

void BLENativeInitialise(BLENativeManager *this, void *cs_context)
{
}

void BLENativeDeInitialise(BLENativeManager *this)
{
}

void BLENativeScanStart(
    BLENativeManager *this, char *serviceUUID,
    BLENativeScanDeviceFoundCallback *callback)
{
}

void BLENativeScanStop(BLENativeManager *this)
{
}

NativeConnection *BLENativeConnect(
    BLENativeManager *this, BLENativePeripheral *p)
{
    return (void*)0;
}

void BLENativeDisconnect(
    BLENativeManager *this, NativeConnection *c)
{
}

void BLENativeDisconnectAll(BLENativeManager *this)
{
}

