Unity3D BlueTooth Low Energy (BLE) Plugin
=========================================

This is a quick and dirty plugin for allowing Unity3D games to access
BlueTooth Low Energy (BLE) peripherals and subscribe to specific
characteristics at those peripherals.

The bulk of this code (all?) was written by Pekka Nikander in
August 2018, as a part of Aalto University [NEPPI course](http://neppi.aalto.fi).

This code is _not_ production quality.  However, it contains a few
interesting aspects for Unity3D plugin developers, such as how to use
C# `SafeHandle` objects and how to pass C# object references for the
native code to retain and pass back, using `GCHandle`.

For the time being (expect this to be fixed), the C# code is not in
this repository but in the
[parent reposity](https://github.com/AaltoNEPPI/Unity_NEPPI_Skeleton).
Why?  Since Unity3D (version 2018.2) is silly in how it detects when
it needs to recompile C# code.


