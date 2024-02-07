#include <main.h>


FileContx::FileContx(PFLT_INSTANCE Instance, PFILE_OBJECT FileObject)
{
	auto status = FltGetFileContext(Instance, FileObject, (PFLT_CONTEXT*)&_context);
	if (!NT_SUCCESS(status))
		_context = nullptr;
}


FileContx::~FileContx()
{
	if (_context)
		FltReleaseContext(_context);
}