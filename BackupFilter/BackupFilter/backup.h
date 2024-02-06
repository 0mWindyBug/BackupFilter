#pragma once
#include <main.h>

#define BACKUP_DIR L"\\??\\C:\\users\\dorge\\onedrive\\pictures\\backup\\"


bool IsBackupDirectory(_In_ PCUNICODE_STRING directory);
NTSTATUS CreateBackupDirectory(_In_ PUNICODE_STRING directory);
NTSTATUS BackupFile(PUNICODE_STRING Name, PVOID Content, ULONG ContentSize);