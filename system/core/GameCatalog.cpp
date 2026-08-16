#include "GameCatalog.h"

#include <algorithm>
#include <ctype.h>
#include <string.h>

using namespace PRUZEA;

namespace
{
    int compareAsciiIgnoreCase(const char* a, const char* b)
    {
        if (a == nullptr) a = "";
        if (b == nullptr) b = "";

        const char* originalA = a;
        const char* originalB = b;

        while (*a != '\0' && *b != '\0')
        {
            const unsigned char ca = static_cast<unsigned char>(
                tolower(static_cast<unsigned char>(*a)));
            const unsigned char cb = static_cast<unsigned char>(
                tolower(static_cast<unsigned char>(*b)));

            if (ca < cb) return -1;
            if (ca > cb) return 1;
            ++a;
            ++b;
        }

        if (*a == '\0' && *b == '\0')
        {
            // Deterministic order when names differ only by case.
            return strcmp(originalA, originalB);
        }
        return *a == '\0' ? -1 : 1;
    }

}

GameCatalog::GameCatalog()
{
    games.reserve(16);
    rootGames.reserve(16);
    groups.reserve(8);
}

void GameCatalog::addGame(const Game& game, CreateGameHandler create, GraphicsBase& graphics)
{
    if (create == nullptr || games.size() >= INVALID_GAME_INDEX) return;

    const uint16_t gameWidth  = game.getTargetScreenWidth();
    const uint16_t gameHeight = game.getTargetScreenHeight();

    const uint16_t screenWidth  = graphics.getScreenWidth();
    const uint16_t screenHeight = graphics.getScreenHeight();

    bool accept = false;

    switch (graphics.getCatalogFilterMode())
    {
    case GraphicsBase::CatalogFilterMode::ExactMatch:
        accept =
            (gameWidth  == screenWidth) &&
            (gameHeight == screenHeight);
        break;

    case GraphicsBase::CatalogFilterMode::FitInside:
        accept =
            (gameWidth  <= screenWidth) &&
            (gameHeight <= screenHeight);
        break;
    }

    if (!accept) return;

    GameEntry entry;
    const char* id = game.getId();
    const char* name = game.getName();
    const char* menuName = game.getMenuName();
    const char* menuGroup = game.getMenuGroup();
    entry.id = id == nullptr ? "" : id;
    entry.name = name == nullptr ? "" : name;
    entry.menuName = menuName == nullptr ? "" : menuName;
    entry.menuGroup = menuGroup == nullptr ? "" : menuGroup;
    entry.targetWidth = gameWidth;
    entry.targetHeight = gameHeight;
    entry.create = create;
    games.push_back(entry);
    menuBuilt = false;
}

void GameCatalog::ensureMenuBuilt() const
{
    if (menuBuilt)
    {
        return;
    }

    rootGames.clear();
    groups.clear();

    for (GameIndex gameIndex = 0; gameIndex < games.size(); ++gameIndex)
    {
        const GameEntry& game = games[gameIndex];

        if (game.menuGroup.empty())
        {
            rootGames.push_back(gameIndex);
            continue;
        }

        MenuGroup* target = nullptr;
        for (MenuGroup& group : groups)
        {
            if (compareAsciiIgnoreCase(group.name.c_str(), game.menuGroup.c_str()) == 0)
            {
                target = &group;
                break;
            }
        }

        if (target == nullptr)
        {
            groups.push_back(MenuGroup{});
            groups.back().name = game.menuGroup;
            target = &groups.back();
        }

        target->games.push_back(gameIndex);
    }

    const auto gameNameLess = [this](GameIndex a, GameIndex b)
    {
        return compareAsciiIgnoreCase(games[a].menuName.c_str(), games[b].menuName.c_str()) < 0;
    };

    std::sort(rootGames.begin(), rootGames.end(), gameNameLess);

    std::sort(groups.begin(), groups.end(),
        [](const MenuGroup& a, const MenuGroup& b)
        {
            return compareAsciiIgnoreCase(a.name.c_str(), b.name.c_str()) < 0;
        });

    for (MenuGroup& group : groups)
    {
        std::sort(group.games.begin(), group.games.end(), gameNameLess);
    }

    menuBuilt = true;
}

Game* GameCatalog::createGame(GameIndex index) const
{
    if (index >= games.size() || games[index].create == nullptr) return nullptr;
    return games[index].create();
}

const char* GameCatalog::getGameId(GameIndex index) const
{
    return index < games.size() ? games[index].id.c_str() : nullptr;
}

const char* GameCatalog::getGameMenuName(GameIndex index) const
{
    return index < games.size() ? games[index].menuName.c_str() : nullptr;
}

uint16_t GameCatalog::getGameCount() const
{
    return static_cast<uint16_t>(games.size());
}

uint16_t GameCatalog::getRootItemCount() const
{
    ensureMenuBuilt();
    const size_t count = groups.size() + rootGames.size();
    return count > UINT16_MAX ? UINT16_MAX : static_cast<uint16_t>(count);
}

bool GameCatalog::isRootItemGroup(uint16_t index) const
{
    ensureMenuBuilt();
    return index < groups.size();
}

const char* GameCatalog::getRootItemName(uint16_t index) const
{
    ensureMenuBuilt();

    if (index < groups.size())
    {
        return groups[index].name.c_str();
    }

    const size_t gameIndex = static_cast<size_t>(index) - groups.size();
    return gameIndex < rootGames.size() ? getGameMenuName(rootGames[gameIndex]) : nullptr;
}

GameCatalog::GameIndex GameCatalog::getRootItemGameIndex(uint16_t index) const
{
    ensureMenuBuilt();

    if (index < groups.size())
    {
        return INVALID_GAME_INDEX;
    }

    const size_t gameIndex = static_cast<size_t>(index) - groups.size();
    return gameIndex < rootGames.size() ? rootGames[gameIndex] : INVALID_GAME_INDEX;
}

uint16_t GameCatalog::getRootItemGroupIndex(uint16_t index) const
{
    ensureMenuBuilt();
    return index < groups.size() ? index : UINT16_MAX;
}

uint16_t GameCatalog::getGroupCount() const
{
    ensureMenuBuilt();
    return groups.size() > UINT16_MAX ? UINT16_MAX : static_cast<uint16_t>(groups.size());
}

const char* GameCatalog::getGroupName(uint16_t groupIndex) const
{
    ensureMenuBuilt();
    return groupIndex < groups.size() ? groups[groupIndex].name.c_str() : nullptr;
}

uint16_t GameCatalog::getGroupGameCount(uint16_t groupIndex) const
{
    ensureMenuBuilt();
    if (groupIndex >= groups.size()) return 0;

    const size_t count = groups[groupIndex].games.size();
    return count > UINT16_MAX ? UINT16_MAX : static_cast<uint16_t>(count);
}

GameCatalog::GameIndex GameCatalog::getGroupGameIndex(uint16_t groupIndex, uint16_t gameIndex) const
{
    ensureMenuBuilt();
    if (groupIndex >= groups.size()) return INVALID_GAME_INDEX;

    const std::vector<GameIndex>& groupGames = groups[groupIndex].games;
    return gameIndex < groupGames.size() ? groupGames[gameIndex] : INVALID_GAME_INDEX;
}
