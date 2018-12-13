/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

    enum.c

Abstract:

    This file demonstrates sample methodologies of enumerating secure services
    from a secure environment that does not support runtime enumeration.

    ACPI: Store secure services in a _DSM method under the device.

    Registry: Store secure services in a registry key from the INF.

Environment:

    Kernel mode

*/

#include <ntddk.h>
#include <wdf.h>
#include <acpiioct.h>
#include <initguid.h>
#include <TrustedRuntimeClx.h>
#include "OpteeTrEE.h"

#define OPTEE_TREE_DEFAULT_MAJOR_VERSION 1
#define OPTEE_TREE_DEFAULT_MINOR_VERSION 0
#define OPTEE_TREE_DEFAULT_MAX_SIMULTANEOUS_REQUESTS 1

#define DSM_OPTEE_TREE_SERVICES_REVISION 1
#define DSM_OPTEE_TREE_SERVICES_ENUMERATE 1
DEFINE_GUID(GUID_DSM_OPTEE_TREE_SERVICES, 0x418E2DA4, 0x7089, 0x4DDB, 0xAA, 0xCA, 0xA7, 0xE2, 0x37, 0x7D, 0xBE, 0xCE);

DECLARE_CONST_UNICODE_STRING(OPTEE_TREE_ENUM_REGISTRY_KEY_NAME, L"SecureServices");
DECLARE_CONST_UNICODE_STRING(OPTEE_TREE_ENUM_REGISTRY_MAJOR_VERSION_VALUE_NAME, L"MajorVersion");
DECLARE_CONST_UNICODE_STRING(OPTEE_TREE_ENUM_REGISTRY_MINOR_VERSION_VALUE_NAME, L"MinorVersion");
DECLARE_CONST_UNICODE_STRING(OPTEE_TREE_ENUM_REGISTRY_OS_DEPENDENCIES_VALUE_NAME, L"OSDependencies");
DECLARE_CONST_UNICODE_STRING(OPTEE_TREE_ENUM_REGISTRY_RESOURCE_DEPENDENCIES_VALUE_NAME, L"ResourceDependencies");
DECLARE_CONST_UNICODE_STRING(OPTEE_TREE_ENUM_REGISTRY_SECURITY_NAME, L"Security");

NTSTATUS
OpteeTreeQueryACPIFirmwareForSecureServices(
    _In_ WDFDEVICE Device,
    _Out_ WDFMEMORY *AcpiOutputBufferMemory
    )

/*++

    Routine Description:

        This routine attempts to evaluate the _DSM method of the object in the ACPI
        namespace in order to retrieve a list of secure services supported on this
        system.

    Arguments:

        Device - Supplies a handle to the device object.

        AcpiOutputBufferMemory - Supplies a pointer to be assigned a WDFMEMORY
                                 object containing ACPI_EVAL_OUTPUT_BUFFER results
                                 from the _DSM query on successful completion of
                                 this routine.

    Return Value:

        NTSTATUS code.

--*/

{

    ULONG_PTR BytesReturned;
    PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX EvalParameters;
    PACPI_METHOD_ARGUMENT EvalParametersArgument;
    WDFMEMORY EvalParametersMemory;
    ULONG EvalParametersSize;
    ACPI_EVAL_OUTPUT_BUFFER EvalResultsHeader;
    WDFMEMORY EvalResultsMemory;
    WDF_MEMORY_DESCRIPTOR EvalParametersMemDescriptor;
    WDF_MEMORY_DESCRIPTOR EvalResultsMemDescriptor;
    NTSTATUS Status;

    EvalParametersMemory = NULL;
    EvalResultsMemory = NULL;
    EvalParametersSize =
        (ULONG)(
        FIELD_OFFSET(ACPI_EVAL_INPUT_BUFFER_COMPLEX_EX, Argument) +
        ACPI_METHOD_ARGUMENT_LENGTH(sizeof(GUID)) +
        ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ULONG)) +
        ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ULONG)) +
        ACPI_METHOD_ARGUMENT_LENGTH(0));

    Status = WdfMemoryCreate(WDF_NO_OBJECT_ATTRIBUTES,
                             PagedPool,
                             OPTEE_TREE_POOL_TAG,
                             EvalParametersSize,
                             &EvalParametersMemory,
                             (PVOID *)&EvalParameters);

    if (!NT_SUCCESS(Status)) {
        goto OpteeTreeQueryACPIFirmwareForSecureServicesEnd;
    }

    EvalParameters->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE_EX;
    RtlCopyMemory(EvalParameters->MethodName, "_DSM", sizeof("_DSM"));
    EvalParameters->Size = EvalParametersSize;
    EvalParameters->ArgumentCount = 4;
    EvalParametersArgument = EvalParameters->Argument;
    EvalParameters->Argument;
    ACPI_METHOD_SET_ARGUMENT_BUFFER(EvalParametersArgument,
                                    &GUID_DSM_OPTEE_TREE_SERVICES,
                                    sizeof(GUID_DSM_OPTEE_TREE_SERVICES));

    EvalParametersArgument = ACPI_METHOD_NEXT_ARGUMENT(EvalParametersArgument);
    ACPI_METHOD_SET_ARGUMENT_INTEGER(EvalParametersArgument,
                                     DSM_OPTEE_TREE_SERVICES_REVISION);

    EvalParametersArgument = ACPI_METHOD_NEXT_ARGUMENT(EvalParametersArgument);
    ACPI_METHOD_SET_ARGUMENT_INTEGER(EvalParametersArgument,
                                     DSM_OPTEE_TREE_SERVICES_ENUMERATE);

    EvalParametersArgument = ACPI_METHOD_NEXT_ARGUMENT(EvalParametersArgument);
    RtlZeroMemory(EvalParametersArgument, sizeof(*EvalParametersArgument));
    EvalParametersArgument->Type = ACPI_METHOD_ARGUMENT_PACKAGE_EX;
    EvalParametersArgument->DataLength = 0;

    WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&EvalParametersMemDescriptor,
                                      EvalParametersMemory,
                                      0);

    RtlZeroMemory(&EvalResultsHeader, sizeof(EvalResultsHeader));
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&EvalResultsMemDescriptor,
                                      &EvalResultsHeader,
                                      sizeof(EvalResultsHeader));

    Status = WdfIoTargetSendIoctlSynchronously(WdfDeviceGetIoTarget(Device),
                                               NULL,
                                               IOCTL_ACPI_EVAL_METHOD_EX,
                                               &EvalParametersMemDescriptor,
                                               &EvalResultsMemDescriptor,
                                               NULL,
                                               &BytesReturned);

    if (Status == STATUS_BUFFER_OVERFLOW) {
        Status = STATUS_SUCCESS;

    } else if (!NT_SUCCESS(Status)) {

        goto OpteeTreeQueryACPIFirmwareForSecureServicesEnd;

    } else if ((BytesReturned < sizeof(EvalResultsHeader)) ||
               (EvalResultsHeader.Length <= sizeof(EvalResultsHeader))) {

        Status = STATUS_UNSUCCESSFUL;
        goto OpteeTreeQueryACPIFirmwareForSecureServicesEnd;
    }

    Status = WdfMemoryCreate(WDF_NO_OBJECT_ATTRIBUTES,
                             PagedPool,
                             OPTEE_TREE_POOL_TAG,
                             EvalResultsHeader.Length,
                             &EvalResultsMemory,
                             (PVOID *)&EvalParameters);

    if (!NT_SUCCESS(Status)) {
        goto OpteeTreeQueryACPIFirmwareForSecureServicesEnd;
    }

    WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&EvalResultsMemDescriptor,
                                      EvalResultsMemory,
                                      0);

    Status = WdfIoTargetSendIoctlSynchronously(WdfDeviceGetIoTarget(Device),
                                               NULL,
                                               IOCTL_ACPI_EVAL_METHOD_EX,
                                               &EvalParametersMemDescriptor,
                                               &EvalResultsMemDescriptor,
                                               NULL,
                                               &BytesReturned);

    if (!NT_SUCCESS(Status)) {
        goto OpteeTreeQueryACPIFirmwareForSecureServicesEnd;
    }

OpteeTreeQueryACPIFirmwareForSecureServicesEnd:

    if (EvalParametersMemory != NULL) {
        WdfObjectDelete(EvalParametersMemory);
        EvalParametersMemory = NULL;
    }

    if (NT_SUCCESS(Status)) {
        *AcpiOutputBufferMemory = EvalResultsMemory;

    } else {

        *AcpiOutputBufferMemory = NULL;
        if (EvalResultsMemory != NULL) {
            WdfObjectDelete(EvalResultsMemory);
            EvalResultsMemory = NULL;
        }
    }

    return Status;
}

NTSTATUS
OpteeTreeConvertACPIServiceDescription(
    _In_ ULONG Version,
    _In_ PACPI_METHOD_ARGUMENT AcpiDescription,
    _Inout_updates_bytes_opt_(*DescriptionSize) PTR_SECURE_SERVICE ServiceDescription,
    _Inout_ PULONG DescriptionSize
    )

/*++

    Routine Description:

        This routine decodes the service description from ACPI and fills in the
        secure service structure.

    Arguments:

        Version - Supplies the version of the description data provided.

        AcpiDescription - Supplies the ACPI provided description of the secure
                          service.

        ServiceDescription - Supplies a pointer to the secure service
                             description to be filled out.

        DescriptionSize - Supplies a pointer to the size of the description
                          passed in and on return, the number of bytes either
                          written or required.

    Return Value:

        STATUS_BUFFER_TOO_SMALL - DescriptionSize will contain the required size.

        NTSTATUS code.

--*/

{

    ULONG ArgumentIndex;
    WDF_OBJECT_ATTRIBUTES Attributes;
    ULONG DependencyIndex;
    PACPI_METHOD_ARGUMENT EndOfArguments;
    PACPI_METHOD_ARGUMENT EndOfDependencyArguments;
    PACPI_METHOD_ARGUMENT NextArgument;
    PACPI_METHOD_ARGUMENT NextDependencyArgument;
    ULONG RequiredSize;
    NTSTATUS Status;

    NT_ASSERT(AcpiDescription->Type == ACPI_METHOD_ARGUMENT_PACKAGE);

    if (ServiceDescription == NULL && DescriptionSize > 0) {
        return STATUS_INVALID_PARAMETER;
    }

    DependencyIndex = 0;
    RequiredSize = FIELD_OFFSET(TR_SECURE_SERVICE, Dependencies);
    NextArgument = (PACPI_METHOD_ARGUMENT)(AcpiDescription->Data);
    EndOfArguments = (PACPI_METHOD_ARGUMENT)
                     ((PUCHAR)NextArgument + AcpiDescription->DataLength);

    NT_ASSERT(Version == 1);

    if (Version != 1) {
        Status = STATUS_UNSUCCESSFUL;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = WdfObjectContextGetObject(ServiceDescription);
    for (ArgumentIndex = 0; ArgumentIndex < 5; ++ArgumentIndex) {
        if (NextArgument >= EndOfArguments) {
            break;
        }

        switch (ArgumentIndex) {

        //
        // Service ID
        //

        case 0:
            if (NextArgument->Type != ACPI_METHOD_ARGUMENT_BUFFER ||
                (ACPI_METHOD_ARGUMENT_LENGTH_FROM_ARGUMENT(NextArgument) !=
                 sizeof(GUID))) {

                Status = STATUS_UNSUCCESSFUL;
                goto OpteeTreeConvertACPIServiceDescriptionEnd;
            }

            if (*DescriptionSize >= RequiredSize) {
                ServiceDescription->ServiceGuid = *(LPCGUID)NextArgument->Data;
            }

            break;

        //
        // Major Version
        //

        case 1:
            if (NextArgument->Type != ACPI_METHOD_ARGUMENT_INTEGER) {
                Status = STATUS_UNSUCCESSFUL;
                goto OpteeTreeConvertACPIServiceDescriptionEnd;
            }

            if (*DescriptionSize >= RequiredSize) {
                ServiceDescription->MajorVersion = (ULONG)NextArgument->Argument;
            }

            break;

        //
        // Minor Version
        //

        case 2:
            if (NextArgument->Type != ACPI_METHOD_ARGUMENT_INTEGER) {
                Status = STATUS_UNSUCCESSFUL;
                goto OpteeTreeConvertACPIServiceDescriptionEnd;
            }

            if (*DescriptionSize >= RequiredSize) {
                ServiceDescription->MinorVersion = (ULONG)NextArgument->Argument;
            }

            break;

        //
        // Dependencies
        //

        case 3:
        case 4:
            if (NextArgument->Type != ACPI_METHOD_ARGUMENT_PACKAGE) {
                Status = STATUS_UNSUCCESSFUL;
                goto OpteeTreeConvertACPIServiceDescriptionEnd;
            }

            NextDependencyArgument =
                (PACPI_METHOD_ARGUMENT)(NextArgument->Data);

            EndOfDependencyArguments = (PACPI_METHOD_ARGUMENT)
                ((PUCHAR)NextDependencyArgument + NextArgument->DataLength);

            while (NextDependencyArgument < EndOfDependencyArguments) {
                if (NextDependencyArgument->Type != ACPI_METHOD_ARGUMENT_BUFFER ||
                    NextDependencyArgument->DataLength != sizeof(GUID)) {

                    Status = STATUS_UNSUCCESSFUL;
                    goto OpteeTreeConvertACPIServiceDescriptionEnd;
                }

                RequiredSize += sizeof(TR_SECURE_DEPENDENCY);

                if (*DescriptionSize >= RequiredSize) {
                    ServiceDescription->Dependencies[DependencyIndex].Id =
                        *(LPCGUID)NextArgument->Data;

                    if (ArgumentIndex == 3 ) {
                        ServiceDescription->Dependencies[DependencyIndex].Type =
                            TRSecureOSDependency;

                    } else {

                        ServiceDescription->Dependencies[DependencyIndex].Type =
                            TRSecureResourceDependency;
                    }
                }

                ++DependencyIndex;
                NextDependencyArgument = ACPI_METHOD_NEXT_ARGUMENT(NextArgument);
            }

            break;

        default:

            NT_ASSERT(FALSE);
            break;
        }

        NextArgument = ACPI_METHOD_NEXT_ARGUMENT(NextArgument);
    }

    RequiredSize = max(RequiredSize, sizeof(TR_SECURE_SERVICE));
    switch (ArgumentIndex) {
    case 0:

        //
        // At a minimum require ID.
        //

        Status = STATUS_UNSUCCESSFUL;
        goto OpteeTreeConvertACPIServiceDescriptionEnd;
        break;

    case 1:
        if (*DescriptionSize >= RequiredSize) {
            ServiceDescription->MajorVersion =
                OPTEE_TREE_DEFAULT_MAJOR_VERSION;
        }

        //
        // fall through
        //

    case 2:
        if (*DescriptionSize >= RequiredSize) {
            ServiceDescription->MinorVersion =
                OPTEE_TREE_DEFAULT_MINOR_VERSION;
        }

        //
        // fall through
        //

    case 3:
    case 4:
        if (*DescriptionSize >= RequiredSize) {
            ServiceDescription->DescriptionSize = RequiredSize;
            ServiceDescription->CountDependencies = DependencyIndex;
        }

        break;

    default:

        NT_ASSERT(FALSE);
        break;
    }

    if (*DescriptionSize < RequiredSize) {
        Status = STATUS_BUFFER_TOO_SMALL;

    } else {

        Status = STATUS_SUCCESS;
    }

    *DescriptionSize = RequiredSize;

OpteeTreeConvertACPIServiceDescriptionEnd:
    return Status;
}

NTSTATUS
TreeEnumSecureServicesFromACPI(
    _In_ WDFDEVICE Device,
    _In_ WDFCOLLECTION ServiceCollection
    )

/*++

    Routine Description:

        This routine queries the ACPI namespace for information about the secure
        services.

    Arguments:

        Device - Supplies a handle to the device object.

        ServiceCollection - Supplies a pointer to the collection in which to
                            add enumerated services.

    Notes:

        [Example ASL]

        Name(SVCS, Package()
        {
            //
            // Data Format Version Information
            //
            1,

            //
            // Service 1
            //
            Package()
            {
                //
                // ID
                //
                ToUUID("12345678-9abc-def0-1234-56789abcdef0"),

                //
                // Major Version
                //
                1,

                //
                // Minor Version
                //
                0,

                //
                // OS Dependencies
                //
                Package() {},

                //
                // Secure Resource Dependencies
                //
                Package() {}
            },

            //
            // Service 2
            //
            Package()
            {
                //
                // ID
                //
                ToUUID("0fedcba9-8765-4321-0fed-cba987654321"),

                //
                // Major Version
                //
                1,

                //
                // Minor Version
                //
                0,

                //
                // OS Dependencies
                //
                Package() {},

                //
                // Secure Resource Dependencies
                //
                Package() {}
            }
        )

        Function(_DSM,{IntObj,BuffObj},{BuffObj, IntObj, IntObj, PkgObj})
        {
            switch(ToBuffer(Arg0))
            {
                case(ToUUID("418E2DA4-7089-4DDB-AACA-A7E2377DBECE"))
                {
                    switch(ToInteger(Arg2))
                    {
                        //
                        // Function 0: Return supported functions, based on revision
                        //
                        case(0)
                        {
                            return (Buffer() {0x3})
                        }

                        //
                        // Function 1: Return list of secure service descriptions
                        //
                        case(1)
                        {
                            Return(SVCS)
                        }

                        default {BreakPoint}
                    }
                }

                default {BreakPoint}
            }

            //
            // If not one of the function identifiers we recognize, then return a buffer
            // with bit 0 set to 0 indicating no functions supported.
            //
            return(Buffer(){0})
        }

    Return Value:

        NTSTATUS code.

--*/

{

    PACPI_EVAL_OUTPUT_BUFFER EvalResults;
    WDFMEMORY EvalResultsMemory;
    size_t EvalResultsMemorySize;
    PACPI_METHOD_ARGUMENT NextArgument;
    PTR_SECURE_SERVICE NewService;
    WDF_OBJECT_ATTRIBUTES NewServiceAttributes;
    WDFMEMORY NewServiceMemory;
    ULONG NewServiceSize;
    ULONG ServiceIndex;
    NTSTATUS Status;
    ULONG Version;

    EvalResultsMemory = NULL;
    NewServiceMemory = NULL;
    Status = OpteeTreeQueryACPIFirmwareForSecureServices(Device,
                                                          &EvalResultsMemory);

    if (!NT_SUCCESS(Status)) {
        goto TreeEnumSecureServicesFromACPIEnd;
    }

    EvalResults = (PACPI_EVAL_OUTPUT_BUFFER)
                  WdfMemoryGetBuffer(EvalResultsMemory,
                                     &EvalResultsMemorySize);

    if (EvalResults->Count < 1) {
        Status = STATUS_UNSUCCESSFUL;
        goto TreeEnumSecureServicesFromACPIEnd;
    }

    //
    // First element is the version of the remaining data.
    //

    NextArgument = (PACPI_METHOD_ARGUMENT)EvalResults->Argument;
    if (NextArgument->Type != ACPI_METHOD_ARGUMENT_INTEGER) {
        Status = STATUS_UNSUCCESSFUL;
        goto TreeEnumSecureServicesFromACPIEnd;
    }

    Version = NextArgument->Argument;

    //
    // Allocate memory for each service and store in ServiceColleciton.
    //

    WDF_OBJECT_ATTRIBUTES_INIT(&NewServiceAttributes);
    NewServiceAttributes.ParentObject = ServiceCollection;
    for (ServiceIndex = 0;
         ServiceIndex < (EvalResults->Count - 1);
         ++ServiceIndex) {

        NextArgument = ACPI_METHOD_NEXT_ARGUMENT(NextArgument);
        if (NextArgument->Type != ACPI_METHOD_ARGUMENT_PACKAGE) {
            Status = STATUS_UNSUCCESSFUL;
            goto TreeEnumSecureServicesFromACPIEnd;
        }

        NewServiceSize = 0;
        Status = OpteeTreeConvertACPIServiceDescription(Version,
                                                         NextArgument,
                                                         NULL,
                                                         &NewServiceSize);

        if (Status != STATUS_BUFFER_TOO_SMALL) {

            if (NT_SUCCESS(Status)) {
                Status = STATUS_UNSUCCESSFUL;
            }

            goto TreeEnumSecureServicesFromACPIEnd;
        }

        NT_ASSERT(NewServiceSize >= sizeof(TR_SECURE_SERVICE));

        Status = WdfMemoryCreate(&NewServiceAttributes,
                                 NonPagedPoolNx,
                                 OPTEE_TREE_POOL_TAG,
                                 NewServiceSize,
                                 &NewServiceMemory,
                                 &NewService);

        if (!NT_SUCCESS(Status)) {
            goto TreeEnumSecureServicesFromACPIEnd;
        }

        Status = OpteeTreeConvertACPIServiceDescription(Version,
                                                         NextArgument,
                                                         NewService,
                                                         &NewServiceSize);

        if (!NT_SUCCESS(Status)) {
            goto TreeEnumSecureServicesFromACPIEnd;
        }

        Status = WdfCollectionAdd(ServiceCollection,
                                  NewServiceMemory);

        if (!NT_SUCCESS(Status)) {
            goto TreeEnumSecureServicesFromACPIEnd;
        }

        NewService = NULL;
        NewServiceMemory = NULL;
    }

TreeEnumSecureServicesFromACPIEnd:

    if (EvalResultsMemory != NULL) {
        WdfObjectDelete(EvalResultsMemory);
        EvalResultsMemory = NULL;
    }

    if (NewServiceMemory != NULL) {
        WdfObjectDelete(NewServiceMemory);
        NewServiceMemory = NULL;
    }

    return Status;
}

NTSTATUS
OpteeTreeConvertRegistryServiceDescription(
    _In_ WDFKEY SecureServiceParentKey,
    _In_ PCUNICODE_STRING SecureServiceName,
    _Inout_updates_bytes_opt_(*DescriptionSize) PTR_SECURE_SERVICE ServiceDescription,
    _Inout_ PULONG DescriptionSize
    )

/*++

    Routine Description:

        This routine uses the values in a registry subkey entry for a secure
        service to populate the contents of the service description.

    Arguments:

        SecureServiceParentKey - Supplies a handle to the registry key under which
                                 the secure service subkey is located.

        SecureServiceName - Supplies the name of the secure service subkey.

        ServiceDescription - Supplies a pointer to the secure service
                             description to be filled out.

        DescriptionSize - Supplies a pointer to the size of the description
                          passed in and on return, the number of bytes either
                          written or required.

    Return Value:

        STATUS_BUFFER_TOO_SMALL - DescriptionSize will contain the required size.

        NTSTATUS code.

--*/

{

    WDFSTRING DependencyId;
    UNICODE_STRING DependencyIdName;
    ULONG DependencyIndex;
    ULONG ExtensionOffset;
    GUID GuidValue;
    WDFCOLLECTION StringCollection;
    ULONG RequiredSize;
    PTR_SECURE_SERVICE_EXTENSION ServiceExtension;
    WDFKEY ServiceKey;
    NTSTATUS Status;
    WDFSTRING StringObject;
    UNICODE_STRING StringValue;
    ULONG UlongValue;

    DependencyId = NULL;
    ServiceKey = NULL;
    StringCollection = NULL;
    StringObject = NULL;

    RequiredSize = FIELD_OFFSET(TR_SECURE_SERVICE, Dependencies);
    Status = RtlGUIDFromString(SecureServiceName, &GuidValue);
    NT_ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status)) {
        goto OpteeTreeConvertRegistryServiceDescriptionEnd;
    }

    Status = WdfRegistryOpenKey(SecureServiceParentKey,
                                SecureServiceName,
                                KEY_READ,
                                WDF_NO_OBJECT_ATTRIBUTES,
                                &ServiceKey);

    if (!NT_SUCCESS(Status)) {
        goto OpteeTreeConvertRegistryServiceDescriptionEnd;
    }

    if (*DescriptionSize >= RequiredSize) {
        RtlZeroMemory(ServiceDescription, RequiredSize);
        ServiceDescription->ServiceGuid = GuidValue;
    }

    //
    // Major version
    //

    Status = WdfRegistryQueryULong(
                 ServiceKey,
                 &OPTEE_TREE_ENUM_REGISTRY_MAJOR_VERSION_VALUE_NAME,
                 &UlongValue);

    if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        UlongValue = OPTEE_TREE_DEFAULT_MAJOR_VERSION;
        Status = STATUS_SUCCESS;

    } else if (!NT_SUCCESS(Status)) {

        goto OpteeTreeConvertRegistryServiceDescriptionEnd;
    }

    if (*DescriptionSize >= RequiredSize) {
        ServiceDescription->MajorVersion = UlongValue;
    }

    //
    // Minor version
    //

    Status = WdfRegistryQueryULong(
        ServiceKey,
        &OPTEE_TREE_ENUM_REGISTRY_MINOR_VERSION_VALUE_NAME,
        &UlongValue);

    if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        UlongValue = OPTEE_TREE_DEFAULT_MINOR_VERSION;
        Status = STATUS_SUCCESS;

    } else if (!NT_SUCCESS(Status)) {

        goto OpteeTreeConvertRegistryServiceDescriptionEnd;
    }

    if (*DescriptionSize >= RequiredSize) {
        ServiceDescription->MinorVersion = UlongValue;
    }

    //
    // OS Dependencies
    //

    DependencyIndex = 0;
    Status = WdfCollectionCreate(WDF_NO_OBJECT_ATTRIBUTES, &StringCollection);
    if (!NT_SUCCESS(Status)) {
        goto OpteeTreeConvertRegistryServiceDescriptionEnd;
    }

    Status = WdfRegistryQueryMultiString(
                 ServiceKey,
                 &OPTEE_TREE_ENUM_REGISTRY_OS_DEPENDENCIES_VALUE_NAME,
                 WDF_NO_OBJECT_ATTRIBUTES,
                 StringCollection);

    if (NT_SUCCESS(Status)) {
        while (WdfCollectionGetCount(StringCollection) > 0) {
            DependencyId = (WDFSTRING)
                WdfCollectionGetFirstItem(StringCollection);

            WdfCollectionRemoveItem(StringCollection, 0);
            WdfStringGetUnicodeString(DependencyId, &DependencyIdName);
            Status = RtlGUIDFromString(&DependencyIdName, &GuidValue);
            if (!NT_SUCCESS(Status)) {
                goto OpteeTreeConvertRegistryServiceDescriptionEnd;
            }

            RequiredSize += sizeof(TR_SECURE_DEPENDENCY);
            if (*DescriptionSize >= RequiredSize) {
                ServiceDescription->Dependencies[DependencyIndex].Id = GuidValue;
                ServiceDescription->Dependencies[DependencyIndex].Type =
                    TRSecureOSDependency;
            }

            ++DependencyIndex;
            WdfObjectDelete(DependencyId);
            DependencyId = NULL;
        }

    } else if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {

        NT_ASSERT(!NT_SUCCESS(Status));

        if (NT_SUCCESS(Status)) {
            Status = STATUS_UNSUCCESSFUL;
        }

        goto OpteeTreeConvertRegistryServiceDescriptionEnd;
    }

    //
    // Secure Resource Dependencies
    //

    Status = WdfRegistryQueryMultiString(
        ServiceKey,
        &OPTEE_TREE_ENUM_REGISTRY_RESOURCE_DEPENDENCIES_VALUE_NAME,
        WDF_NO_OBJECT_ATTRIBUTES,
        StringCollection);

    if (NT_SUCCESS(Status)) {
        while (WdfCollectionGetCount(StringCollection) > 0) {
            DependencyId = (WDFSTRING)
                WdfCollectionGetFirstItem(StringCollection);

            WdfCollectionRemoveItem(StringCollection, 0);
            WdfStringGetUnicodeString(DependencyId, &DependencyIdName);
            Status = RtlGUIDFromString(&DependencyIdName, &GuidValue);
            if (!NT_SUCCESS(Status)) {
                goto OpteeTreeConvertRegistryServiceDescriptionEnd;
            }

            RequiredSize += sizeof(TR_SECURE_DEPENDENCY);
            if (*DescriptionSize >= RequiredSize) {
                ServiceDescription->Dependencies[DependencyIndex].Id = GuidValue;
                ServiceDescription->Dependencies[DependencyIndex].Type =
                    TRSecureResourceDependency;
            }

            ++DependencyIndex;
            WdfObjectDelete(DependencyId);
            DependencyId = NULL;
        }

    } else if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {

        NT_ASSERT(!NT_SUCCESS(Status));

        if (NT_SUCCESS(Status)) {
            Status = STATUS_UNSUCCESSFUL;
        }

        goto OpteeTreeConvertRegistryServiceDescriptionEnd;
    }

    //
    // Security
    //

    Status = WdfStringCreate(NULL, WDF_NO_OBJECT_ATTRIBUTES, &StringObject);
    if (!NT_SUCCESS(Status)) {
        goto OpteeTreeConvertRegistryServiceDescriptionEnd;
    }

    Status = WdfRegistryQueryString(
        ServiceKey,
        &OPTEE_TREE_ENUM_REGISTRY_SECURITY_NAME,
        StringObject);

    if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        StringObject = NULL;
        Status = STATUS_SUCCESS;

    } else if (!NT_SUCCESS(Status)) {

        goto OpteeTreeConvertRegistryServiceDescriptionEnd;
    }

    if (StringObject != NULL) {
        WdfStringGetUnicodeString(StringObject, &StringValue);
        ExtensionOffset = RequiredSize;
        RequiredSize += sizeof(TR_SECURE_SERVICE_EXTENSION) + StringValue.Length + sizeof(WCHAR);
        if (*DescriptionSize >= RequiredSize) {
            ServiceDescription->ExtensionOffset = ExtensionOffset;
            ServiceExtension = (PTR_SECURE_SERVICE_EXTENSION)(((ULONG_PTR)ServiceDescription) + ExtensionOffset);
            ServiceExtension->ExtensionVersion = TR_SECURE_SERVICE_EXTENSION_VERSION;
            ServiceExtension->SecurityDescriptorStringOffset = ExtensionOffset + sizeof(TR_SECURE_SERVICE_EXTENSION);
            RtlCopyMemory((PVOID)(((ULONG_PTR)ServiceDescription) + ServiceExtension->SecurityDescriptorStringOffset),
                          StringValue.Buffer,
                          StringValue.Length);

            *(PWCHAR)(((ULONG_PTR)ServiceDescription) + ServiceExtension->SecurityDescriptorStringOffset + StringValue.Length) = L'\0';
        }
    }

    if (*DescriptionSize >= RequiredSize) {
        ServiceDescription->MinorVersion = UlongValue;
    }

    if (*DescriptionSize < RequiredSize) {
        Status = STATUS_BUFFER_TOO_SMALL;

    } else {

        ServiceDescription->DescriptionSize = RequiredSize;
        ServiceDescription->CountDependencies = DependencyIndex;
        Status = STATUS_SUCCESS;
    }

    *DescriptionSize = RequiredSize;

OpteeTreeConvertRegistryServiceDescriptionEnd:

    if (ServiceKey != NULL) {
        WdfRegistryClose(ServiceKey);
        ServiceKey = NULL;
    }

    if (StringCollection != NULL) {
        while (WdfCollectionGetCount(StringCollection) > 0) {
            WdfObjectDelete(WdfCollectionGetFirstItem(StringCollection));
            WdfCollectionRemoveItem(StringCollection, 0);
        }

        WdfObjectDelete(StringCollection);
        StringCollection = NULL;
    }

    if (StringObject != NULL) {
        WdfObjectDelete(StringObject);
        StringObject = NULL;
    }

    if (DependencyId != NULL) {
        WdfObjectDelete(DependencyId);
        DependencyId = NULL;
    }

    return Status;
}

NTSTATUS
TreeEnumSecureServicesFromRegistry(
    _In_ WDFDEVICE Device,
    _In_ WDFCOLLECTION ServiceCollection
    )

/*++

    Routine Description:

        This routine queries the device instance key for information about the
        secure services.

    Arguments:

        Device - Supplies a handle to the device object.

        ServiceCollection - Supplies a pointer to the collection in which to
                            add enumerated services.

    Notes:

        [Example Registry]

        <Instance Key>
            Parameters
                SecureServices
                    {<Service GUID>}
                        MajorVersion = (DWORD)1
                        MinorVersion = (DWORD)1
                        OSDependencies = (REG_MULTI_SZ)
                        ResourceDependencies = (REG_MUTLI_SZ)

    Return Value:

        NTSTATUS code.

--*/

{

    PKEY_BASIC_INFORMATION KeyInformation;
    WDFMEMORY KeyInformationMemory;
    ULONG KeyInformationSize;
    ULONG KeyInformationSizeRequired;
    PTR_SECURE_SERVICE NewService;
    WDF_OBJECT_ATTRIBUTES NewServiceAttributes;
    WDFMEMORY NewServiceMemory;
    ULONG NewServiceSize;
    WDFKEY ParametersKey;
    ULONG ServiceIndex;
    UNICODE_STRING ServiceKeyName;
    WDFKEY ServicesParentKey;
    NTSTATUS Status;
    WDFMEMORY TempKeyInformationMemory;

    ParametersKey = NULL;
    ServicesParentKey = NULL;
    KeyInformationMemory = NULL;
    NewServiceMemory = NULL;

    Status = WdfDriverOpenParametersRegistryKey(WdfDeviceGetDriver(Device),
                                                KEY_READ,
                                                WDF_NO_OBJECT_ATTRIBUTES,
                                                &ParametersKey);

    if (!NT_SUCCESS(Status)) {
        goto TreeEnumSecureServicesFromRegistryEnd;
    }

    Status = WdfRegistryOpenKey(ParametersKey,
                                &OPTEE_TREE_ENUM_REGISTRY_KEY_NAME,
                                KEY_READ,
                                WDF_NO_OBJECT_ATTRIBUTES,
                                &ServicesParentKey);

    if (!NT_SUCCESS(Status)) {
        goto TreeEnumSecureServicesFromRegistryEnd;
    }

    KeyInformationSize = sizeof(*KeyInformation) + (128 * sizeof(WCHAR));
    Status = WdfMemoryCreate(WDF_NO_OBJECT_ATTRIBUTES,
                             NonPagedPoolNx,
                             OPTEE_TREE_POOL_TAG,
                             KeyInformationSize,
                             &KeyInformationMemory,
                             (PVOID *)&KeyInformation);

    if (!NT_SUCCESS(Status)) {
        goto TreeEnumSecureServicesFromRegistryEnd;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&NewServiceAttributes);
    NewServiceAttributes.ParentObject = Device;
    ServiceIndex = 0;
    do {
        Status = ZwEnumerateKey(WdfRegistryWdmGetHandle(ServicesParentKey),
                                ServiceIndex,
                                KeyBasicInformation,
                                KeyInformation,
                                KeyInformationSize,
                                &KeyInformationSizeRequired);

        if (Status == STATUS_NO_MORE_ENTRIES) {
            Status = STATUS_SUCCESS;
            break;

        } else if (Status == STATUS_BUFFER_TOO_SMALL ||
                   Status == STATUS_BUFFER_OVERFLOW) {

            NT_ASSERT(KeyInformationSizeRequired > KeyInformationSize);

            if (KeyInformationSizeRequired >
                (sizeof(*KeyInformation) + UNICODE_STRING_MAX_BYTES)) {

                NT_ASSERT(FALSE);
                ++ServiceIndex;
                continue;
            }

            Status = WdfMemoryCreate(WDF_NO_OBJECT_ATTRIBUTES,
                                     NonPagedPoolNx,
                                     OPTEE_TREE_POOL_TAG,
                                     KeyInformationSizeRequired,
                                     &TempKeyInformationMemory,
                                     NULL);

            if (NT_SUCCESS(Status)) {
                WdfObjectDelete(KeyInformationMemory);
                KeyInformationMemory = TempKeyInformationMemory;
                TempKeyInformationMemory = NULL;
                KeyInformationSize = KeyInformationSizeRequired;
                KeyInformation = (PKEY_BASIC_INFORMATION)
                                 WdfMemoryGetBuffer(KeyInformationMemory,
                                                    NULL);

            } else {

                //
                // Can't allocate bigger buffer - skip this entry.
                //

                ++ServiceIndex;
            }

            continue;

        }
        else if (!NT_SUCCESS(Status)) {

            break;
        }

        if (KeyInformation->NameLength >
            (KeyInformationSize - FIELD_OFFSET(KEY_BASIC_INFORMATION, Name))) {

            NT_ASSERT(FALSE);
            Status = STATUS_UNSUCCESSFUL;
            goto TreeEnumSecureServicesFromRegistryEnd;
        }

        ServiceKeyName.Length = (USHORT)KeyInformation->NameLength;
        ServiceKeyName.MaximumLength = ServiceKeyName.Length;
        ServiceKeyName.Buffer = KeyInformation->Name;
        NewServiceSize = 0;
        Status = OpteeTreeConvertRegistryServiceDescription(
                     ServicesParentKey,
                     &ServiceKeyName,
                     NULL,
                     &NewServiceSize);

        if (Status != STATUS_BUFFER_TOO_SMALL) {

            if (NT_SUCCESS(Status)) {
                Status = STATUS_UNSUCCESSFUL;
            }

            goto TreeEnumSecureServicesFromRegistryEnd;
        }

        NT_ASSERT(NewServiceSize >= sizeof(TR_SECURE_SERVICE) - sizeof(TR_SECURE_DEPENDENCY_V1));

        Status = WdfMemoryCreate(&NewServiceAttributes,
                                 NonPagedPoolNx,
                                 OPTEE_TREE_POOL_TAG,
                                 NewServiceSize,
                                 &NewServiceMemory,
                                 &NewService);

        if (!NT_SUCCESS(Status)) {
            goto TreeEnumSecureServicesFromRegistryEnd;
        }

        Status = OpteeTreeConvertRegistryServiceDescription(
                     ServicesParentKey,
                     &ServiceKeyName,
                     NewService,
                     &NewServiceSize);

        if (!NT_SUCCESS(Status)) {
            goto TreeEnumSecureServicesFromRegistryEnd;
        }

        Status = WdfCollectionAdd(ServiceCollection,
                                  NewServiceMemory);

        if (!NT_SUCCESS(Status)) {
            goto TreeEnumSecureServicesFromRegistryEnd;
        }

        NewService = NULL;
        NewServiceMemory = NULL;
        ++ServiceIndex;

    } while (NT_SUCCESS(Status));

TreeEnumSecureServicesFromRegistryEnd:

    if (ParametersKey != NULL) {
        WdfRegistryClose(ParametersKey);
        ParametersKey = NULL;
    }

    if (ServicesParentKey != NULL) {
        WdfRegistryClose(ServicesParentKey);
        ServicesParentKey = NULL;
    }

    if (KeyInformationMemory != NULL) {
        WdfObjectDelete(KeyInformationMemory);
        KeyInformationMemory = NULL;
    }

    if (NewServiceMemory != NULL) {
        WdfObjectDelete(NewServiceMemory);
        NewServiceMemory = NULL;
    }

    return Status;
}
