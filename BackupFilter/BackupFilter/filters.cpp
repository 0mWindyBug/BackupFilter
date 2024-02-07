#include <main.h>

PFLT_PORT FilterPort;
PFLT_PORT SendClientPort;

// handle client connections 
NTSTATUS PortConnectNotify(
	PFLT_PORT ClientPort, PVOID ServerPortCookie, PVOID ConnectionContext,
	ULONG SizeOfContext, PVOID* ConnectionPortCookie) {
	UNREFERENCED_PARAMETER(ServerPortCookie);
	UNREFERENCED_PARAMETER(ConnectionContext);
	UNREFERENCED_PARAMETER(SizeOfContext);
	UNREFERENCED_PARAMETER(ConnectionPortCookie);
	SendClientPort = ClientPort;
	return STATUS_SUCCESS;
}

// handle client disconnections 
void PortDisconnectNotify(PVOID ConnectionCookie) {
	UNREFERENCED_PARAMETER(ConnectionCookie);
	FltCloseClientPort(gFilterHandle, &SendClientPort);
	SendClientPort = nullptr;
}

NTSTATUS PortMessageNotify(
	_In_opt_ PVOID PortCookie,
	_In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
	_In_ ULONG InputBufferLength,
	_Out_writes_bytes_to_opt_(OutputBufferLength, *ReturnOutputBufferLength) PVOID OutputBuffer,
	_In_ ULONG OutputBufferLength,
	_Out_ PULONG ReturnOutputBufferLength)
{
	UNREFERENCED_PARAMETER(PortCookie);
	UNREFERENCED_PARAMETER(OutputBuffer);
	UNREFERENCED_PARAMETER(OutputBufferLength);


	if (!SendClientPort)
		return STATUS_FAIL_CHECK;
	if (InputBuffer == NULL || InputBufferLength < sizeof(FilterBackupRequest)) {
		return STATUS_INVALID_PARAMETER;
	}

	PFilterBackupRequest request = (PFilterBackupRequest)InputBuffer;
	request->FileName[request->FileNameLength] = L'\0';

	UNICODE_STRING fileName;
	fileName.Length = request->FileNameLength;
	fileName.MaximumLength = request->FileNameLength;
	fileName.Buffer = request->FileName;
	DbgPrint("[*] setting new restore point for %wZ\n",fileName);
	UnsetFileRestorePoint(&fileName);

	*ReturnOutputBufferLength = 0;
	return STATUS_SUCCESS;
}



// role : create a backup for the file if needed
FLT_PREOP_CALLBACK_STATUS
BackupFilterPreWrite(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
)

{
    NTSTATUS status;
	ULONG BytesRead = 0;
	FILE_STANDARD_INFORMATION FileInfo;
	PVOID DiskContent;
	LARGE_INTEGER ByteOffset;
	ByteOffset.QuadPart = 0;

    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);
	// check if file is monitored and it hasnt been backed up yet 
	FileContx FileCtx(FltObjects->Instance, FltObjects->FileObject);
	FileStateContext* Context = FileCtx.Get(); 
	if(Context)
	{
		DbgPrint("[*] file is monitored as it has a context for it\n");
		AutoLock<Mutex> Locker(Context->Lock);
		if (!Context->IsBacked)
		{
			DbgPrint("[*] file hasnt been backed yet , creating copy in backup folder\n");
			// get file name info
			FilterFileNameInformation fileNameInfo(Data);
			if (!fileNameInfo) {
				return FLT_PREOP_SUCCESS_NO_CALLBACK;
			}
			if (!NT_SUCCESS(fileNameInfo.Parse()))
			{
				return FLT_PREOP_SUCCESS_NO_CALLBACK;
			}
			// we need the name for the storing it in the backup directory later on 
			PUNICODE_STRING OriginalFilename = &fileNameInfo->FinalComponent;




			// get file size
			status = FltQueryInformationFile(FltObjects->Instance, FltObjects->FileObject, &FileInfo, sizeof(FileInfo), FileStandardInformation, NULL);
			if (!NT_SUCCESS(status))
			{
				return FLT_PREOP_SUCCESS_NO_CALLBACK;
			}
			if (&FltObjects->FileObject->FileName && FltObjects->FileObject)
			{
				DiskContent = FltAllocatePoolAlignedWithTag(FltObjects->Instance, NonPagedPool, FileInfo.EndOfFile.QuadPart, TAG);
				if (!DiskContent)
				{
					return FLT_PREOP_SUCCESS_NO_CALLBACK;
				}
				status = FltReadFile(FltObjects->Instance, FltObjects->FileObject, &ByteOffset, FileInfo.EndOfFile.QuadPart, DiskContent, FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET | FLTFL_IO_OPERATION_NON_CACHED, &BytesRead, NULL, NULL);
				if (!NT_SUCCESS(status))
				{
					FltFreePoolAlignedWithTag(FltObjects->Instance, DiskContent, TAG);
					return FLT_PREOP_SUCCESS_NO_CALLBACK;
				}
				DbgPrint("[*] read file content from disk\n");
				ULONG ContentSize = (SIZE_T)FileInfo.EndOfFile.QuadPart;
				BackupFile(OriginalFilename, DiskContent, ContentSize);
				if (NT_SUCCESS(status))
				{
					DbgPrint("[*] successfully backed up file!\n");
					Context->IsBacked = true;
				}
				FltFreePoolAlignedWithTag(FltObjects->Instance, DiskContent, TAG);

			}
		}
		else
		{
			DbgPrint("[*] file has already been backed\n");
		}

	}
    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}



// role : filter out files we dont care about and allocate initial context for file 
FLT_POSTOP_CALLBACK_STATUS
BackupFilterPostCreate(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
)

{
	UNREFERENCED_PARAMETER(CompletionContext);
	if (Flags & FLTFL_POST_OPERATION_DRAINING)
		return FLT_POSTOP_FINISHED_PROCESSING;

	const auto& params = Data->Iopb->Parameters.Create;
	if (Data->RequestorMode == KernelMode
		|| (params.SecurityContext->DesiredAccess & FILE_WRITE_DATA) == 0
		|| Data->IoStatus.Information == FILE_DOES_NOT_EXIST) {
		// kernel caller, not write access or a new file - we do not care 
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	// get file name info
	FilterFileNameInformation fileNameInfo(Data);
	if (!fileNameInfo) {
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	if (!NT_SUCCESS(fileNameInfo.Parse()))
		return FLT_POSTOP_FINISHED_PROCESSING;


	if (!IsBackupDirectory(&fileNameInfo->ParentDir))
		return FLT_POSTOP_FINISHED_PROCESSING;

	// if it's not the default stream, we don't care
	if (fileNameInfo->Stream.Length > 0)
		return FLT_POSTOP_FINISHED_PROCESSING;

	// if the file has a state already just return 
	FileContx FileCtx(FltObjects->Instance, FltObjects->FileObject);
	FileStateContext* CheckContext = FileCtx.Get();
	if (CheckContext)
	{
		AutoLock<Mutex> Locker(CheckContext->Lock);
		DbgPrint("[*] already allocated context for file :%wZ\n", CheckContext->FileName);
		return FLT_POSTOP_FINISHED_PROCESSING;
	}



	// allocate and initialize a file context if needed
	FileStateContext* context;

	 auto status = FltAllocateContext(FltObjects->Filter,
		FLT_FILE_CONTEXT, sizeof(FileStateContext), PagedPool,
		(PFLT_CONTEXT*)&context);
	if (!NT_SUCCESS(status)) {
		DbgPrint("[*] failed to allocate file state context\n");
		return FLT_POSTOP_FINISHED_PROCESSING;
	}
	// for clean unload we need to keep track of those contexts as we dont release them on cleanup/close
	SaveFileStateContextToDelete(context);
	context->IsBacked = FALSE;
	context->FileName.MaximumLength = fileNameInfo->Name.Length;
	context->FileName.Buffer = (WCHAR*)ExAllocatePoolWithTag(PagedPool, fileNameInfo->Name.Length, TAG);
	if (!context->FileName.Buffer) {
		FltReleaseContext(context);
		return FLT_POSTOP_FINISHED_PROCESSING;
	}
	RtlCopyUnicodeString(&context->FileName, &fileNameInfo->FinalComponent);
	DbgPrint("[*] built context for file :%wZ\n", context->FileName);
	context->Lock.Init();
	status = FltSetFileContext(FltObjects->Instance,
		FltObjects->FileObject,
		FLT_SET_CONTEXT_KEEP_IF_EXISTS,
		context, nullptr);
	if (!NT_SUCCESS(status)) {
		DbgPrint("[*] failed to set file context\n");
		ExFreePool(context->FileName.Buffer);
	}
	FltReleaseContext(context);


	DbgPrint("[*] allocated and set file context for file\n");
	return FLT_POSTOP_FINISHED_PROCESSING;


}

FLT_POSTOP_CALLBACK_STATUS
BackupFilterPostCleanup(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
)

{
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);
    UNREFERENCED_PARAMETER(Flags);



    return FLT_POSTOP_FINISHED_PROCESSING;
}

FLT_PREOP_CALLBACK_STATUS
BackupFilterPreOperationNoPostOperation(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
)

{
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);


    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

