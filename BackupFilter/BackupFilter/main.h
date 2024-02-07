#pragma once
#include <fltKernel.h>
#include <dontuse.h>
#include <filters.h>
#include <ntddk.h>
#include <mutex.h>
#include <filenameinfo.h>
#include <context.h>
#include <backup.h>
#include <new.h>
#include <autolock.h>
#include <filecontx.h>

#define TAG 'buft'


extern PFLT_FILTER gFilterHandle;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, BackupFilterUnload)
#pragma alloc_text(PAGE, BackupFilterInstanceQueryTeardown)
#pragma alloc_text(PAGE, BackupFilterInstanceSetup)
#pragma alloc_text(PAGE, BackupFilterInstanceTeardownStart)
#pragma alloc_text(PAGE, BackupFilterInstanceTeardownComplete)
#endif