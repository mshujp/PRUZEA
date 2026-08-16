#include "SystemUI.h"

#include <stdio.h>

using namespace PRUZEA;

SystemUI::SystemUI(GetSystemInfoHandler infoHandler, void* infoContext)
    : getSystemInfoHandler(infoHandler),
      getSystemInfoContext(infoContext)
{
}

const char* SystemUI::getId() const
{
    return "pruzea_systemui";
}

const char* SystemUI::getName() const
{
    return "PRUZEA SystemUI";
}

const char* SystemUI::getMenuName() const
{
    return "PRUZEA SystemUI";
}

void SystemUI::setCatalog(GameCatalog* catalog)
{
    this->catalog = catalog;
    selectedIndex = 0;
    pageIndex = 0;
    selectedGameIndex = GameCatalog::INVALID_GAME_INDEX;
    currentGroupIndex = UINT16_MAX;
    uiDirty = true;
}

void SystemUI::setSystemInfoHandler(GetSystemInfoHandler infoHandler, void* infoContext)
{
    getSystemInfoHandler = infoHandler;
    getSystemInfoContext = infoContext;
    uiDirty = true;
    dirty = true;
}

void SystemUI::setDeleteGameDataHandler(DeleteGameDataHandler handler, void* context)
{
    deleteGameDataHandler = handler;
    deleteGameDataContext = context;
}

void SystemUI::drawStartupSplash(Graphics& graphics)
{
    drawSplash(graphics);
}

void SystemUI::completeStartupSplash()
{
    splashShownOnce = true;
    mode = MODE_SELECT;
    splashFrames = 0;
    uiDirty = true;
    dirty = true;
}

GameCatalog::GameIndex SystemUI::takeSelectedGameIndex()
{
    const GameCatalog::GameIndex result = selectedGameIndex;
    selectedGameIndex = GameCatalog::INVALID_GAME_INDEX;
    return result;
}

SystemUI::Mode SystemUI::getMode() const
{
    return mode;
}

void SystemUI::onInit(Storage& storage)
{
    mode = splashShownOnce ? MODE_SELECT : MODE_SPLASH;
    selectedIndex = 0;
    pageIndex = 0;
    splashFrames = 0;
    selectedGameIndex = GameCatalog::INVALID_GAME_INDEX;
    currentGroupIndex = UINT16_MAX;
    storageAvailable = storage.isAvailable();
    shutdownYesSelected = false;
    deleteGameDataYesSelected = false;
    uiDirty = true;
    dirty = true;
}

Game::GameState SystemUI::onUpdate(Input& input, Audio& audio, Storage& storage, float deltaSec)
{
    GameState result = GameState::RUNNING;

    if ((mode == MODE_SELECT || mode == MODE_INFO) &&
        input.justPressed(Input::Button::HOME))
    {
        shutdownYesSelected = false;
        mode = MODE_SHUTDOWN_CONFIRM;
        uiDirty = true;
        dirty = true;
        audio.playSE(&Audio::SE::NO_3, 0.8f);
        return result;
    }

    switch (mode)
    {
    case MODE_SPLASH:
        splashFrames++;
        uiDirty = true;

        if (splashFrames >= 24 ||
            input.justPressed(Input::Button::A) ||
            input.justPressed(Input::Button::B) ||
            input.justPressed(Input::Button::X) ||
            input.justPressed(Input::Button::Y) ||
            input.justPressed(Input::Button::START))
        {
            splashShownOnce = true;
            mode = MODE_SELECT;
            uiDirty = true;
            audio.playSE(&Audio::SE::NO_8, 0.7f);
        }
        break;

    case MODE_INFO:
        if (input.justPressed(Input::Button::SELECT) ||
            input.justPressed(Input::Button::A) ||
            input.justPressed(Input::Button::B) ||
            input.justPressed(Input::Button::X) ||
            input.justPressed(Input::Button::Y))
        {
            mode = MODE_SELECT;
            uiDirty = true;
            audio.playSE(&Audio::SE::NO_2, 0.8f);
        }
        break;

    case MODE_SHUTDOWN_CONFIRM:
    {
        if (input.justPressed(Input::Button::LEFT) ||
            input.justPressed(Input::Button::RIGHT))
        {
            shutdownYesSelected = !shutdownYesSelected;
            uiDirty = true;
            audio.playSE(&Audio::SE::NO_1, 0.5f);
        }

        if (input.justPressed(Input::Button::B) ||
            input.justPressed(Input::Button::HOME))
        {
            shutdownYesSelected = false;
            mode = MODE_SELECT;
            uiDirty = true;
            audio.playSE(&Audio::SE::NO_2, 0.8f);
            break;
        }

        if (input.justPressed(Input::Button::A) ||
            input.justPressed(Input::Button::START))
        {
            if (!shutdownYesSelected)
            {
                mode = MODE_SELECT;
                uiDirty = true;
                audio.playSE(&Audio::SE::NO_2, 0.8f);
                break;
            }
            return GameState::TERMINATE_REQUEST;
        }
        break;
    }

    case MODE_DELETE_GAME_DATA_CONFIRM:
    {
        if (input.justPressed(Input::Button::LEFT) ||
            input.justPressed(Input::Button::RIGHT))
        {
            deleteGameDataYesSelected = !deleteGameDataYesSelected;
            uiDirty = true;
            audio.playSE(&Audio::SE::NO_1, 0.5f);
        }

        if (input.justPressed(Input::Button::B) ||
            input.justPressed(Input::Button::HOME))
        {
            deleteGameDataYesSelected = false;
            mode = MODE_SELECT;
            uiDirty = true;
            audio.playSE(&Audio::SE::NO_2, 0.8f);
            break;
        }

        if (input.justPressed(Input::Button::A) ||
            input.justPressed(Input::Button::START))
        {
            bool deleted = false;
            if (deleteGameDataYesSelected)
            {
                const GameCatalog::GameIndex gameIndex = getSlotGameIndex(selectedIndex);
                const char* gameId = catalog == nullptr ? nullptr : catalog->getGameId(gameIndex);
                if (gameId != nullptr && deleteGameDataHandler != nullptr)
                {
                    deleted = deleteGameDataHandler(gameId, deleteGameDataContext);
                }
            }

            const bool confirmed = deleteGameDataYesSelected;
            deleteGameDataYesSelected = false;
            mode = MODE_SELECT;
            uiDirty = true;
            audio.playSE(confirmed && deleted ? &Audio::SE::NO_8 : &Audio::SE::NO_2, 0.8f);
            break;
        }
        break;
    }

    case MODE_SELECT:
    {
        static constexpr uint64_t DELETE_GAME_DATA_HOLD_MSEC = 2000;

        if (supportsGameDataDelete() &&
            storageAvailable &&
            getSlotGameIndex(selectedIndex) != GameCatalog::INVALID_GAME_INDEX &&
            input.pressed(Input::Button::Y) &&
            input.holdMillis(Input::Button::Y) >= DELETE_GAME_DATA_HOLD_MSEC)
        {
            deleteGameDataYesSelected = false;
            mode = MODE_DELETE_GAME_DATA_CONFIRM;
            uiDirty = true;
            audio.playSE(&Audio::SE::NO_3, 0.8f);
            break;
        }

        if (input.justPressed(Input::Button::B) && isInsideGroup())
        {
            returnToRootMenu();
            uiDirty = true;
            audio.playSE(&Audio::SE::NO_2, 0.8f);
            break;
        }

        if (input.justPressed(Input::Button::SELECT))
        {
            mode = MODE_INFO;
            uiDirty = true;
            audio.playSE(&Audio::SE::NO_3, 0.8f);
            break;
        }

        bool moved = false;

        if (input.justPressed(Input::Button::UP) || input.repeat(Input::Button::UP))
        {
            moveSelection(-1);
            moved = true;
        }
        else if (input.justPressed(Input::Button::DOWN) || input.repeat(Input::Button::DOWN))
        {
            moveSelection(1);
            moved = true;
        }
        else if (input.justPressed(Input::Button::LEFT) || input.justPressed(Input::Button::L))
        {
            movePage(-1);
            moved = true;
        }
        else if (input.justPressed(Input::Button::RIGHT) || input.justPressed(Input::Button::R))
        {
            movePage(1);
            moved = true;
        }

        if (moved)
        {
            uiDirty = true;
            audio.playSE(&Audio::SE::NO_1, 0.5f);
        }

        if (input.justPressed(Input::Button::A) || input.justPressed(Input::Button::START))
        {
            if (catalog != nullptr)
            {
                if (!isInsideGroup() && isSlotGroup(selectedIndex))
                {
                    currentGroupIndex = catalog->getRootItemGroupIndex(selectedIndex);
                    selectedIndex = 0;
                    pageIndex = 0;
                    audio.playSE(&Audio::SE::NO_8, 0.9f);
                    uiDirty = true;
                    break;
                }

                const GameCatalog::GameIndex gameIndex = getSlotGameIndex(selectedIndex);
                if (gameIndex != GameCatalog::INVALID_GAME_INDEX)
                {
                    selectedGameIndex = gameIndex;
                    audio.playSE(&Audio::SE::NO_8, 0.9f);
                    uiDirty = true;
                    break;
                }
            }

            audio.playSE(&Audio::SE::NO_2, 0.7f);
            uiDirty = true;
        }
        break;
    }

    case MODE_TERMINATED:
    {
        return Game::TERMINATED;
    }

    }

    if (uiDirty)
    {
        dirty = true;
    }

    return result;
}

bool SystemUI::onDraw(Graphics& graphics, bool requestFullRedraw)
{
    if (!requestFullRedraw && !dirty)
    {
        return false;
    }

    graphics.setViewport(0, 0);

    if (mode == MODE_SPLASH)
    {
        drawSplash(graphics);
    }
    else if (mode == MODE_INFO)
    {
        drawInfo(graphics);
    }
    else if (mode == MODE_SHUTDOWN_CONFIRM)
    {
        drawShutdownConfirm(graphics);
    }
    else if (mode == MODE_DELETE_GAME_DATA_CONFIRM)
    {
        drawDeleteGameDataConfirm(graphics);
    }
    else
    {
        drawSelect(graphics);
    }

    uiDirty = false;
    dirty = false;
    return true;
}

void SystemUI::onTerminate(Storage& storage)
{
    selectedGameIndex = GameCatalog::INVALID_GAME_INDEX;
    mode = Mode::MODE_TERMINATED;
}

void SystemUI::moveSelection(int8_t dy)
{
    const uint16_t count = getVisibleItemCount();
    if (count == 0) return;

    int32_t next = static_cast<int32_t>(selectedIndex) + static_cast<int32_t>(dy);

    if (next < 0)
    {
        next = count - 1;
    }
    else if (next >= count)
    {
        next = 0;
    }

    selectedIndex = static_cast<uint16_t>(next);
    pageIndex = selectedIndex / getItemsPerPage();
}

void SystemUI::movePage(int8_t direction)
{
    const uint16_t count = getVisibleItemCount();
    if (count == 0) return;

    const uint16_t pageCount = getPageCount();
    const uint16_t slot = selectedIndex % getItemsPerPage();

    int32_t nextPage = static_cast<int32_t>(pageIndex) + static_cast<int32_t>(direction);

    if (nextPage < 0)
    {
        nextPage = pageCount - 1;
    }
    else if (nextPage >= pageCount)
    {
        nextPage = 0;
    }

    pageIndex = static_cast<uint16_t>(nextPage);

    uint16_t nextIndex = pageIndex * getItemsPerPage() + slot;
    if (nextIndex >= count)
    {
        nextIndex = count - 1;
    }

    selectedIndex = nextIndex;
}

void SystemUI::returnToRootMenu()
{
    currentGroupIndex = UINT16_MAX;
    selectedIndex = 0;
    pageIndex = 0;
}

bool SystemUI::isInsideGroup() const
{
    return currentGroupIndex != UINT16_MAX;
}

uint16_t SystemUI::getVisibleItemCount() const
{
    if (catalog == nullptr) return 0;

    return isInsideGroup()
        ? catalog->getGroupGameCount(currentGroupIndex)
        : catalog->getRootItemCount();
}

bool SystemUI::isSlotAvailable(uint16_t index) const
{
    return index < getVisibleItemCount();
}

bool SystemUI::isSlotGroup(uint16_t index) const
{
    return catalog != nullptr &&
           !isInsideGroup() &&
           catalog->isRootItemGroup(index);
}

GameCatalog::GameIndex SystemUI::getSlotGameIndex(uint16_t index) const
{
    if (catalog == nullptr) return GameCatalog::INVALID_GAME_INDEX;

    return isInsideGroup()
        ? catalog->getGroupGameIndex(currentGroupIndex, index)
        : catalog->getRootItemGameIndex(index);
}

const char* SystemUI::getSlotName(uint16_t index) const
{
    if (!isSlotAvailable(index))
    {
        return "EMPTY";
    }

    if (isSlotGroup(index))
    {
        const char* groupName = catalog->getRootItemName(index);
        if (groupName == nullptr || groupName[0] == '\0')
        {
            groupName = "UNTITLED";
        }

        snprintf(slotNameBuffer, sizeof(slotNameBuffer), "[%s]", groupName);
        return slotNameBuffer;
    }

    const GameCatalog::GameIndex gameIndex = getSlotGameIndex(index);
    if (gameIndex == GameCatalog::INVALID_GAME_INDEX)
    {
        return "EMPTY";
    }

    const char* name = catalog->getGameMenuName(gameIndex);
    return (name == nullptr || name[0] == '\0') ? "UNTITLED" : name;
}

const char* SystemUI::getCurrentMenuTitle() const
{
    if (catalog == nullptr || !isInsideGroup())
    {
        return "PRUZEA";
    }

    const char* name = catalog->getGroupName(currentGroupIndex);
    return (name == nullptr || name[0] == '\0') ? "System" : name;
}

uint16_t SystemUI::getPageCount() const
{
    const uint16_t perPage = getItemsPerPage();
    if (perPage == 0) return 1;

    const uint16_t count = getVisibleItemCount();
    if (count == 0) return 1;

    return (count + perPage - 1) / perPage;
}
