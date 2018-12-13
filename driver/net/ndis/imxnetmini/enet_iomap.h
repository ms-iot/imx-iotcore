/*
* Copyright 2018 NXP
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted (subject to the limitations in the disclaimer
* below) provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice, this
* list of conditions and the following disclaimer.
*
* * Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
*
* * Neither the name of NXP nor the names of its contributors may be used to
* endorse or promote products derived from this software without specific prior
* written permission.
*
* NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS
* LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
* THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
* GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
* OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* 
*/

#ifndef _ENET_IOMAP_H
#define _ENET_IOMAP_H

// MMFR
#define ENET_MMFR_TA_VALUE                         2
#define ENET_MMFR_OP_WRITE                         1
#define ENET_MMFR_OP_READ                          2
#define ENET_MMFR_ST_VALUE                         1

#define ENET_MAC_RX_OPD_DEFAULT_VALUE              0xFFF0
#define ENET_MAC_RX_OPD_MIN_VALUE                  0x0
#define ENET_MAC_RX_OPD_MAX_VALUE                  0xFFFF
#define ENET_MAC_RX_SECTION_FULL_DEFAULT_VALUE     12    // RSFL=12
#define ENET_MAC_RX_SECTION_FULL_MIN_VALUE         0
#define ENET_MAC_RX_SECTION_FULL_MAX_VALUE         0x1FF
#define ENET_MAC_RX_SECTION_EMPTY_DEFAULT_VALUE    0x68  // RSEM=104
#define ENET_MAC_RX_SECTION_EMPTY_MIN_VALUE        0
#define ENET_MAC_RX_SECTION_EMPTY_MAX_VALUE        0x1FF
#define ENET_MAC_RX_ALMOST_EMPTY_DEFAULT_VALUE     8     // RAEM=8
#define ENET_MAC_RX_ALMOST_EMPTY_MIN_VALUE         4
#define ENET_MAC_RX_ALMOST_EMPTY_MAX_VALUE         0x1FF
#define ENET_MAC_RX_ALMOST_FULL_DEFAULT_VALUE      4     // RAFL=4
#define ENET_MAC_RX_ALMOST_FULL_MIN_VALUE          4
#define ENET_MAC_RX_ALMOST_FULL_MAX_VALUE          0x1FF

#define BF_ENET_MAC_TFW_TFWR_DEFAULT_VALUE         0x1F
#define BF_ENET_MAC_TFW_TFWR_MIN_VALUE             0
#define BF_ENET_MAC_TFW_TFWR_MAX_VALUE             0x3F
#define ENET_MAC_TX_ALMOST_EMPTY_DEFAULT_VALUE     8     // TAEM=8
#define ENET_MAC_TX_ALMOST_EMPTY_MIN_VALUE         4
#define ENET_MAC_TX_ALMOST_EMPTY_MAX_VALUE         0x1FF
#define ENET_MAC_TX_ALMOST_FULL_DEFAULT_VALUE      8     // TAFL=8
#define ENET_MAC_TX_ALMOST_FULL_MIN_VALUE          8
#define ENET_MAC_TX_ALMOST_FULL_MAX_VALUE          0x1FF
#define ENET_MAC_TX_SECTION_EMPTY_DEFAULT_VALUE    0x1C0 // TSEM  7/8 full
#define ENET_MAC_TX_SECTION_EMPTY_MIN_VALUE        0
#define ENET_MAC_TX_SECTION_EMPTY_MAX_VALUE        0x1FF

#define BIT_FIELD_VAL(_bit_field_name,_val_) (((_val_)<<_bit_field_name##_SHIFT) & (_bit_field_name##_MASK))

/*
 * ENET_EIR - ENET Interrupt Event Register
 */
typedef union {
    UINT32 U;
    struct {
        unsigned RSRVD_0_14 : 15;
        unsigned TS_TIMER   :  1;
        unsigned TS_AVAIL   :  1;
        unsigned WAKEUP     :  1;
        unsigned PLR        :  1;
        unsigned UN         :  1;
        unsigned RL         :  1;
        unsigned LC         :  1;
        unsigned EBERR      :  1;
        unsigned MII        :  1;
        unsigned RXB        :  1;
        unsigned RXF        :  1;
        unsigned TXB        :  1;
        unsigned TXF        :  1;
        unsigned GRA        :  1;
        unsigned BABT       :  1;
        unsigned BABR       :  1;
        unsigned RSRVD_31   :  1;
    } B;
} EIR_t;
                                               
#define ENET_EIR_TS_TIMER_MASK                 0x00008000
#define ENET_EIR_TS_AVAIL_MASK                 0x00010000
#define ENET_EIR_WAKEUP_MASK                   0x00020000
#define ENET_EIR_PLR_MASK                      0x00040000
#define ENET_EIR_UN_MASK                       0x00080000
#define ENET_EIR_RL_MASK                       0x00100000
#define ENET_EIR_LC_MASK                       0x00200000
#define ENET_EIR_EBERR_MASK                    0x00400000
#define ENET_EIR_MII_MASK                      0x00800000
#define ENET_EIR_RXB_MASK                      0x01000000
#define ENET_EIR_RXF_MASK                      0x02000000
#define ENET_EIR_TXB_MASK                      0x04000000
#define ENET_EIR_TXF_MASK                      0x08000000
#define ENET_EIR_GRA_MASK                      0x10000000
#define ENET_EIR_BABT_MASK                     0x20000000
#define ENET_EIR_BABR_MASK                     0x40000000

/*
 * ENET_ECR - ENET Ethernet Control Register
 */
typedef union {
    UINT32  U;
    struct {
        unsigned RESET      :  1;
        unsigned ETHER_EN   :  1;
        unsigned MAGIC_ENA  :  1;
        unsigned SLEEP      :  1;
        unsigned ENA_1588   :  1;
        unsigned SPEED      :  1;
        unsigned DBG_EN     :  1;
        unsigned STOP_EN    :  1;
        unsigned DBSWP      :  1;
        unsigned RSRVD_9_31 : 23;
    } B;
} ECR_t;

#define ENET_ECR_RESET_MASK                    0x00000001
#define ENET_ECR_ETHER_EN_MASK                 0x00000002
#define ENET_ECR_MAGICPKT_EN_MASK              0x00000004
#define ENET_ECR_SLEEP_EN_MASK                 0x00000008
#define ENET_ECR_EN1588_EN_MASK                0x00000010
#define ENET_ECR_SPEED_MASK                    0x00000020
#define ENET_ECR_DBG_EN_MASK                   0x00000040
#define ENET_ECR_STOP_EN_MASK                  0x00000080
#define ENET_ECR_DBSW_MASK                     0x00000100

/*
 * ENET_MMFR - MII Management Frame Register
 */
typedef union {
    UINT32  U;
    struct {
        unsigned DATA        : 16;
        unsigned TA          :  2;
        unsigned RA          :  5;
        unsigned PA          :  5;
        unsigned OP          :  2;
        unsigned ST          :  2;
    } B;
} MMFR_t;

#define ENET_MMFR_ST_MASK                      0xC0000000
#define ENET_MMFR_OP_MASK                      0x30000000
#define ENET_MMFR_PA_MASK                      0x0F800000
#define ENET_MMFR_RA_MASK                      0x007C0000
#define ENET_MMFR_TA_MASK                      0x00030000
#define ENET_MMFR_DATA_MASK                    0x0000FFFF

#define ENET_MMFR_ST_SHIFT                     30
#define ENET_MMFR_OP_SHIFT                     28
#define ENET_MMFR_PA_SHIFT                     23
#define ENET_MMFR_RA_SHIFT                     18
#define ENET_MMFR_TA_SHIFT                     16
#define ENET_MMFR_DATA_SHIFT                   0

/*
 * ENET_MSCR - MII Speed Control Register
 */
typedef union {
    UINT32  U;
    struct {
        unsigned RSRVD_0     :  1;
        unsigned MII_SPEED   :  6;
        unsigned DIS_PRE     :  1;
        unsigned HOLDTIME    :  3;
        unsigned RSRVD_11_31 : 21;
    } B;
} MSCR_t;

#define ENET_MSCR_MII_SPEED_MASK               0x0000007E
#define ENET_MSCR_DIS_PRE_MASK                 0x00000080
#define ENET_MSCR_HOLDTIME_MASK                0x00000700

#define ENET_MSCR_MII_SPEED_SHIFT              1
#define ENET_MSCR_DIS_PRE_SHIFT                7
#define ENET_MSCR_HOLDTIME_SHIFT               8

/*
 * ENET_MIBC - ENET MIB Control Register
 */
typedef union {
    UINT32  U;
    struct {
        unsigned RSRVD_0_28  : 29;
        unsigned MIB_CLEAR   :  1;
        unsigned MIB_IDLE    :  1;
        unsigned MIB_DIS     :  1;
    } B;
} MIBC_t;

#define ENET_MIBC_MIB_CLEAR_MASK               0x20000000
#define ENET_MIBC_MIB_IDLE_MASK                0x40000000
#define ENET_MIBC_MIB_DIS_MASK                 0x80000000

/*
 * ENET_RCR - ENET Receive Control Register
 */
typedef union {
    UINT32  U;
    struct {
        unsigned LOOP           :  1;
        unsigned DRT            :  1;
        unsigned MII_MODE       :  1;
        unsigned PROM           :  1;
        unsigned BC_REJ         :  1;
        unsigned FCE            :  1;
        unsigned RGMII_ENA      :  1;
        unsigned RSVD_7         :  1;
        unsigned RMII_MODE      :  1;
        unsigned RMII_10T       :  1;
        unsigned RRSRVD_10_11   :  2;
        unsigned PAD_EN         :  1;
        unsigned PAUSE_FWD      :  1;
        unsigned CRC_FWD        :  1;
        unsigned CFEN           :  1;
        unsigned MAX_FL         : 14;
        unsigned NLC            :  1;
        unsigned GRS            :  1;
    } B;
} RCR_t;

#define ENET_RCR_LOOP_MASK                     0x00000001
#define ENET_RCR_DRT_MASK                      0x00000002
#define ENET_RCR_MII_MODE_MASK                 0x00000004
#define ENET_RCR_PROM_MASK                     0x00000008
#define ENET_RCR_BC_REJ_MASK                   0x00000010
#define ENET_RCR_FCE_MASK                      0x00000020
#define ENET_RCR_RGMII_EN_MASK                 0x00000040
#define ENET_RCR_RMII_MODE_MASK                0x00000100
#define ENET_RCR_RMII_10T_MASK                 0x00000200
#define ENET_RCR_PAD_EN_MASK                   0x00001000
#define ENET_RCR_PAUSE_FWD_MASK                0x00002000
#define ENET_RCR_CRC_FWD_MASK                  0x00004000
#define ENET_RCR_CFEN_MASK                     0x00008000
#define ENET_RCR_MAX_FL_MASK                   0x00010000
#define ENET_RCR_NLC_MASK                      0x40000000
#define ENET_RCR_GRS_MASK                      0x80000000

#define ENET_RCR_MAX_FL_SHIFT                  16

/*
 * ENET_TCR - ENET Transmit Control Register
 */
typedef union {
    UINT32  U;
    struct {
        unsigned GTS          :  1;
        unsigned RSRVD_0      :  1;
        unsigned FEDN         :  1;
        unsigned TFC_PAUSE    :  1;
        unsigned RFC_PAUSE    :  1;
        unsigned ADD_SEL      :  3;
        unsigned ADD_INS      :  1;
        unsigned CRC_FWD      :  1;
        unsigned RSRVD_11_31  : 22;
    } B;
} TCR_t;

#define ENET_TCR_GTS_MASK                      0x00000001
#define ENET_TCR_FEDN_MASK                     0x00000004
#define ENET_TCR_TFC_PAUSE_MASK                0x00000008
#define ENET_TCR_RFC_PAUSE_MASK                0x00000010
#define ENET_TCR_ADD_SEL_MASK                  0x000000E0
#define ENET_TCR_ADD_INS_MASK                  0x00000100
#define ENET_TCR_CRC_FWD_MASK                  0x00000200

/*
 * ENET_TFWR - ENET Transmit FIFO Watermark Register
 */
typedef union {
    UINT32  U;
    struct {
        unsigned TFWR       :  6;
        unsigned RSVD_6_7   :  2;
        unsigned STRFWD     :  1;
        unsigned RSRVD_9_31 : 23;
    } B;
} TFWR_t;

#define ENET_TFWR_TFWR_MASK                    0x0000003F
#define ENET_TCR_STRFWD_MASK                   0x00000100

/*
 * ENET_TACC - ENET Transmit Accelerator Function Configuration
 */
typedef union {
    UINT32  U;
    struct {
        unsigned SHIFT16    :  1;
        unsigned RSVD_1_2   :  2;
        unsigned IPCHK      :  1;
        unsigned PROCHK     :  1;
        unsigned RSRVD_5_31 : 27;
    } B;
} TACC_t;

#define ENET_TACC_SHIFT16_MASK                 0x00000001
#define ENET_TACC_IPCHK_MASK                   0x00000004
#define ENET_TACC_PROCHK6_MASK                 0x00000010

/*
 * ENET_RACC - ENET Receive Accelerator Function Configuration
 */
typedef union {
    UINT32  U;
    struct {
        unsigned PADREM     :  1;
        unsigned IPDIS      :  1;
        unsigned PRODIS     :  1;
        unsigned RSVD_3_5   :  3;
        unsigned LINEDIS    :  1;
        unsigned SHIFT16    :  1;
        unsigned RSRVD_8_31 : 24;
    } B;
} RACC_t;

#define ENET_RACC_PADREM_MASK                  0x00000001
#define ENET_RACC_IPDIS_MASK                   0x00000002
#define ENET_RACC_PRODIS_MASK                  0x00000004
#define ENET_RACC_LINEDIS_MASK                 0x00000040
#define ENET_RACC_SHIFT16_MASK                 0x00000080

/*
 * ENET_OPD - ENET Opcode/Pause Duration Register
 */
typedef union {
    UINT32  U;
    struct {
        unsigned PAUSE_DUR  :  16;
        unsigned OPCODE     :  16;
    } B;
} OPD_t;

#define ENET_OPD_PAUSE_DUR_MASK                0x0000FFFF
#define ENET_OPD_OPCODE_MASK                   0xFFFF0000

//------------------------------------------------------------------------------
// REGISTER LAYOUT
//------------------------------------------------------------------------------
typedef struct {
    UINT32  ___RES_000;
    EIR_t   EIR;                // 004
    EIR_t   EIMR;               // 008
    UINT32  ___RES_00C;
    UINT32  RDAR;               // 010
    UINT32  TDAR;               // 014
    UINT32  ___RES_018[3];
    ECR_t   ECR;                // 024
    UINT32  ___RES_028[6];
    MMFR_t  MMFR;               // 040
    MSCR_t  MSCR;               // 044
    UINT32  ___RES_048[7];
    MIBC_t  MIBC;               // 064
    UINT32  ___RES_068[7];
    RCR_t   RCR;                // 084
    UINT32  ___RES_088[15];
    TCR_t   TCR;                // 0C4
    UINT32  ___RES_0C8[7];
    UINT32  PALR;               // 0E4
    UINT32  PAUR;               // 0E8
    OPD_t   OPD;                // 0EC
    UINT32  ___RES_0F0[10];
    UINT32  IAUR;               // 118
    UINT32  IALR;               // 11C
    UINT32  GAUR;               // 120
    UINT32  GALR;               // 124
    UINT32  ___RES_128[7];
    TFWR_t  TFWR;               // 144
    UINT32  ___RES_D148[14];
    UINT32  ERDSR;              // 180
    UINT32  ETDSR;              // 184
    UINT32  EMRBR;              // 188
    UINT32  ___RES_18C;
    UINT32  RSFL;               // 190
    UINT32  RSEM;               // 194
    UINT32  RAEM;               // 198
    UINT32  RAFL;               // 19C
    UINT32  TSEM;               // 1A0
    UINT32  TAEM;               // 1A4
    UINT32  TAFL;               // 1A8
    UINT32  TIPG;               // 1AC
    UINT32  FTRL;               // 1B0
    UINT32  ___RES_1B4[3];
    TACC_t  TACC;               // 1C0
    RACC_t  RACC;               // 1C4
    UINT32  ___RES_1C8[14];

    // statistics
    UINT32  RMON_T_DROP_NI;     // 200
    UINT32  RMON_T_PACKETS;     // 204
    UINT32  RMON_T_BC_PKT;      // 208
    UINT32  RMON_T_MC_PKT;      // 20C
    UINT32  RMON_T_CRC_ALIGN;   // 210
    UINT32  RMON_T_UNDERSIZE;   // 214
    UINT32  RMON_T_OVERSIZE;    // 218
    UINT32  RMON_T_FRAG;        // 21C
    UINT32  RMON_T_JAB;         // 220
    UINT32  RMON_T_COL;         // 224
    UINT32  RMON_T_P64;         // 228
    UINT32  RMON_T_P65TO127;    // 22C
    UINT32  RMON_T_P128TO255;   // 230
    UINT32  RMON_T_P256TO511;   // 234
    UINT32  RMON_T_P512TO1023;  // 238
    UINT32  RMON_T_P1024TO2047; // 23C
    UINT32  RMON_T_P_GTE2048;   // 240
    UINT32  RMON_T_OCTETS;      // 244
    UINT32  IEEE_T_DROP_NI;     // 248
    UINT32  IEEE_T_FRAME_OK;    // 24C
    UINT32  IEEE_T_1COL;        // 250
    UINT32  IEEE_T_MCOL;        // 254
    UINT32  IEEE_T_DEF;         // 258
    UINT32  IEEE_T_LCOL;        // 25C
    UINT32  IEEE_T_EXCOL;       // 260
    UINT32  IEEE_T_MACERR;      // 264
    UINT32  IEEE_T_CSERR;       // 268
    UINT32  IEEE_T_SQE_NI;      // 26C
    UINT32  IEEE_T_FDXFC;       // 270
    UINT32  IEEE_T_OCTETS_OK;   // 274
    UINT32  ___RES_278[3];
    UINT32  RMON_R_PACKETS;     // 284
    UINT32  RMON_R_BC_PKT;      // 288
    UINT32  RMON_R_MC_PKT;      // 28C
    UINT32  RMON_R_CRC_ALIGN;   // 290
    UINT32  RMON_R_UNDERSIZE;   // 294
    UINT32  RMON_R_OVERSIZE;    // 298
    UINT32  RMON_R_FRAG;        // 29C
    UINT32  RMON_R_JAB;         // 2A0
    UINT32  RMON_R_RESVD_0;     // 2A4
    UINT32  RMON_R_P64;         // 2A8
    UINT32  RMON_R_P65TO127;    // 2AC
    UINT32  RMON_R_P128TO255;   // 2B0
    UINT32  RMON_R_P256TO511;   // 2B4
    UINT32  RMON_R_P512TO1023;  // 2B8
    UINT32  RMON_R_P1024TO2047; // 2BC
    UINT32  RMON_R_P_GTE2048;   // 2C0
    UINT32  RMON_R_OCTETS;      // 2C4
    UINT32  IEEE_R_DROP;        // 2C8
    UINT32  IEEE_R_FRAME_OK;    // 2CC
    UINT32  IEEE_R_CRC;         // 2D0
    UINT32  IEEE_R_ALIGN;       // 2D4
    UINT32  IEEE_R_MACERR;      // 2D8
    UINT32  IEEE_R_FDXFC;       // 2DC
    UINT32  IEEE_R_OCTETS_OK;   // 2E0
} CSP_ENET_REGS, *PCSP_ENET_REGS;

// the buffer descriptor structure for the ENET Legacy Buffer Descriptor
typedef struct  _ENET_BD {
    USHORT  DataLen;
    union {
        USHORT  ControlStatus;
        struct {
            USHORT  RX_TR       : 1;
            USHORT  RX_OV       : 1;
            USHORT  RX_CR       : 1;
            USHORT  RX_res_3    : 1;
            USHORT  RX_NO       : 1;
            USHORT  RX_LG       : 1;
            USHORT  RX_MC       : 1;
            USHORT  RX_BC       : 1;
            USHORT  RX_M        : 1;
            USHORT  RX_res_9_10 : 2;
            USHORT  RX_L        : 1;
            USHORT  RX_RO2      : 1;
            USHORT  RX_W        : 1;
            USHORT  RX_RO1      : 1;
            USHORT  RX_E        : 1;
        } ControlStatus_rx;
        struct {
            USHORT  TX_res_0_8 : 9;
            USHORT  TX_ABC     : 1;
            USHORT  TX_TC      : 1;
            USHORT  TX_L       : 1;
            USHORT  TX_TO2     : 1;
            USHORT  TX_W       : 1;
            USHORT  TX_TO1     : 1;
            USHORT  TX_R       : 1;
        } ControlStatus_tx;
    };
    ULONG BufferAddress;
} ENET_BD, *PENET_BD;

#define ENET_RX_BD_E_MASK            ((USHORT)0x8000)
#define ENET_RX_BD_W_MASK            ((USHORT)0x2000)
#define ENET_RX_BD_L_MASK            ((USHORT)0x0800)
#define ENET_RX_BD_M_MASK            ((USHORT)0x0100)
#define ENET_RX_BD_BC_MASK           ((USHORT)0x0080)
#define ENET_RX_BD_MC_MASK           ((USHORT)0x0040)
#define ENET_RX_BD_LG_MASK           ((USHORT)0x0020)
#define ENET_RX_BD_NO_MASK           ((USHORT)0x0010)
#define ENET_RX_BD_CR_MASK           ((USHORT)0x0004)
#define ENET_RX_BD_OV_MASK           ((USHORT)0x0002)
#define ENET_RX_BD_TR_MASK           ((USHORT)0x0001)

#define ENET_TX_BD_R_MASK            ((USHORT)0x8000)
#define ENET_TX_BD_W_MASK            ((USHORT)0x2000)
#define ENET_TX_BD_L_MASK            ((USHORT)0x0800)
#define ENET_TX_BD_TC_MASK           ((USHORT)0x0400)

#endif