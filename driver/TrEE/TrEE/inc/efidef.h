/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#ifndef _EFIDEF_
#define _EFIDEF_

typedef UINT16 CHAR16;

#ifdef _WIN64
#define EFI_ERROR(x) (0x8000000000000000 | x)
typedef UINT64 EFI_STATUS;
#else // _WIN32
#define EFI_ERROR(x) (0x80000000 | x)
typedef UINT32 EFI_STATUS;
#endif

#define EFI_SUCCESS                               0
#define EFI_LOAD_ERROR                  EFI_ERROR(1)
#define EFI_INVALID_PARAMETER           EFI_ERROR(2)
#define EFI_UNSUPPORTED                 EFI_ERROR(3)
#define EFI_BAD_BUFFER_SIZE             EFI_ERROR(4)
#define EFI_BUFFER_TOO_SMALL            EFI_ERROR(5)
#define EFI_NOT_READY                   EFI_ERROR(6)
#define EFI_DEVICE_ERROR                EFI_ERROR(7)
#define EFI_WRITE_PROTECTED             EFI_ERROR(8)
#define EFI_OUT_OF_RESOURCES            EFI_ERROR(9)
#define EFI_VOLUME_CORRUPTED            EFI_ERROR(10)
#define EFI_VOLUME_FULL                 EFI_ERROR(11)
#define EFI_NO_MEDIA                    EFI_ERROR(12)
#define EFI_MEDIA_CHANGED               EFI_ERROR(13)
#define EFI_NOT_FOUND                   EFI_ERROR(14)
#define EFI_ACCESS_DENIED               EFI_ERROR(15)
#define EFI_NO_RESPONSE                 EFI_ERROR(16)
#define EFI_NO_MAPPING                  EFI_ERROR(17)
#define EFI_TIMEOUT                     EFI_ERROR(18)
#define EFI_NOT_STARTED                 EFI_ERROR(19)
#define EFI_ALREADY_STARTED             EFI_ERROR(20)
#define EFI_ABORTED                     EFI_ERROR(21)
#define EFI_ICMP_ERROR                  EFI_ERROR(22)
#define EFI_TFTP_ERROR                  EFI_ERROR(23)
#define EFI_PROTOCOL_ERROR              EFI_ERROR(24)
#define EFI_INCOMPATIBLE_VERSION        EFI_ERROR(25)
#define EFI_SECURITY_VIOLATION          EFI_ERROR(26)
#define EFI_CRC_ERROR                   EFI_ERROR(27)
#define EFI_END_OF_MEDIA                EFI_ERROR(28)
#define EFI_END_OF_FILE                 EFI_ERROR(31)
#define EFI_INVALID_LANGUAGE            EFI_ERROR(32)
#define EFI_COMPROMISED_DATA            EFI_ERROR(33)
#define EFI_IP_ADDRESS_CONFLICT         EFI_ERROR(34)
#define EFI_HTTP_ERROR                  EFI_ERROR(35)

#endif // _EFIDEF_
