#include "StorageStub.h"

using namespace PRUZEA;

bool StorageStub::begin()
{
    return true;
}

void StorageStub::end()
{
}

Storage::File* StorageStub::openRead(const char* path)
{
    return nullptr;
}

Storage::File* StorageStub::openRead(const char* gameId, const char* fileName)
{
    return nullptr;
}

StorageBaseFile* StorageStub::openWrite(const char* gameId, const char* fileName)
{
    return nullptr;
}

bool StorageStub::onDeleteGameData(const char* gameId)
{
    return false;
}

bool StorageStub::directoryExists(const char* path)
{
    return false;
}

bool StorageStub::fileExists(const char* path)
{
    return false;
}
