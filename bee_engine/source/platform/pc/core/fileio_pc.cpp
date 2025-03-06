#include <precompiled/engine_precompiled.hpp>

#include "core/fileio.hpp"
#include "tools/log.hpp"

using namespace bee;
using namespace std;

FileIO::FileIO()
{
	Paths[Directory::Asset] = "assets/";
}

FileIO::~FileIO() = default;
