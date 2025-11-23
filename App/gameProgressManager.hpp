#ifndef GAMEPROGRESSMANAGER_HPP
#define GAMEPROGRESSMANAGER_HPP

#include "gamePerkCardManager.hpp"

extern uint8_t             controlDelayTime; // 由 threads.cpp 定义
extern GameEntityManager   g_entityManager;
extern GamePerkCardManager g_perkCardManager;

class GameProgressManager {
public:
    uint8_t currentChapter = 1; // 当前游戏关卡
    uint8_t lastChapter    = 3; // 最大游戏关卡

    uint8_t currentWave         = 1;  // 当前波次
    uint8_t maxWave             = 15; // 最大波次
    uint8_t currentChapterWaves = 0;  // 当前关卡总波次

    uint8_t time_count = 0;

    bool     isPlayingOpeningCG = false; // 是否播放开场动画
    uint16_t openingCGTimer     = 0;     // 开场动画计时器

    uint8_t  showWhichBoss = 0;
    bool     showBoss      = false; // 是否 展示Boss海报
    uint16_t showBossTimer = 0;     // 展示Boss海报计时器
    //播放3秒Boss海报

public:
    GameProgressManager()  = default;
    ~GameProgressManager() = default;

    GameProgressManager(const GameProgressManager &)            = delete;
    GameProgressManager &operator=(const GameProgressManager &) = delete;

public:
    // 重置游戏进度
    void resetGameProgress() {
        // 重置实体管理器
        g_entityManager.isGameOver = false;
        g_entityManager.clearAllEntities();

        isPlayingOpeningCG = true;
        openingCGTimer     = 3000; // 3秒开场动画

        showBoss      = false;
        showBossTimer = 0;

        // 重置游戏进度数据
        currentChapter      = 1;
        currentWave         = 1;
        currentChapterWaves = 10 + rand() % 6; // 随机生成当前关卡波次，10~15波

        // 添加初始角色
        LeadingRole *player = new LeadingRole();
        g_entityManager.addRole(player);

        // 重置Perk卡片管理器
        g_perkCardManager.initWarehouse();
    }

    void drawOpeningCG() {
        if (openingCGTimer >= 2 * controlDelayTime)
            openingCGTimer -= 2 * controlDelayTime;
        else
            isPlayingOpeningCG = false;

        time_count += 2 * controlDelayTime;
        if (time_count < 200) return;
        time_count = 0;
    }


    void updateGameProgress() {
        
    }


    // 绘图展示功能

    // 绘制展示Boss海报
    void drawShowBoss() {
        if (showBossTimer >= 2 * controlDelayTime)
            showBossTimer -= 2 * controlDelayTime;
        else
            showBoss = false;

        switch (showWhichBoss) {
        case 1:
            OLED_PrintString(1, 28, "XIANGLIU", &font8x6, OLED_COLOR_NORMAL);
            break;

        case 2:
            OLED_PrintString(1, 28, "TAOTIE", &font8x6, OLED_COLOR_NORMAL);
            break;

        case 3:
            OLED_PrintString(1, 28, "TAOWU", &font8x6, OLED_COLOR_NORMAL);
            break;

        default:
            break;
        }
    }
};

#endif // GAMEPROGRESSMANAGER_HPP
