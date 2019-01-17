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

#ifndef _MP_DBG_H
#define _MP_DBG_H

#ifdef DBG

#pragma warning(disable:4189)   // local variable is initialized but not used
#pragma warning(disable:4152)   // non standard extension, function/data ptr conversion in expression used

void Dbg_PrintOidDescription(_In_ PMP_ADAPTER pAdapter, _In_ PNDIS_OID_REQUEST NdisRequest, _In_ NTSTATUS Status);
void Dbg_DumpMACRegs(_In_ PMP_ADAPTER pAdapter);
void Dbg_DumpRxStats(_In_ PMP_ADAPTER pAdapter);
void Dbg_DumpTxStats(_In_ PMP_ADAPTER pAdapter);
void Dbg_DumpStatistics(_In_ PMP_ADAPTER pAdapter);
void Dbg_DumpFifoTrasholdsAndPauseFrameDuration(_In_ PMP_ADAPTER pAdapter);
void Dbg_PrintMDIODeviceInfo(_In_ MP_MDIO_DEVICE *pMDIODev, _In_ char* prefix);
void Dbg_PrintMDIOBusInfo(_In_ MP_MDIO_BUS *pMDIOBus, _In_ char* prefix);
void Dbg_PrintENETDeviceSettings(_In_ MP_ADAPTER *pENETDev);
const char* Dbg_GetMpStateMachineStateName(_In_ MP_STATE i);
const char* Dbg_GetMpMediaConnectStateName(_In_ NDIS_MEDIA_CONNECT_STATE i);
const char* Dbg_GetMpMediaDuplexStateName(_In_ NDIS_MEDIA_CONNECT_STATE i);
const char* Dbg_GetNdisStatusName(_In_ NDIS_STATUS i);
const char* Dbg_GetNdisOidName(_In_ NDIS_OID i);
const char* Dbg_GetNdisOidRequestTypeName(_In_ NDIS_REQUEST_TYPE i);
const char* Dbg_GetNdisPowerStateName(_In_ NDIS_DEVICE_POWER_STATE i);
const char* Dbg_GetEnetPhyInterfaceTypeName(_In_ MP_MDIO_PHY_INTERFACE_TYPE i);
const char* Dbg_GetACPIFunctionName(_In_ ULONG i);
const char* Dbg_GetMIICallbackName(_In_ void * FunctionAddress);

// DO NOT COMMENT OUT ERROR MACROS in debug build
#define DBG_DRV_PRINT_ERROR(_format_str_,...)                                             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_format_str_,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pDriver?pDriver->MDIODrv_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_DRV_PRINT_ERROR_WITH_STATUS(_str_)                                            DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_str_" Status %s\n",KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pDriver?pDriver->MDIODrv_DeviceName:"ENETx",__FUNCTION__,Dbg_GetNdisStatusName(Status))
#define DBG_DRV_PRINT_ERROR_WITH_STATUS_AND_PARAMS(_str_,_format_str_,...)                DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_str_" Status %s, "_format_str_"\n",KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pDriver?pDriver->MDIODrv_DeviceName:"ENETx",__FUNCTION__,Dbg_GetNdisStatusName(Status),__VA_ARGS__)
#define DBG_ENET_DEV_PRINT_ERROR(_format_str_,...)                                        DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_format_str_,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ENET_DEV_PRINT_ERROR_WITH_STATUS(_str_)                                       DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_str_" Status %s\n",KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,Dbg_GetNdisStatusName(Status))
#define DBG_ENET_DEV_PRINT_ERROR_WITH_STATUS_AND_PARAMS(_str_,_format_str_,...)           DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_str_" Status %s, "_format_str_"\n",KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,Dbg_GetNdisStatusName(Status),__VA_ARGS__)
#define DBG_ENET_DEV_ISR_PRINT_ERROR(_format_str_,...)                                    DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_format_str_,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ENET_DEV_ISR_PRINT_ERROR_WITH_STATUS(_str_)                                   DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_str_" Status %s\n",KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,Dbg_GetNdisStatusName(Status))
#define DBG_ENET_DEV_ISR_PRINT_ERROR_WITH_STATUS_AND_PARAMS(_str_,_format_str_,...)       DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_str_" Status %s, "_format_str_"\n",KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,Dbg_GetNdisStatusName(Status),__VA_ARGS__)
#define DBG_ENET_DEV_DPC_PRINT_ERROR(_format_str_,...)                                    DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_format_str_,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ENET_DEV_DPC_PRINT_ERROR_WITH_STATUS(_str_)                                   DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_str_" Status %s\n",KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,Dbg_GetNdisStatusName(Status))
#define DBG_ENET_DEV_DPC_PRINT_ERROR_WITH_STATUS_AND_PARAMS(_str_,_format_str_,...)       DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_str_" Status %s, "_format_str_"\n",KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,Dbg_GetNdisStatusName(Status),__VA_ARGS__)
#define DBG_ENET_DEV_RX_PRINT_ERROR(_format_str_,...)                                     DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_format_str_,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ENET_DEV_RX_PRINT_ERROR_WITH_STATUS(_str_)                                    DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_str_" Status %s\n",KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,Dbg_GetNdisStatusName(Status))
#define DBG_ENET_DEV_RX_PRINT_ERROR_WITH_STATUS_AND_PARAMS(_str_,_format_str_,...)        DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_str_" Status %s, "_format_str_"\n",KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,Dbg_GetNdisStatusName(Status),__VA_ARGS__)
#define DBG_ENET_DEV_TX_PRINT_ERROR(_format_str_,...)                                     DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_format_str_,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ENET_DEV_TX_PRINT_ERROR_WITH_STATUS(_str_)                                    DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_str_" Status %s\n",KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,Dbg_GetNdisStatusName(Status))
#define DBG_ENET_DEV_TX_PRINT_ERROR_WITH_STATUS_AND_PARAMS(_str_,_format_str_,...)        DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_str_" Status %s, "_format_str_"\n",KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,Dbg_GetNdisStatusName(Status),__VA_ARGS__)
#define DBG_ENET_DEV_DPC_RX_PRINT_ERROR(_format_str_,...)                                 DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_format_str_,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ENET_DEV_DPC_RX_PRINT_ERROR_WITH_STATUS(_str_)                                DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_str_" Status %s\n",KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,Dbg_GetNdisStatusName(Status))
#define DBG_ENET_DEV_DPC_RX_PRINT_ERROR_WITH_STATUS_AND_PARAMS(_str_,_format_str_,...)    DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_str_" Status %s, "_format_str_"\n",KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,Dbg_GetNdisStatusName(Status),__VA_ARGS__)
#define DBG_ENET_DEV_DPC_TX_PRINT_ERROR(_format_str_,...)                                 DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_format_str_,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ENET_DEV_DPC_TX_PRINT_ERROR_WITH_STATUS(_str_)                                DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_str_" Status %s\n",KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,Dbg_GetNdisStatusName(Status))
#define DBG_ENET_DEV_DPC_TX_PRINT_ERROR_WITH_STATUS_AND_PARAMS(_str_,_format_str_,...)    DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_str_" Status %s, "_format_str_"\n",KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,Dbg_GetNdisStatusName(Status),__VA_ARGS__)
#define DBG_ENET_DEV_OIDS_PRINT_ERROR(_format_str_,...)                                   DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_format_str_,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ENET_DEV_OIDS_PRINT_ERROR_WITH_STATUS(_str_)                                  DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_str_" Status %s\n",KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,Dbg_GetNdisStatusName(Status))
#define DBG_ENET_DEV_OIDS_PRINT_ERROR_WITH_STATUS_AND_PARAMS(_str_,_format_str_,...)      DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_str_" Status %s, "_format_str_"\n",KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,Dbg_GetNdisStatusName(Status),__VA_ARGS__)
#define DBG_PHY_DEV_PRINT_ERROR(_format_str_,...)                                         DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_format_str_,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pPHYDev?pPHYDev->PHYDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_PHY_DEV_PRINT_ERROR_WITH_STATUS(_str_)                                        DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_str_" Status %s\n",KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pPHYDev?pPHYDev->PHYDev_DeviceName:"ENETx",__FUNCTION__,Dbg_GetNdisStatusName(Status))
#define DBG_PHY_DEV_PRINT_ERROR_WITH_STATUS_AND_PARAMS(_str_,_format_str_,...)            DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_str_" Status %s, "_format_str_"\n",KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pPHYDev?pPHYDev->PHYDev_DeviceName:"ENETx",__FUNCTION__,Dbg_GetNdisStatusName(Status),__VA_ARGS__)
#define DBG_ACPI_PRINT_ERROR(_format_str_,...)                                            DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_format_str_,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ACPI_PRINT_ERROR_WITH_STATUS(_str_)                                           DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_str_" Status %s\n",KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,Dbg_GetNdisStatusName(Status))
#define DBG_ACPI_PRINT_ERROR_WITH_STATUS_AND_PARAMS(_str_,_format_str_,...)               DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_str_" Status %s, "_format_str_"\n",KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,Dbg_GetNdisStatusName(Status),__VA_ARGS__)
#define DBG_SM_PRINT_ERROR(_format_str_,...)                                              DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_format_str_,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_SM_PRINT_ERROR_WITH_STATUS(_str_)                                             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_str_" Status %s\n",KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,Dbg_GetNdisStatusName(Status))
#define DBG_SM_PRINT_ERROR_WITH_STATUS_AND_PARAMS(_str_,_format_str_,...)                 DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_str_" Status %s, "_format_str_"\n",KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,Dbg_GetNdisStatusName(Status),__VA_ARGS__)
#define DBG_MDIO_BUS_PRINT_ERROR(_format_str_,...)                                        DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_format_str_,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pMDIOBus?pMDIOBus->MDIOBus_DeviceName:"MDIOx",__FUNCTION__,__VA_ARGS__)
#define DBG_MDIO_BUS_PRINT_ERROR_WITH_STATUS(_str_)                                       DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_str_" Status %s\n",KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pMDIOBus?pMDIOBus->MDIOBus_DeviceName:"MDIOx",__FUNCTION__,Dbg_GetNdisStatusName(Status))
#define DBG_MDIO_BUS_PRINT_ERROR_WITH_STATUS_AND_PARAMS(_str_,_format_str_,...)           DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_str_" Status %s, "_format_str_"\n",KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pMDIOBus?pMDIOBus->MDIOBus_DeviceName:"MDIOx",__FUNCTION__,Dbg_GetNdisStatusName(Status),__VA_ARGS__)
#define DBG_MDIO_DEV_PRINT_ERROR(_format_str_,...)                                        DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_format_str_,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pMDIODev?pMDIODev->MDIODev_DeviceName:"MDIOx(ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_MDIO_DEV_PRINT_ERROR_WITH_STATUS(_str_)                                       DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_str_" Status %s\n",KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pMDIODev?pMDIODev->MDIODev_DeviceName:"MDIOx(ENETx",__FUNCTION__,Dbg_GetNdisStatusName(Status))
#define DBG_MDIO_DEV_PRINT_ERROR_WITH_STATUS_AND_PARAMS(_str_,_format_str_,...)           DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_str_" Status %s, "_format_str_"\n",KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pMDIODev?pMDIODev->MDIODev_DeviceName:"MDIOx(ENETx",__FUNCTION__,Dbg_GetNdisStatusName(Status),__VA_ARGS__)
#define DBG_MDIO_DEV_CMD_PRINT_ERROR(_format_str_,...)                                    DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_format_str_,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pMDIODev?pMDIODev->MDIODev_DeviceName:"MDIOx(ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_MDIO_DEV_CMD_PRINT_ERROR_WITH_STATUS(_str_)                                   DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_str_" Status %s\n",KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pMDIODev?pMDIODev->MDIODev_DeviceName:"MDIOx(ENETx",__FUNCTION__,Dbg_GetNdisStatusName(Status))
#define DBG_MDIO_DEV_CMD_PRINT_ERROR_WITH_STATUS_AND_PARAMS(_str_,_format_str_,...)       DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ERROR !!!"_str_" Status %s, "_format_str_"\n",KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pMDIODev?pMDIODev->MDIODev_DeviceName:"MDIOx(ENETx",__FUNCTION__,Dbg_GetNdisStatusName(Status),__VA_ARGS__)

// Uncomment next line for debug message printing
//#define DBG_MESSAGE_PRINTING
#ifdef DBG_MESSAGE_PRINTING

#define DBG_CODE(_line_) _line_

// Uncomment next line to print basic ENET HW configuration
#define DBG_PRINT_BASIC_ENET_CONFIGURATION(_format_str_,...)                              DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFF,_format_str_"\n",__VA_ARGS__)

// ENET driver specific macros - uncomment next line for message printing
//#define DBG_DRV
#ifdef DBG_DRV
#define DBG_DRV_CODE(_line_) _line_
#define DBG_DRV_METHOD_BEG()                             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s +++\n"                     ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pDriver?pDriver->MDIODrv_DeviceName:"ENETx",__FUNCTION__)
#define DBG_DRV_METHOD_BEG_WITH_PARAMS(_format_str_,...) DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s +++ "_format_str_"\n"      ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pDriver?pDriver->MDIODrv_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_DRV_METHOD_END()                             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ---\n"                     ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pDriver?pDriver->MDIODrv_DeviceName:"ENETx",__FUNCTION__)
#define DBG_DRV_METHOD_END_WITH_STATUS(_status_)         DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s --- [0x%.8X]\n"            ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pDriver?pDriver->MDIODrv_DeviceName:"ENETx",__FUNCTION__,_status_)
#define DBG_DRV_PRINT_WARNING(_format_str_,...)          DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s WARNING "_format_str_"\n"  ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pDriver?pDriver->MDIODrv_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_DRV_PRINT_TRACE(_format_str_,...)            DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s "_format_str_"\n"          ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pDriver?pDriver->MDIODrv_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_DRV_PRINT_INFO(_format_str_,...)             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s "_format_str_"\n"          ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pDriver?pDriver->MDIODrv_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#endif //DBG_DRV

// ENET device specific macros - uncomment next line for message printing
//#define DBG_ENET_DEV
#ifdef DBG_ENET_DEV
#define DBG_ENET_DEV_CODE(_line_) _line_
#define DBG_ENET_DEV_METHOD_BEG()                             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s +++\n"                     ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__)
#define DBG_ENET_DEV_METHOD_BEG_WITH_PARAMS(_format_str_,...) DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s +++ "_format_str_"\n"      ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ENET_DEV_METHOD_END()                             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ---\n"                     ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__)
#define DBG_ENET_DEV_METHOD_END_WITH_STATUS(_status_)         DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s --- [0x%.8X]\n"            ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,_status_)
#define DBG_ENET_DEV_PRINT_WARNING(_format_str_,...)          DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s WARNING "_format_str_"\n"  ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ENET_DEV_PRINT_TRACE(_format_str_,...)            DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s "_format_str_"\n"          ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ENET_DEV_PRINT_INFO(_format_str_,...)             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s "_format_str_"\n"          ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#endif //DBG_ENET_DEV

// ENET device ISR specific macros - uncomment next line for message printing
//#define DBG_ENET_DEV_ISR
#ifdef DBG_ENET_DEV_ISR
#define DBG_ENET_DEV_ISR_CODE(_line_) _line_
#define DBG_ENET_DEV_ISR_METHOD_BEG()                             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s +++\n"                     ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__)
#define DBG_ENET_DEV_ISR_METHOD_BEG_WITH_PARAMS(_format_str_,...) DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s +++ "_format_str_"\n"      ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ENET_DEV_ISR_METHOD_END()                             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ---\n"                     ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__)
#define DBG_ENET_DEV_ISR_METHOD_END_WITH_STATUS(_status_)         DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s --- [0x%.8X]\n"            ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,_status_)
#define DBG_ENET_DEV_ISR_PRINT_WARNING(_format_str_,...)          DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s WARNING "_format_str_"\n"  ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ENET_DEV_ISR_PRINT_TRACE(_format_str_,...)            DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s "_format_str_"\n"          ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ENET_DEV_ISR_PRINT_INFO(_format_str_,...)             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s "_format_str_"\n"          ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#endif //DBG_ENET_DEV_ISR

// ENET device DPC specific macros - uncomment next line for message printing
//#define DBG_ENET_DEV_DPC
#ifdef DBG_ENET_DEV_DPC
#define DBG_ENET_DEV_DPC_CODE(_line_) _line_
#define DBG_ENET_DEV_DPC_METHOD_BEG()                             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s +++\n"                     ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__)
#define DBG_ENET_DEV_DPC_METHOD_BEG_WITH_PARAMS(_format_str_,...) DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s +++ "_format_str_"\n"      ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ENET_DEV_DPC_METHOD_END()                             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ---\n"                     ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__)
#define DBG_ENET_DEV_DPC_METHOD_END_WITH_STATUS(_status_)         DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s --- [0x%.8X]\n"            ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,_status_)
#define DBG_ENET_DEV_DPC_PRINT_WARNING(_format_str_,...)          DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s WARNING "_format_str_"\n"  ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ENET_DEV_DPC_PRINT_TRACE(_format_str_,...)            DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s "_format_str_"\n"          ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ENET_DEV_DPC_PRINT_INFO(_format_str_,...)             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s "_format_str_"\n"          ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#endif //DBG_ENET_DEV_DPC

// ENET device RX specific macros - uncomment next line for message printing
//#define DBG_ENET_DEV_RX
#ifdef DBG_ENET_DEV_RX
#define DBG_ENET_DEV_RX_CODE(_line_) _line_
#define DBG_ENET_DEV_RX_METHOD_BEG()                             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s +++\n"                     ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__)
#define DBG_ENET_DEV_RX_METHOD_BEG_WITH_PARAMS(_format_str_,...) DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s +++ "_format_str_"\n"      ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ENET_DEV_RX_METHOD_END()                             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ---\n"                     ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__)
#define DBG_ENET_DEV_RX_METHOD_END_WITH_STATUS(_status_)         DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s --- [0x%.8X]\n"            ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,_status_)
#define DBG_ENET_DEV_RX_PRINT_WARNING(_format_str_,...)          DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s WARNING "_format_str_"\n"  ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ENET_DEV_RX_PRINT_TRACE(_format_str_,...)            DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s "_format_str_"\n"          ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ENET_DEV_RX_PRINT_INFO(_format_str_,...)             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s "_format_str_"\n"          ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#endif //DBG_ENET_DEV_RX

// ENET device TX specific macros - uncomment next line for message printing
//#define DBG_ENET_DEV_TX
#ifdef DBG_ENET_DEV_TX
#define DBG_ENET_DEV_TX_CODE(_line_) _line_
#define DBG_ENET_DEV_TX_METHOD_BEG()                             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s +++\n"                     ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__)
#define DBG_ENET_DEV_TX_METHOD_BEG_WITH_PARAMS(_format_str_,...) DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s +++ "_format_str_"\n"      ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ENET_DEV_TX_METHOD_END()                             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ---\n"                     ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__)
#define DBG_ENET_DEV_TX_METHOD_END_WITH_STATUS(_status_)         DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s --- [0x%.8X]\n"            ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,_status_)
#define DBG_ENET_DEV_TX_PRINT_WARNING(_format_str_,...)          DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s WARNING "_format_str_"\n"  ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ENET_DEV_TX_PRINT_TRACE(_format_str_,...)            DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s "_format_str_"\n"          ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ENET_DEV_TX_PRINT_INFO(_format_str_,...)             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s "_format_str_"\n"          ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#endif //DBG_ENET_DEV_TX

// ENET device DPC RX specific macros - uncomment next line for message printing
//#define DBG_ENET_DEV_DPC_RX
#ifdef DBG_ENET_DEV_DPC_RX
#define DBG_ENET_DEV_DPC_RX_CODE(_line_) _line_
#define DBG_ENET_DEV_DPC_RX_METHOD_BEG()                             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s +++\n"                     ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__)
#define DBG_ENET_DEV_DPC_RX_METHOD_BEG_WITH_PARAMS(_format_str_,...) DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s +++ "_format_str_"\n"      ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ENET_DEV_DPC_RX_METHOD_END()                             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ---\n"                     ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__)
#define DBG_ENET_DEV_DPC_RX_METHOD_END_WITH_STATUS(_status_)         DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s --- [0x%.8X]\n"            ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,_status_)
#define DBG_ENET_DEV_DPC_RX_PRINT_WARNING(_format_str_,...)          DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s WARNING "_format_str_"\n"  ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ENET_DEV_DPC_RX_PRINT_TRACE(_format_str_,...)            DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s "_format_str_"\n"          ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ENET_DEV_DPC_RX_PRINT_INFO(_format_str_,...)             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s "_format_str_"\n"          ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#endif //DBG_ENET_DEV_DPC_RX

// ENET device DPC TX specific macros - uncomment next line for message printing
//#define DBG_ENET_DEV_DPC_TX
#ifdef DBG_ENET_DEV_DPC_TX
#define DBG_ENET_DEV_DPC_TX_CODE(_line_) _line_
#define DBG_ENET_DEV_DPC_TX_METHOD_BEG()                             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s +++\n"                     ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__)
#define DBG_ENET_DEV_DPC_TX_METHOD_BEG_WITH_PARAMS(_format_str_,...) DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s +++ "_format_str_"\n"      ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ENET_DEV_DPC_TX_METHOD_END()                             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ---\n"                     ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__)
#define DBG_ENET_DEV_DPC_TX_METHOD_END_WITH_STATUS(_status_)         DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s --- [0x%.8X]\n"            ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,_status_)
#define DBG_ENET_DEV_DPC_TX_PRINT_WARNING(_format_str_,...)          DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s WARNING "_format_str_"\n"  ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ENET_DEV_DPC_TX_PRINT_TRACE(_format_str_,...)            DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s "_format_str_"\n"          ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ENET_DEV_DPC_TX_PRINT_INFO(_format_str_,...)             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s "_format_str_"\n"          ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#endif //DBG_ENET_DEV_DPC_TX

// ENET device OIDS specific macros - uncomment next line for message printing
//#define DBG_ENET_DEV_OIDS
#ifdef DBG_ENET_DEV_OIDS
#define DBG_ENET_DEV_OIDS_CODE(_line_) _line_
#define DBG_ENET_DEV_OIDS_METHOD_BEG()                             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s +++\n"                     ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__)
#define DBG_ENET_DEV_OIDS_METHOD_BEG_WITH_PARAMS(_format_str_,...) DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s +++ "_format_str_"\n"      ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ENET_DEV_OIDS_METHOD_END()                             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ---\n"                     ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__)
#define DBG_ENET_DEV_OIDS_METHOD_END_WITH_STATUS(_status_)         DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s --- [0x%.8X]\n"            ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,_status_)
#define DBG_ENET_DEV_OIDS_PRINT_WARNING(_format_str_,...)          DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s WARNING "_format_str_"\n"  ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ENET_DEV_OIDS_PRINT_TRACE(_format_str_,...)            DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s "_format_str_"\n"          ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ENET_DEV_OIDS_PRINT_INFO(_format_str_,...)             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s "_format_str_"\n"          ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#endif //DBG_ENET_DEV_OIDS

// ENET PHY device specific macros - uncomment next line for message printing
//#define DBG_PHY_DEV
#ifdef DBG_PHY_DEV
#define DBG_PHY_DEV_CODE(_line_) _line_
#define DBG_PHY_DEV_METHOD_BEG()                             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s +++\n"                     ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pPHYDev?pPHYDev->PHYDev_DeviceName:"ENETx",__FUNCTION__)
#define DBG_PHY_DEV_METHOD_BEG_WITH_PARAMS(_format_str_,...) DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s +++ "_format_str_"\n"      ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pPHYDev?pPHYDev->PHYDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_PHY_DEV_METHOD_END()                             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ---\n"                     ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pPHYDev?pPHYDev->PHYDev_DeviceName:"ENETx",__FUNCTION__)
#define DBG_PHY_DEV_METHOD_END_WITH_STATUS(_status_)         DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s --- [0x%.8X]\n"            ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pPHYDev?pPHYDev->PHYDev_DeviceName:"ENETx",__FUNCTION__,_status_)
#define DBG_PHY_DEV_PRINT_WARNING(_format_str_,...)          DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s WARNING "_format_str_"\n"  ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pPHYDev?pPHYDev->PHYDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_PHY_DEV_PRINT_TRACE(_format_str_,...)            DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s "_format_str_"\n"          ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pPHYDev?pPHYDev->PHYDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_PHY_DEV_PRINT_INFO(_format_str_,...)             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s "_format_str_"\n"          ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pPHYDev?pPHYDev->PHYDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#endif //DBG_PHY_DEV

// ENET device ACPI specific macros - uncomment next line for message printing
//#define DBG_ACPI
#ifdef DBG_ACPI
#define DBG_ACPI_CODE(_line_) _line_
#define DBG_ACPI_METHOD_BEG()                             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s +++\n"                     ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__)
#define DBG_ACPI_METHOD_BEG_WITH_PARAMS(_format_str_,...) DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s +++ "_format_str_"\n"      ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ACPI_METHOD_END()                             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ---\n"                     ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__)
#define DBG_ACPI_METHOD_END_WITH_STATUS(_status_)         DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s --- [0x%.8X]\n"            ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,_status_)
#define DBG_ACPI_PRINT_WARNING(_format_str_,...)          DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s WARNING "_format_str_"\n"  ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ACPI_PRINT_TRACE(_format_str_,...)            DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s "_format_str_"\n"          ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_ACPI_PRINT_INFO(_format_str_,...)             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s "_format_str_"\n"          ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#endif //DBG_ACPI

// ENET device state machine specific macros - uncomment next line for message printing
//#define DBG_SM
#ifdef DBG_SM
#define DBG_SM_CODE(_line_) _line_
#define DBG_SM_METHOD_BEG()                             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s +++\n"                     ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__)
#define DBG_SM_METHOD_BEG_WITH_PARAMS(_format_str_,...) DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s +++ "_format_str_"\n"      ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_SM_METHOD_END()                             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ---\n"                     ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__)
#define DBG_SM_METHOD_END_WITH_STATUS(_status_)         DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s --- [0x%.8X]\n"            ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,_status_)
#define DBG_SM_PRINT_WARNING(_format_str_,...)          DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s WARNING "_format_str_"\n"  ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_SM_PRINT_TRACE(_format_str_,...)            DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s "_format_str_"\n"          ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_SM_PRINT_INFO(_format_str_,...)             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s "_format_str_"\n"          ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pAdapter?pAdapter->ENETDev_DeviceName:"ENETx",__FUNCTION__,__VA_ARGS__)
#endif //DBG_SM

// MDIO bus specific macros - uncomment next line for message printing
//#define DBG_MDIO_BUS
#ifdef DBG_MDIO_BUS
#define DBG_MDIO_BUS_CODE(_line_) _line_
#define DBG_MDIO_BUS_METHOD_BEG()                             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s +++\n"                     ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pMDIOBus?pMDIOBus->MDIOBus_DeviceName:"MDIOx",__FUNCTION__)
#define DBG_MDIO_BUS_METHOD_BEG_WITH_PARAMS(_format_str_,...) DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s +++ "_format_str_"\n"      ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pMDIOBus?pMDIOBus->MDIOBus_DeviceName:"MDIOx",__FUNCTION__,__VA_ARGS__)
#define DBG_MDIO_BUS_METHOD_END()                             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ---\n"                     ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pMDIOBus?pMDIOBus->MDIOBus_DeviceName:"MDIOx",__FUNCTION__)
#define DBG_MDIO_BUS_METHOD_END_WITH_STATUS(_status_)         DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s --- [0x%.8X]\n"            ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pMDIOBus?pMDIOBus->MDIOBus_DeviceName:"MDIOx",__FUNCTION__,_status_)
#define DBG_MDIO_BUS_PRINT_WARNING(_format_str_,...)          DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s WARNING "_format_str_"\n"  ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pMDIOBus?pMDIOBus->MDIOBus_DeviceName:"MDIOx",__FUNCTION__,__VA_ARGS__)
#define DBG_MDIO_BUS_PRINT_TRACE(_format_str_,...)            DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s "_format_str_"\n"          ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pMDIOBus?pMDIOBus->MDIOBus_DeviceName:"MDIOx",__FUNCTION__,__VA_ARGS__)
#define DBG_MDIO_BUS_PRINT_INFO(_format_str_,...)             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s "_format_str_"\n"          ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pMDIOBus?pMDIOBus->MDIOBus_DeviceName:"MDIOx",__FUNCTION__,__VA_ARGS__)
#endif //DBG_MDIO_BUS

// MDIO device specific macros - uncomment next line for message printing
//#define DBG_MDIO_DEV
#ifdef DBG_MDIO_DEV
#define DBG_MDIO_DEV_CODE(_line_) _line_
#define DBG_MDIO_DEV_METHOD_BEG()                             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s +++\n"                     ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pMDIODev?pMDIODev->MDIODev_DeviceName:"MDIOx(ENETx",__FUNCTION__)
#define DBG_MDIO_DEV_METHOD_BEG_WITH_PARAMS(_format_str_,...) DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s +++ "_format_str_"\n"      ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pMDIODev?pMDIODev->MDIODev_DeviceName:"MDIOx(ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_MDIO_DEV_METHOD_END()                             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ---\n"                     ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pMDIODev?pMDIODev->MDIODev_DeviceName:"MDIOx(ENETx",__FUNCTION__)
#define DBG_MDIO_DEV_METHOD_END_WITH_STATUS(_status_)         DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s --- [0x%.8X]\n"            ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pMDIODev?pMDIODev->MDIODev_DeviceName:"MDIOx(ENETx",__FUNCTION__,_status_)
#define DBG_MDIO_DEV_PRINT_WARNING(_format_str_,...)          DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s WARNING "_format_str_"\n"  ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pMDIODev?pMDIODev->MDIODev_DeviceName:"MDIOx(ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_MDIO_DEV_PRINT_TRACE(_format_str_,...)            DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s "_format_str_"\n"          ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pMDIODev?pMDIODev->MDIODev_DeviceName:"MDIOx(ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_MDIO_DEV_PRINT_INFO(_format_str_,...)             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s "_format_str_"\n"          ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pMDIODev?pMDIODev->MDIODev_DeviceName:"MDIOx(ENETx",__FUNCTION__,__VA_ARGS__)
#endif //DBG_MDIO_DEV

// MDIO device command specific macros - uncomment next line for message printing
//#define DBG_MDIO_DEV_CMD
#ifdef DBG_MDIO_DEV_CMD
#define DBG_MDIO_DEV_CMD_CODE(_line_) _line_
#define DBG_MDIO_DEV_CMD_METHOD_BEG()                             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s +++\n"                     ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pMDIODev?pMDIODev->MDIODev_DeviceName:"MDIOx(ENETx",__FUNCTION__)
#define DBG_MDIO_DEV_CMD_METHOD_BEG_WITH_PARAMS(_format_str_,...) DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s +++ "_format_str_"\n"      ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pMDIODev?pMDIODev->MDIODev_DeviceName:"MDIOx(ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_MDIO_DEV_CMD_METHOD_END()                             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s ---\n"                     ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pMDIODev?pMDIODev->MDIODev_DeviceName:"MDIOx(ENETx",__FUNCTION__)
#define DBG_MDIO_DEV_CMD_METHOD_END_WITH_STATUS(_status_)         DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s --- [0x%.8X]\n"            ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pMDIODev?pMDIODev->MDIODev_DeviceName:"MDIOx(ENETx",__FUNCTION__,_status_)
#define DBG_MDIO_DEV_CMD_PRINT_WARNING(_format_str_,...)          DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s WARNING "_format_str_"\n"  ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pMDIODev?pMDIODev->MDIODev_DeviceName:"MDIOx(ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_MDIO_DEV_CMD_PRINT_TRACE(_format_str_,...)            DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s "_format_str_"\n"          ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pMDIODev?pMDIODev->MDIODev_DeviceName:"MDIOx(ENETx",__FUNCTION__,__VA_ARGS__)
#define DBG_MDIO_DEV_CMD_PRINT_INFO(_format_str_,...)             DbgPrintEx(DPFLTR_IHVDRIVER_ID,0xFFFFFFFE,"C%d D%d %s:%s "_format_str_"\n"          ,KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),pMDIODev?pMDIODev->MDIODev_DeviceName:"MDIOx(ENETx",__FUNCTION__,__VA_ARGS__)
#endif //DBG_MDIO_DEV_CMD

#endif // DBG_MESSAGE_PRINTING

#else // DBG

#define Dbg_PrintOidDescription(...)
#define Dbg_DumpMACRegs(...)
#define Dbg_DumpRxStats(...)
#define Dbg_DumpTxStats(...)
#define Dbg_DumpStatistics(...)
#define Dbg_DumpFifoTrasholdsAndPauseFrameDuration(...)
#define Dbg_PrintMDIODeviceInfo(...)
#define Dbg_PrintMDIOBusInfo(...)
#define Dbg_PrintENETDeviceSettings(...)
#define Dbg_GetMpStateMachineStateName(...)
#define Dbg_GetMpMediaConnectStateName(...)
#define Dbg_GetMpMediaDuplexStateName(...)
#define Dbg_GetNdisStatusName(...)
#define Dbg_GetNdisOidName(...)
#define Dbg_GetNdisOidRequestTypeName(...)
#define Dbg_GetNdisPowerStateName(...)
#define Dbg_GetEnetPhyInterfaceTypeName(...)
#define Dbg_GetACPIFunctionName(...)
#define Dbg_GetMIICallbackName(...)

#endif // DBG

#ifndef DBG_CODE
#define DBG_CODE(...)
#endif
#ifndef  DBG_PRINT_BASIC_ENET_CONFIGURATION
#define DBG_PRINT_BASIC_ENET_CONFIGURATION(...)
#endif

#ifndef DBG_DRV_CODE
#define DBG_DRV_CODE(...)
#endif
#ifndef DBG_DRV_METHOD_BEG
#define DBG_DRV_METHOD_BEG(...)
#endif
#ifndef DBG_DRV_METHOD_BEG_WITH_PARAMS
#define DBG_DRV_METHOD_BEG_WITH_PARAMS(...)
#endif
#ifndef DBG_DRV_METHOD_END
#define DBG_DRV_METHOD_END(...)
#endif
#ifndef DBG_DRV_METHOD_END_WITH_STATUS
#define DBG_DRV_METHOD_END_WITH_STATUS(...)
#endif
#ifndef DBG_DRV_PRINT_ERROR
#define DBG_DRV_PRINT_ERROR(...)
#endif
#ifndef DBG_DRV_PRINT_ERROR_WITH_STATUS
#define DBG_DRV_PRINT_ERROR_WITH_STATUS(...)
#endif
#ifndef DBG_DRV_PRINT_ERROR_WITH_STATUS_AND_PARAMS
#define DBG_DRV_PRINT_ERROR_WITH_STATUS_AND_PARAMS(...)
#endif
#ifndef DBG_DRV_PRINT_WARNING
#define DBG_DRV_PRINT_WARNING(...)
#endif
#ifndef DBG_DRV_PRINT_TRACE
#define DBG_DRV_PRINT_TRACE(...)
#endif
#ifndef DBG_DRV_PRINT_INFO
#define DBG_DRV_PRINT_INFO(...)
#endif

#ifndef DBG_ENET_DEV_CODE
#define DBG_ENET_DEV_CODE(...)
#endif
#ifndef DBG_ENET_DEV_METHOD_BEG
#define DBG_ENET_DEV_METHOD_BEG(...)
#endif
#ifndef DBG_ENET_DEV_METHOD_BEG_WITH_PARAMS
#define DBG_ENET_DEV_METHOD_BEG_WITH_PARAMS(...)
#endif
#ifndef DBG_ENET_DEV_METHOD_END
#define DBG_ENET_DEV_METHOD_END(...)
#endif
#ifndef DBG_ENET_DEV_METHOD_END_WITH_STATUS
#define DBG_ENET_DEV_METHOD_END_WITH_STATUS(...)
#endif
#ifndef DBG_ENET_DEV_PRINT_ERROR
#define DBG_ENET_DEV_PRINT_ERROR(...)
#endif
#ifndef DBG_ENET_DEV_PRINT_ERROR_WITH_STATUS
#define DBG_ENET_DEV_PRINT_ERROR_WITH_STATUS(...)
#endif
#ifndef DBG_ENET_DEV_PRINT_ERROR_WITH_STATUS_AND_PARAMS
#define DBG_ENET_DEV_PRINT_ERROR_WITH_STATUS_AND_PARAMS(...)
#endif
#ifndef DBG_ENET_DEV_PRINT_WARNING
#define DBG_ENET_DEV_PRINT_WARNING(...)
#endif
#ifndef DBG_ENET_DEV_PRINT_TRACE
#define DBG_ENET_DEV_PRINT_TRACE(...)
#endif
#ifndef DBG_ENET_DEV_PRINT_INFO
#define DBG_ENET_DEV_PRINT_INFO(...)
#endif

#ifndef DBG_ENET_DEV_ISR_CODE
#define DBG_ENET_DEV_ISR_CODE(...)
#endif
#ifndef DBG_ENET_DEV_ISR_METHOD_BEG
#define DBG_ENET_DEV_ISR_METHOD_BEG(...)
#endif
#ifndef DBG_ENET_DEV_ISR_METHOD_BEG_WITH_PARAMS
#define DBG_ENET_DEV_ISR_METHOD_BEG_WITH_PARAMS(...)
#endif
#ifndef DBG_ENET_DEV_ISR_METHOD_END
#define DBG_ENET_DEV_ISR_METHOD_END(...)
#endif
#ifndef DBG_ENET_DEV_ISR_METHOD_END_WITH_STATUS
#define DBG_ENET_DEV_ISR_METHOD_END_WITH_STATUS(...)
#endif
#ifndef DBG_ENET_DEV_ISR_PRINT_ERROR
#define DBG_ENET_DEV_ISR_PRINT_ERROR(...)
#endif
#ifndef DBG_ENET_DEV_ISR_PRINT_ERROR_WITH_STATUS
#define DBG_ENET_DEV_ISR_PRINT_ERROR_WITH_STATUS(...)
#endif
#ifndef DBG_ENET_DEV_ISR_PRINT_ERROR_WITH_STATUS_AND_PARAMS
#define DBG_ENET_DEV_ISR_PRINT_ERROR_WITH_STATUS_AND_PARAMS(...)
#endif
#ifndef DBG_ENET_DEV_ISR_PRINT_WARNING
#define DBG_ENET_DEV_ISR_PRINT_WARNING(...)
#endif
#ifndef DBG_ENET_DEV_ISR_PRINT_TRACE
#define DBG_ENET_DEV_ISR_PRINT_TRACE(...)
#endif
#ifndef DBG_ENET_DEV_ISR_PRINT_INFO
#define DBG_ENET_DEV_ISR_PRINT_INFO(...)
#endif

#ifndef DBG_ENET_DEV_DPC_CODE
#define DBG_ENET_DEV_DPC_CODE(...)
#endif
#ifndef DBG_ENET_DEV_DPC_METHOD_BEG
#define DBG_ENET_DEV_DPC_METHOD_BEG(...)
#endif
#ifndef DBG_ENET_DEV_DPC_METHOD_BEG_WITH_PARAMS
#define DBG_ENET_DEV_DPC_METHOD_BEG_WITH_PARAMS(...)
#endif
#ifndef DBG_ENET_DEV_DPC_METHOD_END
#define DBG_ENET_DEV_DPC_METHOD_END(...)
#endif
#ifndef DBG_ENET_DEV_DPC_METHOD_END_WITH_STATUS
#define DBG_ENET_DEV_DPC_METHOD_END_WITH_STATUS(...)
#endif
#ifndef DBG_ENET_DEV_DPC_PRINT_ERROR
#define DBG_ENET_DEV_DPC_PRINT_ERROR(...)
#endif
#ifndef DBG_ENET_DEV_DPC_PRINT_ERROR_WITH_STATUS
#define DBG_ENET_DEV_DPC_PRINT_ERROR_WITH_STATUS(...)
#endif
#ifndef DBG_ENET_DEV_DPC_PRINT_ERROR_WITH_STATUS_AND_PARAMS
#define DBG_ENET_DEV_DPC_PRINT_ERROR_WITH_STATUS_AND_PARAMS(...)
#endif
#ifndef DBG_ENET_DEV_DPC_PRINT_WARNING
#define DBG_ENET_DEV_DPC_PRINT_WARNING(...)
#endif
#ifndef DBG_ENET_DEV_DPC_PRINT_TRACE
#define DBG_ENET_DEV_DPC_PRINT_TRACE(...)
#endif
#ifndef DBG_ENET_DEV_DPC_PRINT_INFO
#define DBG_ENET_DEV_DPC_PRINT_INFO(...)
#endif

#ifndef DBG_ENET_DEV_RX_CODE
#define DBG_ENET_DEV_RX_CODE(...)
#endif
#ifndef DBG_ENET_DEV_RX_METHOD_BEG
#define DBG_ENET_DEV_RX_METHOD_BEG(...)
#endif
#ifndef DBG_ENET_DEV_RX_METHOD_BEG_WITH_PARAMS
#define DBG_ENET_DEV_RX_METHOD_BEG_WITH_PARAMS(...)
#endif
#ifndef DBG_ENET_DEV_RX_METHOD_END
#define DBG_ENET_DEV_RX_METHOD_END(...)
#endif
#ifndef DBG_ENET_DEV_RX_METHOD_END_WITH_STATUS
#define DBG_ENET_DEV_RX_METHOD_END_WITH_STATUS(...)
#endif
#ifndef DBG_ENET_DEV_RX_PRINT_ERROR
#define DBG_ENET_DEV_RX_PRINT_ERROR(...)
#endif
#ifndef DBG_ENET_DEV_RX_PRINT_ERROR_WITH_STATUS
#define DBG_ENET_DEV_RX_PRINT_ERROR_WITH_STATUS(...)
#endif
#ifndef DBG_ENET_DEV_RX_PRINT_ERROR_WITH_STATUS_AND_PARAMS
#define DBG_ENET_DEV_RX_PRINT_ERROR_WITH_STATUS_AND_PARAMS(...)
#endif
#ifndef DBG_ENET_DEV_RX_PRINT_WARNING
#define DBG_ENET_DEV_RX_PRINT_WARNING(...)
#endif
#ifndef DBG_ENET_DEV_RX_PRINT_TRACE
#define DBG_ENET_DEV_RX_PRINT_TRACE(...)
#endif
#ifndef DBG_ENET_DEV_RX_PRINT_INFO
#define DBG_ENET_DEV_RX_PRINT_INFO(...)
#endif

#ifndef DBG_ENET_DEV_TX_CODE
#define DBG_ENET_DEV_TX_CODE(...)
#endif
#ifndef DBG_ENET_DEV_TX_METHOD_BEG
#define DBG_ENET_DEV_TX_METHOD_BEG(...)
#endif
#ifndef DBG_ENET_DEV_TX_METHOD_BEG_WITH_PARAMS
#define DBG_ENET_DEV_TX_METHOD_BEG_WITH_PARAMS(...)
#endif
#ifndef DBG_ENET_DEV_TX_METHOD_END
#define DBG_ENET_DEV_TX_METHOD_END(...)
#endif
#ifndef DBG_ENET_DEV_TX_METHOD_END_WITH_STATUS
#define DBG_ENET_DEV_TX_METHOD_END_WITH_STATUS(...)
#endif
#ifndef DBG_ENET_DEV_TX_PRINT_ERROR
#define DBG_ENET_DEV_TX_PRINT_ERROR(...)
#endif
#ifndef DBG_ENET_DEV_TX_PRINT_ERROR_WITH_STATUS
#define DBG_ENET_DEV_TX_PRINT_ERROR_WITH_STATUS(...)
#endif
#ifndef DBG_ENET_DEV_TX_PRINT_ERROR_WITH_STATUS_AND_PARAMS
#define DBG_ENET_DEV_TX_PRINT_ERROR_WITH_STATUS_AND_PARAMS(...)
#endif
#ifndef DBG_ENET_DEV_TX_PRINT_WARNING
#define DBG_ENET_DEV_TX_PRINT_WARNING(...)
#endif
#ifndef DBG_ENET_DEV_TX_PRINT_TRACE
#define DBG_ENET_DEV_TX_PRINT_TRACE(...)
#endif
#ifndef DBG_ENET_DEV_TX_PRINT_INFO
#define DBG_ENET_DEV_TX_PRINT_INFO(...)
#endif

#ifndef DBG_ENET_DEV_DPC_RX_CODE
#define DBG_ENET_DEV_DPC_RX_CODE(...)
#endif
#ifndef DBG_ENET_DEV_DPC_RX_METHOD_BEG
#define DBG_ENET_DEV_DPC_RX_METHOD_BEG(...)
#endif
#ifndef DBG_ENET_DEV_DPC_RX_METHOD_BEG_WITH_PARAMS
#define DBG_ENET_DEV_DPC_RX_METHOD_BEG_WITH_PARAMS(...)
#endif
#ifndef DBG_ENET_DEV_DPC_RX_METHOD_END
#define DBG_ENET_DEV_DPC_RX_METHOD_END(...)
#endif
#ifndef DBG_ENET_DEV_DPC_RX_METHOD_END_WITH_STATUS
#define DBG_ENET_DEV_DPC_RX_METHOD_END_WITH_STATUS(...)
#endif
#ifndef DBG_ENET_DEV_DPC_RX_PRINT_ERROR
#define DBG_ENET_DEV_DPC_RX_PRINT_ERROR(...)
#endif
#ifndef DBG_ENET_DEV_DPC_RX_PRINT_ERROR_WITH_STATUS
#define DBG_ENET_DEV_DPC_RX_PRINT_ERROR_WITH_STATUS(...)
#endif
#ifndef DBG_ENET_DEV_DPC_RX_PRINT_ERROR_WITH_STATUS_AND_PARAMS
#define DBG_ENET_DEV_DPC_RX_PRINT_ERROR_WITH_STATUS_AND_PARAMS(...)
#endif
#ifndef DBG_ENET_DEV_DPC_RX_PRINT_WARNING
#define DBG_ENET_DEV_DPC_RX_PRINT_WARNING(...)
#endif
#ifndef DBG_ENET_DEV_DPC_RX_PRINT_TRACE
#define DBG_ENET_DEV_DPC_RX_PRINT_TRACE(...)
#endif
#ifndef DBG_ENET_DEV_DPC_RX_PRINT_INFO
#define DBG_ENET_DEV_DPC_RX_PRINT_INFO(...)
#endif

#ifndef DBG_ENET_DEV_DPC_TX_CODE
#define DBG_ENET_DEV_DPC_TX_CODE(...)
#endif
#ifndef DBG_ENET_DEV_DPC_TX_METHOD_BEG
#define DBG_ENET_DEV_DPC_TX_METHOD_BEG(...)
#endif
#ifndef DBG_ENET_DEV_DPC_TX_METHOD_BEG_WITH_PARAMS
#define DBG_ENET_DEV_DPC_TX_METHOD_BEG_WITH_PARAMS(...)
#endif
#ifndef DBG_ENET_DEV_DPC_TX_METHOD_END
#define DBG_ENET_DEV_DPC_TX_METHOD_END(...)
#endif
#ifndef DBG_ENET_DEV_DPC_TX_METHOD_END_WITH_STATUS
#define DBG_ENET_DEV_DPC_TX_METHOD_END_WITH_STATUS(...)
#endif
#ifndef DBG_ENET_DEV_DPC_TX_PRINT_ERROR
#define DBG_ENET_DEV_DPC_TX_PRINT_ERROR(...)
#endif
#ifndef DBG_ENET_DEV_DPC_TX_PRINT_ERROR_WITH_STATUS
#define DBG_ENET_DEV_DPC_TX_PRINT_ERROR_WITH_STATUS(...)
#endif
#ifndef DBG_ENET_DEV_DPC_TX_PRINT_ERROR_WITH_STATUS_AND_PARAMS
#define DBG_ENET_DEV_DPC_TX_PRINT_ERROR_WITH_STATUS_AND_PARAMS(...)
#endif
#ifndef DBG_ENET_DEV_DPC_TX_PRINT_WARNING
#define DBG_ENET_DEV_DPC_TX_PRINT_WARNING(...)
#endif
#ifndef DBG_ENET_DEV_DPC_TX_PRINT_TRACE
#define DBG_ENET_DEV_DPC_TX_PRINT_TRACE(...)
#endif
#ifndef DBG_ENET_DEV_DPC_TX_PRINT_INFO
#define DBG_ENET_DEV_DPC_TX_PRINT_INFO(...)
#endif

#ifndef DBG_ENET_DEV_OIDS_CODE
#define DBG_ENET_DEV_OIDS_CODE(...)
#endif
#ifndef DBG_ENET_DEV_OIDS_METHOD_BEG
#define DBG_ENET_DEV_OIDS_METHOD_BEG(...)
#endif
#ifndef DBG_ENET_DEV_OIDS_METHOD_BEG_WITH_PARAMS
#define DBG_ENET_DEV_OIDS_METHOD_BEG_WITH_PARAMS(...)
#endif
#ifndef DBG_ENET_DEV_OIDS_METHOD_END
#define DBG_ENET_DEV_OIDS_METHOD_END(...)
#endif
#ifndef DBG_ENET_DEV_OIDS_METHOD_END_WITH_STATUS
#define DBG_ENET_DEV_OIDS_METHOD_END_WITH_STATUS(...)
#endif
#ifndef DBG_ENET_DEV_OIDS_PRINT_ERROR
#define DBG_ENET_DEV_OIDS_PRINT_ERROR(...)
#endif
#ifndef DBG_ENET_DEV_OIDS_PRINT_ERROR_WITH_STATUS
#define DBG_ENET_DEV_OIDS_PRINT_ERROR_WITH_STATUS(...)
#endif
#ifndef DBG_ENET_DEV_OIDS_PRINT_ERROR_WITH_STATUS_AND_PARAMS
#define DBG_ENET_DEV_OIDS_PRINT_ERROR_WITH_STATUS_AND_PARAMS(...)
#endif
#ifndef DBG_ENET_DEV_OIDS_PRINT_WARNING
#define DBG_ENET_DEV_OIDS_PRINT_WARNING(...)
#endif
#ifndef DBG_ENET_DEV_OIDS_PRINT_TRACE
#define DBG_ENET_DEV_OIDS_PRINT_TRACE(...)
#endif
#ifndef DBG_ENET_DEV_OIDS_PRINT_INFO
#define DBG_ENET_DEV_OIDS_PRINT_INFO(...)
#endif

#ifndef DBG_PHY_DEV_CODE
#define DBG_PHY_DEV_CODE(...)
#endif
#ifndef DBG_PHY_DEV_METHOD_BEG
#define DBG_PHY_DEV_METHOD_BEG(...)
#endif
#ifndef DBG_PHY_DEV_METHOD_BEG_WITH_PARAMS
#define DBG_PHY_DEV_METHOD_BEG_WITH_PARAMS(...)
#endif
#ifndef DBG_PHY_DEV_METHOD_END
#define DBG_PHY_DEV_METHOD_END(...)
#endif
#ifndef DBG_PHY_DEV_METHOD_END_WITH_STATUS
#define DBG_PHY_DEV_METHOD_END_WITH_STATUS(...)
#endif
#ifndef DBG_PHY_DEV_PRINT_ERROR
#define DBG_PHY_DEV_PRINT_ERROR(...)
#endif
#ifndef DBG_PHY_DEV_PRINT_ERROR_WITH_STATUS
#define DBG_PHY_DEV_PRINT_ERROR_WITH_STATUS(...)
#endif
#ifndef DBG_PHY_DEV_PRINT_ERROR_WITH_STATUS_AND_PARAMS
#define DBG_PHY_DEV_PRINT_ERROR_WITH_STATUS_AND_PARAMS(...)
#endif
#ifndef DBG_PHY_DEV_PRINT_WARNING
#define DBG_PHY_DEV_PRINT_WARNING(...)
#endif
#ifndef DBG_PHY_DEV_PRINT_TRACE
#define DBG_PHY_DEV_PRINT_TRACE(...)
#endif
#ifndef DBG_PHY_DEV_PRINT_INFO
#define DBG_PHY_DEV_PRINT_INFO(...)
#endif

#ifndef DBG_ACPI_CODE
#define DBG_ACPI_CODE(...)
#endif
#ifndef DBG_ACPI_METHOD_BEG
#define DBG_ACPI_METHOD_BEG(...)
#endif
#ifndef DBG_ACPI_METHOD_BEG_WITH_PARAMS
#define DBG_ACPI_METHOD_BEG_WITH_PARAMS(...)
#endif
#ifndef DBG_ACPI_METHOD_END
#define DBG_ACPI_METHOD_END(...)
#endif
#ifndef DBG_ACPI_METHOD_END_WITH_STATUS
#define DBG_ACPI_METHOD_END_WITH_STATUS(...)
#endif
#ifndef DBG_ACPI_PRINT_ERROR
#define DBG_ACPI_PRINT_ERROR(...)
#endif
#ifndef DBG_ACPI_PRINT_ERROR_WITH_STATUS
#define DBG_ACPI_PRINT_ERROR_WITH_STATUS(...)
#endif
#ifndef DBG_ACPI_PRINT_ERROR_WITH_STATUS_AND_PARAMS
#define DBG_ACPI_PRINT_ERROR_WITH_STATUS_AND_PARAMS(...)
#endif
#ifndef DBG_ACPI_PRINT_WARNING
#define DBG_ACPI_PRINT_WARNING(...)
#endif
#ifndef DBG_ACPI_PRINT_TRACE
#define DBG_ACPI_PRINT_TRACE(...)
#endif
#ifndef DBG_ACPI_PRINT_INFO
#define DBG_ACPI_PRINT_INFO(...)
#endif

#ifndef DBG_SM_CODE
#define DBG_SM_CODE(...)
#endif
#ifndef DBG_SM_METHOD_BEG
#define DBG_SM_METHOD_BEG(...)
#endif
#ifndef DBG_SM_METHOD_BEG_WITH_PARAMS
#define DBG_SM_METHOD_BEG_WITH_PARAMS(...)
#endif
#ifndef DBG_SM_METHOD_END
#define DBG_SM_METHOD_END(...)
#endif
#ifndef DBG_SM_METHOD_END_WITH_STATUS
#define DBG_SM_METHOD_END_WITH_STATUS(...)
#endif
#ifndef DBG_SM_PRINT_ERROR
#define DBG_SM_PRINT_ERROR(...)
#endif
#ifndef DBG_SM_PRINT_ERROR_WITH_STATUS
#define DBG_SM_PRINT_ERROR_WITH_STATUS(...)
#endif
#ifndef DBG_SM_PRINT_ERROR_WITH_STATUS_AND_PARAMS
#define DBG_SM_PRINT_ERROR_WITH_STATUS_AND_PARAMS(...)
#endif
#ifndef DBG_SM_PRINT_WARNING
#define DBG_SM_PRINT_WARNING(...)
#endif
#ifndef DBG_SM_PRINT_TRACE
#define DBG_SM_PRINT_TRACE(...)
#endif
#ifndef DBG_SM_PRINT_INFO
#define DBG_SM_PRINT_INFO(...)
#endif

#ifndef DBG_MDIO_BUS_CODE
#define DBG_MDIO_BUS_CODE(...)
#endif
#ifndef DBG_MDIO_BUS_METHOD_BEG
#define DBG_MDIO_BUS_METHOD_BEG(...)
#endif
#ifndef DBG_MDIO_BUS_METHOD_BEG_WITH_PARAMS
#define DBG_MDIO_BUS_METHOD_BEG_WITH_PARAMS(...)
#endif
#ifndef DBG_MDIO_BUS_METHOD_END
#define DBG_MDIO_BUS_METHOD_END(...)
#endif
#ifndef DBG_MDIO_BUS_METHOD_END_WITH_STATUS
#define DBG_MDIO_BUS_METHOD_END_WITH_STATUS(...)
#endif
#ifndef DBG_MDIO_BUS_PRINT_ERROR
#define DBG_MDIO_BUS_PRINT_ERROR(...)
#endif
#ifndef DBG_MDIO_BUS_PRINT_ERROR_WITH_STATUS
#define DBG_MDIO_BUS_PRINT_ERROR_WITH_STATUS(...)
#endif
#ifndef DBG_MDIO_BUS_PRINT_ERROR_WITH_STATUS_AND_PARAMS
#define DBG_MDIO_BUS_PRINT_ERROR_WITH_STATUS_AND_PARAMS(...)
#endif
#ifndef DBG_MDIO_BUS_PRINT_WARNING
#define DBG_MDIO_BUS_PRINT_WARNING(...)
#endif
#ifndef DBG_MDIO_BUS_PRINT_TRACE
#define DBG_MDIO_BUS_PRINT_TRACE(...)
#endif
#ifndef DBG_MDIO_BUS_PRINT_INFO
#define DBG_MDIO_BUS_PRINT_INFO(...)
#endif

#ifndef DBG_MDIO_DEV_CODE
#define DBG_MDIO_DEV_CODE(...)
#endif
#ifndef DBG_MDIO_DEV_METHOD_BEG
#define DBG_MDIO_DEV_METHOD_BEG(...)
#endif
#ifndef DBG_MDIO_DEV_METHOD_BEG_WITH_PARAMS
#define DBG_MDIO_DEV_METHOD_BEG_WITH_PARAMS(...)
#endif
#ifndef DBG_MDIO_DEV_METHOD_END
#define DBG_MDIO_DEV_METHOD_END(...)
#endif
#ifndef DBG_MDIO_DEV_METHOD_END_WITH_STATUS
#define DBG_MDIO_DEV_METHOD_END_WITH_STATUS(...)
#endif
#ifndef DBG_MDIO_DEV_PRINT_ERROR
#define DBG_MDIO_DEV_PRINT_ERROR(...)
#endif
#ifndef DBG_MDIO_DEV_PRINT_ERROR_WITH_STATUS
#define DBG_MDIO_DEV_PRINT_ERROR_WITH_STATUS(...)
#endif
#ifndef DBG_MDIO_DEV_PRINT_ERROR_WITH_STATUS_AND_PARAMS
#define DBG_MDIO_DEV_PRINT_ERROR_WITH_STATUS_AND_PARAMS(...)
#endif
#ifndef DBG_MDIO_DEV_PRINT_WARNING
#define DBG_MDIO_DEV_PRINT_WARNING(...)
#endif
#ifndef DBG_MDIO_DEV_PRINT_TRACE
#define DBG_MDIO_DEV_PRINT_TRACE(...)
#endif
#ifndef DBG_MDIO_DEV_PRINT_INFO
#define DBG_MDIO_DEV_PRINT_INFO(...)
#endif

#ifndef DBG_MDIO_DEV_CMD_CODE
#define DBG_MDIO_DEV_CMD_CODE(...)
#endif
#ifndef DBG_MDIO_DEV_CMD_METHOD_BEG
#define DBG_MDIO_DEV_CMD_METHOD_BEG(...)
#endif
#ifndef DBG_MDIO_DEV_CMD_METHOD_BEG_WITH_PARAMS
#define DBG_MDIO_DEV_CMD_METHOD_BEG_WITH_PARAMS(...)
#endif
#ifndef DBG_MDIO_DEV_CMD_METHOD_END
#define DBG_MDIO_DEV_CMD_METHOD_END(...)
#endif
#ifndef DBG_MDIO_DEV_CMD_METHOD_END_WITH_STATUS
#define DBG_MDIO_DEV_CMD_METHOD_END_WITH_STATUS(...)
#endif
#ifndef DBG_MDIO_DEV_CMD_PRINT_ERROR
#define DBG_MDIO_DEV_CMD_PRINT_ERROR(...)
#endif
#ifndef DBG_MDIO_DEV_CMD_PRINT_ERROR_WITH_STATUS
#define DBG_MDIO_DEV_CMD_PRINT_ERROR_WITH_STATUS(...)
#endif
#ifndef DBG_MDIO_DEV_CMD_PRINT_ERROR_WITH_STATUS_AND_PARAMS
#define DBG_MDIO_DEV_CMD_PRINT_ERROR_WITH_STATUS_AND_PARAMS(...)
#endif
#ifndef DBG_MDIO_DEV_CMD_PRINT_WARNING
#define DBG_MDIO_DEV_CMD_PRINT_WARNING(...)
#endif
#ifndef DBG_MDIO_DEV_CMD_PRINT_TRACE
#define DBG_MDIO_DEV_CMD_PRINT_TRACE(...)
#endif
#ifndef DBG_MDIO_DEV_CMD_PRINT_INFO
#define DBG_MDIO_DEV_CMD_PRINT_INFO(...)
#endif

#endif // _MP_DBG_H
