#pragma once

#include "StorageBase.h"

namespace PRUZEA {

class StorageStub : public StorageBase
{
protected:
    StorageBaseFile* openWrite(const char* gameId, const char* fileName) override;
    bool onDeleteGameData(const char* gameId) override;

public:
    const char* getName() const override { return "NONE"; }
    bool begin() override;
    void end() override;
    bool isAvailable() const override { return false; }
    File* openRead(const char* path) override;
    File* openRead(const char* gameId, const char* fileName) override;
    bool directoryExists(const char* path) override;
    bool fileExists(const char* path) override;
};

} // namespace
