// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
//
// Module Name:
//
//  MX6DodDriver.h
//
// Abstract:
//
//    This is MX6DOD driver initialization
//
// Environment:
//
//    Kernel mode only.
//

#ifndef _MX6DODDRIVER_HPP_
#define  _MX6DODDRIVER_HPP_ 1

extern "C" DRIVER_INITIALIZE DriverEntry;

DXGKDDI_UNLOAD MX6DodDdiUnload;

#endif // _MX6DODDRIVER_HPP_
