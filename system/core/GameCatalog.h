#pragma once

#include "PRUZEA.h"
#include "GraphicsBase.h"
#include <new>
#include <string>
#include <vector>

namespace PRUZEA {

class GameCatalog
{
public:
    using GameIndex = uint16_t;
    static constexpr GameIndex INVALID_GAME_INDEX = UINT16_MAX;

private:
    using CreateGameHandler = Game* (*)();

    struct GameEntry
    {
        std::string id;
        std::string name;
        std::string menuName;
        std::string menuGroup;
        uint16_t targetWidth = 0;
        uint16_t targetHeight = 0;
        CreateGameHandler create = nullptr;
    };

    struct MenuGroup
    {
        std::string name;
        std::vector<GameIndex> games;
    };

    std::vector<GameEntry> games;

    // Menu data is built lazily on first access after registration.
    mutable bool menuBuilt = false;
    mutable std::vector<GameIndex> rootGames;
    mutable std::vector<MenuGroup> groups;

    void addGame(const Game& game, CreateGameHandler create, GraphicsBase& graphics);
    void ensureMenuBuilt() const;

    template<typename T>
    static Game* createGameInstance()
    {
        return new (std::nothrow) T();
    }

public:
    GameCatalog();

    template<typename T>
    void addGame(GraphicsBase& graphics)
    {
        T* game = new (std::nothrow) T();
        if (game == nullptr) return;

        addGame(*game, &GameCatalog::createGameInstance<T>, graphics);
        delete game;
    }

    Game* createGame(GameIndex index) const;
    const char* getGameId(GameIndex index) const;
    const char* getGameMenuName(GameIndex index) const;
    uint16_t getGameCount() const;

    // Two-level menu access. Root folders are always returned before root games.
    uint16_t getRootItemCount() const;
    bool isRootItemGroup(uint16_t index) const;
    const char* getRootItemName(uint16_t index) const;
    GameIndex getRootItemGameIndex(uint16_t index) const;
    uint16_t getRootItemGroupIndex(uint16_t index) const;

    uint16_t getGroupCount() const;
    const char* getGroupName(uint16_t groupIndex) const;
    uint16_t getGroupGameCount(uint16_t groupIndex) const;
    GameIndex getGroupGameIndex(uint16_t groupIndex, uint16_t gameIndex) const;
};

} // namespace PRUZEA
