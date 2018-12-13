// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _IMX6_HDMI_H_
#define _IMX6_HDMI_H_

#define HDMI_REGISTERS_LENGTH 0x9000

#define HDMI_PHY_CONF0                                  0x3000
#define HDMI_MC_PHYRSTZ                                 0x4005

#define HDMI_PHY_CONF0_PDZ                              (0x1 << 7)
#define HDMI_PHY_CONF0_ENTMDS                           (0x1 << 6)
#define HDMI_PHY_CONF0_TXPWRON                          (0x1 << 3)

#define HDMI_MC_PHYRSTZ_PHYRSTZ                         (0x1 << 0)

#endif // _IMX6_HDMI_H_
