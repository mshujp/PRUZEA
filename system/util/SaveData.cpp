#include "PRUZEA.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace PRUZEA
{

class SaveDataImpl
{
public:
    struct Entry
    {
        std::string key;
        std::string value;
    };

    std::vector<Entry> entries;
    bool dirty = false;

    Entry* find(const char* key)
    {
        if (key == nullptr) return nullptr;
        for (Entry& entry : entries)
        {
            if (entry.key == key) return &entry;
        }
        return nullptr;
    }

    const Entry* find(const char* key) const
    {
        if (key == nullptr) return nullptr;
        for (const Entry& entry : entries)
        {
            if (entry.key == key) return &entry;
        }
        return nullptr;
    }

    void release()
    {
        std::vector<Entry>().swap(entries);
    }
};

namespace
{

bool isValidKey(const char* key)
{
    if (key == nullptr || key[0] == '\0') return false;
    for (const char* p = key; *p != '\0'; ++p)
    {
        if (*p == '=' || *p == '\n' || *p == '\r') return false;
    }
    return true;
}

bool isValidValue(const char* value)
{
    if (value == nullptr) return false;
    for (const char* p = value; *p != '\0'; ++p)
    {
        if (*p == '\n' || *p == '\r') return false;
    }
    return true;
}

struct LoadContext
{
    SaveData* self;
    bool failed;
};

bool loadLineCallback(const char* line, void* arg)
{
    LoadContext* context = static_cast<LoadContext*>(arg);
    if (line == nullptr) return true;

    std::string text(line);
    const size_t separator = text.find('=');
    if (separator == std::string::npos || separator == 0) return true;

    std::string key = text.substr(0, separator);
    std::string value = text.substr(separator + 1);
    while (!value.empty() && (value.back() == '\r' || value.back() == '\n')) value.pop_back();

    if (!context->self->setString(key.c_str(), value.c_str()))
    {
        context->failed = true;
        return false;
    }
    return true;
}

struct SaveContext
{
    const SaveDataImpl* impl;
    size_t cursor;
};

bool writeLineCallback(std::string& line, void* arg)
{
    SaveContext* context = static_cast<SaveContext*>(arg);
    if (context->cursor >= context->impl->entries.size()) return false;

    const SaveDataImpl::Entry& entry = context->impl->entries[context->cursor++];
    line = entry.key;
    line += '=';
    line += entry.value;
    return true;
}

} // namespace

SaveData::SaveData() : impl(new SaveDataImpl())
{
}

SaveData::~SaveData()
{
    delete static_cast<SaveDataImpl*>(impl);
}

void SaveData::clear()
{
    SaveDataImpl* data = static_cast<SaveDataImpl*>(impl);
    if (data == nullptr) return;
    data->release();
    data->dirty = true;
}

bool SaveData::contains(const char* key) const
{
    const SaveDataImpl* data = static_cast<const SaveDataImpl*>(impl);
    return data != nullptr && data->find(key) != nullptr;
}

bool SaveData::remove(const char* key)
{
    SaveDataImpl* data = static_cast<SaveDataImpl*>(impl);
    if (data == nullptr || key == nullptr) return false;

    for (auto it = data->entries.begin(); it != data->entries.end(); ++it)
    {
        if (it->key != key) continue;
        data->entries.erase(it);
        data->dirty = true;
        return true;
    }
    return false;
}

bool SaveData::getString(const char* key, char* outValue, size_t outSize, const char* defaultValue) const
{
    if (outValue == nullptr || outSize == 0) return false;

    const SaveDataImpl* data = static_cast<const SaveDataImpl*>(impl);
    const SaveDataImpl::Entry* entry = data != nullptr ? data->find(key) : nullptr;
    const char* source = entry != nullptr ? entry->value.c_str() : (defaultValue != nullptr ? defaultValue : "");

    size_t index = 0;
    for (; index < outSize - 1 && source[index] != '\0'; ++index) outValue[index] = source[index];
    const bool fits = source[index] == '\0';
    outValue[index] = '\0';
    return entry != nullptr && fits;
}

int32_t SaveData::getInt32(const char* key, int32_t defaultValue) const
{
    const SaveDataImpl* data = static_cast<const SaveDataImpl*>(impl);
    const SaveDataImpl::Entry* entry = data != nullptr ? data->find(key) : nullptr;
    if (entry == nullptr || entry->value.empty()) return defaultValue;

    char* end = nullptr;
    const long long result = std::strtoll(entry->value.c_str(), &end, 10);
    if (end == entry->value.c_str() || *end != '\0' || result < INT32_MIN || result > INT32_MAX) return defaultValue;
    return static_cast<int32_t>(result);
}

uint32_t SaveData::getUInt32(const char* key, uint32_t defaultValue) const
{
    const SaveDataImpl* data = static_cast<const SaveDataImpl*>(impl);
    const SaveDataImpl::Entry* entry = data != nullptr ? data->find(key) : nullptr;
    if (entry == nullptr || entry->value.empty() || entry->value[0] == '-') return defaultValue;

    char* end = nullptr;
    const unsigned long long result = std::strtoull(entry->value.c_str(), &end, 10);
    if (end == entry->value.c_str() || *end != '\0' || result > UINT32_MAX) return defaultValue;
    return static_cast<uint32_t>(result);
}

bool SaveData::getBool(const char* key, bool defaultValue) const
{
    const SaveDataImpl* data = static_cast<const SaveDataImpl*>(impl);
    const SaveDataImpl::Entry* entry = data != nullptr ? data->find(key) : nullptr;
    if (entry == nullptr) return defaultValue;
    if (entry->value == "1") return true;
    if (entry->value == "0") return false;
    return defaultValue;
}

bool SaveData::setString(const char* key, const char* value)
{
    if (!isValidKey(key) || !isValidValue(value)) return false;

    SaveDataImpl* data = static_cast<SaveDataImpl*>(impl);
    if (data == nullptr) return false;

    SaveDataImpl::Entry* entry = data->find(key);
    if (entry != nullptr)
    {
        if (entry->value == value) return true;
        entry->value = value;
    }
    else
    {
        data->entries.push_back({key, value});
    }
    data->dirty = true;
    return true;
}

bool SaveData::setInt32(const char* key, int32_t value)
{
    char text[16];
    std::snprintf(text, sizeof(text), "%ld", static_cast<long>(value));
    return setString(key, text);
}

bool SaveData::setUInt32(const char* key, uint32_t value)
{
    char text[16];
    std::snprintf(text, sizeof(text), "%lu", static_cast<unsigned long>(value));
    return setString(key, text);
}

bool SaveData::setBool(const char* key, bool value)
{
    return setString(key, value ? "1" : "0");
}

bool SaveData::isDirty() const
{
    const SaveDataImpl* data = static_cast<const SaveDataImpl*>(impl);
    return data != nullptr && data->dirty;
}

bool SaveData::load(Storage& storage, const char* gameId, const char* fileName)
{
    SaveDataImpl* data = static_cast<SaveDataImpl*>(impl);
    if (data == nullptr) return false;

    data->entries.clear();
    data->dirty = false;
    if (!storage.isAvailable()) return false;

    LoadContext context{this, false};
    const bool result = storage.readUserFile(gameId, fileName, &loadLineCallback, &context);
    if (!result && context.failed)
    {
        data->dirty = false;
        return false;
    }
    if (!result) data->entries.clear();
    data->dirty = false;
    return true;
}

bool SaveData::save(Storage& storage, const char* gameId, const char* fileName)
{
    SaveDataImpl* data = static_cast<SaveDataImpl*>(impl);
    if (data == nullptr) return false;
    if (!data->dirty) return true;
    if (!storage.isAvailable()) return false;

    SaveContext context{data, 0};
    const bool result = storage.writeUserFile(gameId, fileName, &writeLineCallback, &context);
    if (result) data->dirty = false;
    return result;
}

} // namespace PRUZEA
