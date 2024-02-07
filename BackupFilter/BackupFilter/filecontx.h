#pragma once
#include <main.h>



struct FileContx {
	FileContx(PFLT_INSTANCE Instance, PFILE_OBJECT FileObject);
	~FileContx();

	operator bool() const {
		return _context != nullptr;
	}

	FileStateContext* Get() const {
		return _context;
	}

	operator FileStateContext*() const {
		return Get();
	}

	FileStateContext* operator->() {
		return _context;
	}



private:
	FileStateContext* _context;
};