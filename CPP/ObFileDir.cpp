// Generated by OberonViewer 0.7.5 on 2020-01-27T01:00:45
// Then further developed manually by RK
#include "ObFileDir.h"
#include <memory>
#include <QtDebug>
using namespace Ob;

static owned_ptr<FileDir> s_inst;

const int FileDir::FnLength;
const int FileDir::SectorSize;
const int FileDir::HeaderSize;

FileDir* FileDir::_inst()
{
	if( s_inst.get() == 0 )
		s_inst.reset( new FileDir() );
	return s_inst.get();
}

/* first page of each file on disk */
void FileDir::Enumerate(_ValArray<char> prefix, EntryHandler proc)
{
    qWarning() << "FileDir::Enumerate not implemented" << prefix.data();
}

FileDir::FileDir()
{
}

FileDir::~FileDir()
{
	s_inst.release();
}
