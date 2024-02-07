#pragma once
#include <main.h>

enum BackupControlCommands
{
	Restore,
	Unset
};

typedef struct _FilterBackupRequest {
    USHORT FileNameLength;
    WCHAR FileName[1];

}FilterBackupRequest, * PFilterBackupRequest;


bool SendControlRequest();