#include <main.h>

PContextToDelete ContextsToDelete = NULL;


bool SaveFileStateContextToDelete(FileStateContext* context)
{
	AutoLock<Mutex> locker(FileStateContextsMutex);
	PContextToDelete New = (PContextToDelete)ExAllocatePoolWithTag(NonPagedPool, sizeof(ContextToDelete), TAG);
	if (!New)
		return false;
	New->Context = (DWORD64)context;
	New->Next = nullptr;
	if (!ContextsToDelete)
	{
		ContextsToDelete = New; 
		return true;
	}
	PContextToDelete current = ContextsToDelete;
	while (current->Next != nullptr)
	{
		current = (PContextToDelete)current->Next;
	}
	current->Next = (LIST_ENTRY*)New;
	
	return true;
}

void ReleaseFileStateContexts()
{
	AutoLock<Mutex> locker(FileStateContextsMutex);
	if (ContextsToDelete)
	{
		PContextToDelete current = ContextsToDelete;
		while (current != nullptr)
		{
			FltReleaseContext((PFLT_CONTEXT)current->Context); // that is -1 on the refcount to make it 0 
			ExFreePoolWithTag(current, TAG);
			current = (PContextToDelete)current->Next;
		}
	}

}