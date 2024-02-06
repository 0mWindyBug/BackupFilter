#include <main.h>


#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")


PFLT_FILTER gFilterHandle;
Mutex FileStateContextsMutex;

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {



    {IRP_MJ_CREATE,NULL,nullptr,BackupFilterPostCreate},
    {IRP_MJ_WRITE,FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,BackupFilterPreWrite,nullptr},
    { IRP_MJ_OPERATION_END }
};

const FLT_CONTEXT_REGISTRATION Contexts[] = {
    { FLT_FILE_CONTEXT, 0, nullptr, sizeof(FileStateContext), TAG },
    { FLT_CONTEXT_END }
};


CONST FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                  //  Flags

    Contexts,                               //  Context
    Callbacks,                          //  Operation callbacks

    BackupFilterUnload,                           //  MiniFilterUnload

    BackupFilterInstanceSetup,                    //  InstanceSetup
    nullptr,            //  InstanceQueryTeardown
    nullptr,            //  InstanceTeardownStart
    nullptr,         //  InstanceTeardownComplete

    nullptr,                               //  GenerateFileName
    nullptr,                               //  GenerateDestinationFileName
    nullptr                                //  NormalizeNameComponent

};



NTSTATUS
BackupFilterInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )

{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( VolumeDeviceType );
    UNREFERENCED_PARAMETER( VolumeFilesystemType );

    PAGED_CODE();



    return STATUS_SUCCESS;
}



/*************************************************************************
    MiniFilter initialization and unload routines.
*************************************************************************/

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )



{
    NTSTATUS status;
    UNICODE_STRING BackupDirectory;
    RtlInitUnicodeString(&BackupDirectory, BACKUP_DIR);
    CreateBackupDirectory(&BackupDirectory);
    FileStateContextsMutex.Init();

    UNREFERENCED_PARAMETER( RegistryPath );

    status = FltRegisterFilter( DriverObject,
                                &FilterRegistration,
                                &gFilterHandle );

    FLT_ASSERT( NT_SUCCESS( status ) );

    if (NT_SUCCESS( status )) {



        status = FltStartFiltering( gFilterHandle );

        if (!NT_SUCCESS( status )) {

            FltUnregisterFilter( gFilterHandle );
        }
    }
    DbgPrint("[*] starting backup operation\n");
    return status;
}

NTSTATUS
BackupFilterUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    )

{

    PAGED_CODE();
    FltUnregisterFilter( gFilterHandle );
    UNREFERENCED_PARAMETER(Flags);
    ReleaseFileStateContexts();
    return STATUS_SUCCESS;
}

