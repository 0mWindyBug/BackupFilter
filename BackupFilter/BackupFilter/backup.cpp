#include <main.h>

bool IsBackupDirectory(_In_ PCUNICODE_STRING directory) {
	// no counted version of wcsstr :(

	ULONG maxSize = 1024;
	if (directory->Length > maxSize)
		return false;

	auto copy = (WCHAR*)ExAllocatePoolWithTag(PagedPool, maxSize + sizeof(WCHAR), TAG);
	if (!copy)
		return false;

	RtlZeroMemory(copy, maxSize + sizeof(WCHAR));
	wcsncpy_s(copy, 1 + maxSize / sizeof(WCHAR), directory->Buffer, directory->Length / sizeof(WCHAR));
	_wcslwr(copy);
	bool doBackup =  wcsstr(copy, L"\\users\\dorge\\onedrive\\pictures\\");
	ExFreePool(copy);

	return doBackup;
}




NTSTATUS CreateBackupDirectory(PUNICODE_STRING directory)
{
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    HANDLE directoryHandle;
    NTSTATUS status;
    // Initialize object attributes
    InitializeObjectAttributes(&objectAttributes, directory, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    // Create the directory 
    status = ZwCreateFile(&directoryHandle,
        FILE_LIST_DIRECTORY | SYNCHRONIZE,
        &objectAttributes,
        &ioStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN_IF,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0);

    if (!NT_SUCCESS(status))
    {

        return status;
    }
    ZwClose(directoryHandle);
    return STATUS_SUCCESS;
}


NTSTATUS BackupFile(PUNICODE_STRING Name, PVOID Content, ULONG ContentSize)
{
    NTSTATUS status;
    UNICODE_STRING targetFilePath;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE fileHandle = NULL;
    IO_STATUS_BLOCK ioStatusBlock;

    // Construct the target file path by concatenating the backup directory path and the file name
    targetFilePath.MaximumLength = Name->Length + wcslen(BACKUP_DIR) * sizeof(WCHAR);
    targetFilePath.Length = 0;
    targetFilePath.Buffer = (WCHAR*)ExAllocatePoolWithTag(PagedPool, targetFilePath.MaximumLength, TAG);
    if (!targetFilePath.Buffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Copy the backup directory path to the target file path buffe
    UNICODE_STRING BackupDir = RTL_CONSTANT_STRING(BACKUP_DIR);
    RtlCopyUnicodeString(&targetFilePath, &BackupDir);

    // Concatenate the file name to the target file path
    RtlAppendUnicodeStringToString(&targetFilePath, Name);

    // Initialize object attributes for the target file
    InitializeObjectAttributes(&objectAttributes, &targetFilePath, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

    // Create or open the target file for writing
    status = ZwCreateFile(&fileHandle,
        FILE_GENERIC_WRITE | SYNCHRONIZE,
        &objectAttributes,
        &ioStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        0,
        FILE_OVERWRITE_IF,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0);
    if (!NT_SUCCESS(status)) {
        ExFreePoolWithTag(targetFilePath.Buffer, TAG);
        return status;
    }

    // Write the content to the target file
    status = ZwWriteFile(fileHandle,
        NULL,
        NULL,
        NULL,
        &ioStatusBlock,
        Content,
        ContentSize,
        NULL,
        NULL);
    if (!NT_SUCCESS(status)) {
        ZwClose(fileHandle);
        ExFreePoolWithTag(targetFilePath.Buffer, TAG);
        return status;
    }

    // Close the target file handle
    ZwClose(fileHandle);

    // Free the allocated target file path buffer
    ExFreePoolWithTag(targetFilePath.Buffer, TAG);

    return STATUS_SUCCESS;
}

