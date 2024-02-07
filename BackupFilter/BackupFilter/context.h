#pragma once
#include <main.h>

extern Mutex FileStateContextsMutex;

typedef struct FileStateContext
{
	Mutex Lock;
	UNICODE_STRING FileName;
	BOOLEAN IsBacked;
};

typedef struct _ContextToDelete
{
	UINT64 Context;
	LIST_ENTRY * Next;
}ContextToDelete, * PContextToDelete;


bool SaveFileStateContextToDelete(FileStateContext* context);
void ReleaseFileStateContexts();
bool UnsetFileRestorePoint(PUNICODE_STRING FileName);
