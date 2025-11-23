#ifndef GAMEPROGRESSMANAGER_HPP
#define GAMEPROGRESSMANAGER_HPP

#include "gamePerkCardManager.hpp"
extern GameEntityManager   g_entityManager;
extern GamePerkCardManager g_perkCardManager;


class GameProgressManager {
public:
    uint8_t currentChapter = 1; // 当前游戏关卡
    uint8_t lastChapter     = 3; // 最大游戏关卡

    uint8_t currentWave = 1; // 当前波次
    uint8_t maxWave    = 15; // 最大波次
    uint8_t currentChapterWaves = 0 ; // 当前关卡总波次

public:
    GameProgressManager() = default;
    ~GameProgressManager() = default;

    GameProgressManager(const GameProgressManager &)            = delete;
    GameProgressManager &operator=(const GameProgressManager &) = delete;

public:
    // 重置游戏进度
    void resetGameProgress() {
        // 重置实体管理器
        g_entityManager.isGameOver = false;
        g_entityManager.clearAllEntities();

        // 重置游戏进度数据
        currentChapter = 1;
        currentWave    = 1;
        currentChapterWaves = 10+rand()%6 ; // 随机生成当前关卡波次，10~15波

        // 添加初始角色
        LeadingRole *player = new LeadingRole();
        g_entityManager.addRole(player);

        // 重置Perk卡片管理器
        g_perkCardManager.initWarehouse();
    }



};


#endif // GAMEPROGRESSMANAGER_HPP
