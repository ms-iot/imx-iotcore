// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <initguid.h>
#include <ntddk.h>
#include <wdf.h>
#include <sercx.h>
#include <ntddser.h>
#include <devpkey.h>
#include <acpiioct.h>
#include "acpiutil.hpp"

#define RESHUB_USE_HELPER_ROUTINES
#include <reshub.h>
