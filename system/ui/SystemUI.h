#pragma once

#include "PRUZEA.h"
#include "GameCatalog.h"

namespace PRUZEA
{

class SystemUI : public Game
{
public:
    enum Mode : uint8_t
    {
        MODE_SPLASH,
        MODE_SELECT,
        MODE_INFO,
        MODE_SHUTDOWN_CONFIRM,
        MODE_DELETE_GAME_DATA_CONFIRM,
        MODE_TERMINATED
    };

    struct SystemInfo
    {
        char version[16];
        char display[16];
        char input[16];
        char audio[16];
        char storage[16];
        char battery[16];
    };

    using GetSystemInfoHandler = void(*)(SystemInfo& info, void* context);
    using DeleteGameDataHandler = bool(*)(const char* gameId, void* context);

protected:
    GameCatalog* catalog = nullptr;
    Mode mode = MODE_SELECT;

    uint16_t selectedIndex = 0;
    uint16_t pageIndex = 0;
    uint16_t currentGroupIndex = UINT16_MAX;
    uint8_t splashFrames = 0;

    GameCatalog::GameIndex selectedGameIndex = GameCatalog::INVALID_GAME_INDEX;

    bool splashShownOnce = false;
    bool storageAvailable = false;
    bool uiDirty = true;
    bool shutdownYesSelected = false;
    bool deleteGameDataYesSelected = false;

    mutable char slotNameBuffer[64] = {};

    GetSystemInfoHandler getSystemInfoHandler = nullptr;
    void* getSystemInfoContext = nullptr;
    DeleteGameDataHandler deleteGameDataHandler = nullptr;
    void* deleteGameDataContext = nullptr;

    virtual uint16_t getItemsPerPage() const = 0;

    virtual void drawSplash(Graphics& graphics) = 0;
    virtual void drawSelect(Graphics& graphics) = 0;
    virtual void drawInfo(Graphics& graphics) = 0;
    virtual void drawShutdownConfirm(Graphics& graphics) = 0;

    virtual bool supportsGameDataDelete() const { return false; }
    virtual void drawDeleteGameDataConfirm(Graphics& graphics) { (void)graphics; }

    void moveSelection(int8_t dy);
    void movePage(int8_t direction);
    void returnToRootMenu();

    bool isInsideGroup() const;
    uint16_t getVisibleItemCount() const;
    bool isSlotAvailable(uint16_t index) const;
    bool isSlotGroup(uint16_t index) const;
    GameCatalog::GameIndex getSlotGameIndex(uint16_t index) const;
    const char* getSlotName(uint16_t index) const;
    const char* getCurrentMenuTitle() const;
    uint16_t getPageCount() const;

public:
    explicit SystemUI(GetSystemInfoHandler infoHandler = nullptr, void* infoContext = nullptr);

    const char* getId() const override;
    const char* getName() const override;
    const char* getMenuName() const override;

    void setCatalog(GameCatalog* catalog);
    void setSystemInfoHandler(GetSystemInfoHandler infoHandler, void* infoContext);
    void setDeleteGameDataHandler(DeleteGameDataHandler handler, void* context);
    void drawStartupSplash(Graphics& graphics);
    void completeStartupSplash();
    GameCatalog::GameIndex takeSelectedGameIndex();

    Mode getMode() const;

    void onInit(Storage& storage) override;
    GameState onUpdate(Input& input, Audio& audio, Storage& storage, float deltaSec) override;
    bool onDraw(Graphics& graphics, bool requestFullRedraw) override;
    virtual void drawLowBttery(Graphics& graphics) = 0;
    virtual void drawPowerOffReady(Graphics& graphics) = 0;
    void onTerminate(Storage& storage) override;
};

} // namespace PRUZEA
