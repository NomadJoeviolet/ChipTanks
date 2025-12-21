#ifndef GAMEPROGRESSMANAGER_HPP
#define GAMEPROGRESSMANAGER_HPP

#include "gamePerkCardManager.hpp"

extern uint8_t             controlDelayTime; // 由 threads.cpp 定义
extern GameEntityManager   g_entityManager;
extern GamePerkCardManager g_perkCardManager;

enum class WaveType {
    //魑魅阵型
    CHIMEI_LINE = 0, // 10 魑魅直线阵
    CHIMEI_TRIANGLE, // 10 魑魅三角阵

    //飞廉阵型
    THREE_Feilian,   // 三飞廉
    FEILIAN_CLUSTER, // 飞廉群

    //古雕阵型
    GUDIAO_SINGLE,
    GUODIAO_DOUBLE,
    GUDIAO_SQUARE,

    //混合阵型
    MIXED_SMALL,  // 混合小型阵型 2feilian + 1Gudiao
    MIXED_MEDIUM, // 混合中型阵型 5feilian +1Gudiao
    MIXED_LARGE,  // 混合大型阵型 5feilian + 2Gudiao
};

enum class BOSS_TYPE {
    NONE = 0,
    TAO_TIE,   // 饕餮
    XIANG_LIU, // 相柳
    TAO_WU,    // 梼杌
};

class GameProgressManager {
public:
    uint8_t currentChapter = 1; // 当前游戏关卡
    uint8_t lastChapter    = 4; // 最大游戏关卡

    uint8_t currentWave            = 1;  // 当前波次
    uint8_t maxWave                = 15; // 最大波次
    uint8_t currentChapterMaxWaves = 0;  // 当前关卡总波次

    uint8_t time_count = 0;

    bool     isPlayingOpeningCG = false; // 是否播放开场动画
    uint16_t openingCGTimer     = 0;     // 开场动画计时器

    bool     isPlayingClearCG = false; // 是否播放通关动画
    uint16_t clearCGTimer     = 0;     // 通关动画计时器

    bool chatpter4Warning = false; // 第四章警告标记
    BOSS_TYPE showWhichBoss = BOSS_TYPE::NONE; // 展示Boss类型
    bool      showBoss      = false;           // 是否 展示Boss海报
    uint16_t  showBossTimer = 0;               // 展示Boss海报计时器
    //播放3秒Boss海报

    uint16_t PauseTimer = 0;
    bool PauseGame = false; // 暂停游戏标记

    bool isWaitingStartKey = false; // 等待按键开始游戏

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

        // 重置暂停状态
        PauseGame = false;

        // 重置开场动画状态
        isPlayingOpeningCG = true;
        openingCGTimer     = 2000; // 2秒开场动画
        isWaitingStartKey  = false; // 动画播放完后会设为true

        // 通关动画
        isPlayingClearCG = false;
        clearCGTimer     = 0;

        // 重置展示Boss海报状态
        showBoss      = false;
        showBossTimer = 0;

        // 重置游戏进度数据
        currentChapter         = 1;

        //测试用
        // currentChapter         = 4; // 测试时直接从第4关开始

        // 重置当前波次
        currentWave            = 0;
        currentChapterMaxWaves = 8 + rand() % 4; // 随机生成当前关卡波次，8~11波

        //测试用
        // currentChapterMaxWaves = 1; // 测试时每关只1波

        // 添加初始角色
        LeadingRole *player = new LeadingRole();
        g_entityManager.addRole(player);

        // 重置Perk卡片管理器
        g_perkCardManager.initWarehouse();
    }

    // 更新游戏进度
    void updateGameProgress() {
        if (g_entityManager.getPlayerRole() != nullptr && g_entityManager.m_roles.size() == 1) {
            // 当前场上只有玩家角色，说明本波敌人已被清除，准备进入下一波
            currentWave++;
            if (currentWave > currentChapterMaxWaves) {
                // 当前关卡所有波次已完成，进入下一关卡
                currentChapter++;
                g_perkCardManager.triggerPerkSelection(); // 触发选卡机制
                if (currentChapter > lastChapter) {
                    // 已完成所有关卡，游戏胜利，重置游戏
                    isPlayingClearCG = true; // 播放通关动画
                    clearCGTimer     = 2000; // 通关动画持续时间
                    return;
                }

                // 重置波次数据
                if (currentChapter <= 3) {
                    currentWave            = 1;
                    currentChapterMaxWaves = 8 + rand() % 4; // 随机生成当前关卡波次，8~11波
                } else {
                    currentWave            = 1;
                    currentChapterMaxWaves = 1;
                }
            }

            if (currentWave == currentChapterMaxWaves / 2 && currentChapter != 4) {
                g_perkCardManager.triggerPerkSelection(); // 触发选卡机制
            }
            // 添加新一波敌人角色
            AddWaveEnemies();
        }
    }

    void AddWaveEnemies() {
        // 添加新一波敌人角色
        // 根据当前关卡和波次，生成不同类型和数量的敌人
        if (currentChapter == 1 && currentWave == currentChapterMaxWaves) {
            // 第一关最后一波，添加Boss饕餮
            IRole *enemyTaotie = new TaotieEnemy(156, 1 , 64 , 1 , currentChapter, 100 + rand() % 21);
            g_entityManager.addRole(enemyTaotie);

            // 标记展示Boss海报
            showWhichBoss = BOSS_TYPE::TAO_TIE; // 饕餮Boss
            showBoss      = true;
            showBossTimer = 1000; // 播放1秒Boss海报
            return;
        }
        if (currentChapter == 2 && currentWave == currentChapterMaxWaves) {
            // 第二关最后一波，添加Boss相柳
            IRole *enemyXiangliu = new XiangliuEnemy(156, 1 , 64, 1 , currentChapter, 130 + rand() % 21);
            g_entityManager.addRole(enemyXiangliu);

            // 标记展示Boss海报
            showWhichBoss = BOSS_TYPE::XIANG_LIU; // 相柳Boss
            showBoss      = true;
            showBossTimer = 1000; // 播放1秒Boss海报
            return;
        }
        if (currentChapter == 3 && currentWave == currentChapterMaxWaves) {
            // 第三关最后一波，添加Boss梼杌
            IRole *enemyTaowu = new TaowuEnemy(156, 1 , 64, 1 , currentChapter, 160 + rand() % 21);
            g_entityManager.addRole(enemyTaowu);

            // 标记展示Boss海报
            showWhichBoss = BOSS_TYPE::TAO_WU; // 梼杌Boss
            showBoss      = true;
            showBossTimer = 1000; // 播放1秒Boss海报
            return;
        }
        if (currentChapter == 4) {
            chatpter4Warning = true;
            // 第四关，挑战关卡，随意一个高数值BOSS
            BOSS_TYPE bossType = static_cast<BOSS_TYPE>(rand() % 3 + 1);
            switch (bossType) {
            case BOSS_TYPE::TAO_TIE:
                {
                    IRole *enemyTaotie = new TaotieEnemy(156, 1 , 64,1 , currentChapter + 2 , 200 + rand() % 51);
                    g_entityManager.addRole(enemyTaotie);

                    // 标记展示Boss海报
                    showWhichBoss = BOSS_TYPE::TAO_TIE; // 饕餮Boss
                    showBoss      = true;
                    showBossTimer = 1000; // 播放1秒Boss海报
                    break;
                }
            case BOSS_TYPE::XIANG_LIU:
                {
                    IRole *enemyXiangliu = new XiangliuEnemy(156, 1, 64, 1, currentChapter + 2 , 230 + rand() % 51);
                    g_entityManager.addRole(enemyXiangliu);

                    // 标记展示Boss海报
                    showWhichBoss = BOSS_TYPE::XIANG_LIU; // 相柳Boss
                    showBoss      = true;
                    showBossTimer = 1000; // 播放1秒Boss海报
                    break;
                }
            case BOSS_TYPE::TAO_WU:
                {
                    IRole *enemyTaowu = new TaowuEnemy(156, 1 , 64, 1 , currentChapter + 2 , 260 + rand() % 51);
                    g_entityManager.addRole(enemyTaowu);

                    // 标记展示Boss海报
                    showWhichBoss = BOSS_TYPE::TAO_WU; // 梼杌Boss
                    showBoss      = true;
                    showBossTimer = 1000; // 播放1秒Boss海报
                    break;
                }
            default:
                break;
            }

            return;
        }

        WaveType enemyType = static_cast<WaveType>(rand() % 10); // 随机选择敌人类型

        // 根据关卡调整敌人类型概率
        if (currentChapter == 1
            && (enemyType == WaveType::GUDIAO_SQUARE || enemyType == WaveType::MIXED_MEDIUM
                || enemyType == WaveType::MIXED_LARGE || enemyType == WaveType::FEILIAN_CLUSTER)) {
            enemyType = WaveType::THREE_Feilian; // 第一关避免混合阵型
        }
        if (currentChapter == 3 && (enemyType == WaveType::THREE_Feilian)) {
            enemyType = WaveType::MIXED_LARGE; // 第三关避免简单的飞廉阵型
        }

        switch (enemyType) {
        case WaveType::CHIMEI_LINE:
            // 添加魑魅直线阵
            for (int i = 0; i < 6; ++i) {
                IRole *enemyChiMei1 = new ChiMeiEnemy(124, i * 10 + 1, 90, i * 10 + 1, currentChapter, 6 + rand() % 3);
                g_entityManager.addRole(enemyChiMei1);
                IRole *enemyChiMei2 =
                    new ChiMeiEnemy(124 + 20, i * 10 + 1, 90, i * 10 + 1, currentChapter, 6 + rand() % 3);
                g_entityManager.addRole(enemyChiMei2);
            }
            break;
        case WaveType::CHIMEI_TRIANGLE:
            // 添加魑魅三角阵
            for (int i = 0; i < 6; ++i) {
                IRole *enemyChiMei = new ChiMeiEnemy(
                    124 + (i / 5) * 20, (i % 5) * 12 + 1, 90 + (i / 5) * 10, (i % 5) * 12 + 1, currentChapter,
                    6 + rand() % 3
                );
                g_entityManager.addRole(enemyChiMei);
            }
            break;
        case WaveType::THREE_Feilian:
            // 添加三飞廉阵
            for (int i = 0; i < 3; ++i) {
                IRole *enemyFeilian = new FeilianEnemy(140, i * 20 + 1, 90, i * 20 + 1, currentChapter, 15 + rand() % 6);
                g_entityManager.addRole(enemyFeilian);
            }
            break;
        case WaveType::FEILIAN_CLUSTER:
            // 添加飞廉群阵
            for (int i = 0; i < 6 + rand() % 7; ++i) {
                IRole *enemyFeilian = new FeilianEnemy(
                    140 + (i / 5) * 20, (i % 5) * 12 + 1, 90 + (i / 5) * 10, (i % 5) * 12 + 1, currentChapter,
                    15 + rand() % 6
                );
                g_entityManager.addRole(enemyFeilian);
            }
            break;
        case WaveType::GUDIAO_SINGLE:
            // 添加单个古雕
            {
                IRole *enemyGudiao = new GudiaoEnemy(156, 32, 100, 26, currentChapter, 35 + rand() % 11);
                g_entityManager.addRole(enemyGudiao);
            }
            break;
        case WaveType::GUODIAO_DOUBLE:
            // 添加双古雕
            for (int i = 0; i < 2; ++i) {
                IRole *enemyGudiao =
                    new GudiaoEnemy(156, i * 40 + 1, 100, i * 40 + 1, currentChapter, 35 + rand() % 11);
                g_entityManager.addRole(enemyGudiao);
            }
            break;
        case WaveType::GUDIAO_SQUARE:
            // 添加古雕方阵
            for (int i = 0; i < 4; ++i) {
                IRole *enemyGudiao = new GudiaoEnemy(
                    156 + (i / 2) * 20, (i % 2) * 40 + 1, 100 + (i / 2) * 10, (i % 2) * 40 + 1, currentChapter,
                    35 + rand() % 11
                );
                g_entityManager.addRole(enemyGudiao);
            }
            break;
        case WaveType::MIXED_SMALL:
            // 添加混合小型阵型 2feilian + 1Gudiao
            for (int i = 0; i < 2; ++i) {
                IRole *enemyFeilian = new FeilianEnemy(140, i * 30 + 1, 90, i * 30 + 1, currentChapter, 7 + rand() % 6);
                g_entityManager.addRole(enemyFeilian);
            }
            {
                IRole *enemyGudiao = new GudiaoEnemy(156, 32, 100, 26, currentChapter, 30 + rand() % 11);
                g_entityManager.addRole(enemyGudiao);
            }
            break;
        case WaveType::MIXED_MEDIUM:
            // 添加混合中型阵型 5feilian +1Gudiao
            for (int i = 0; i < 5; ++i) {
                IRole *enemyFeilian = new FeilianEnemy(
                    140 + (i / 3) * 20, (i % 3) * 20 + 1, 90 + (i / 3) * 10, (i % 3) * 20 + 1, currentChapter,
                    7 + rand() % 6
                );
                g_entityManager.addRole(enemyFeilian);
            }
            {
                IRole *enemyGudiao = new GudiaoEnemy(156, 32, 100, 26, currentChapter, 30 + rand() % 11);
                g_entityManager.addRole(enemyGudiao);
            }
            break;
        case WaveType::MIXED_LARGE:
            // 添加混合大型阵型 5feilian + 2Gudiao
            for (int i = 0; i < 5; ++i) {
                IRole *enemyFeilian = new FeilianEnemy(
                    140 + (i / 3) * 20, (i % 3) * 20 + 1, 90 + (i / 3) * 10, (i % 3) * 20 + 1, currentChapter,
                    7 + rand() % 6
                );
                g_entityManager.addRole(enemyFeilian);
            }
            for (int i = 0; i < 2; ++i) {
                IRole *enemyGudiao =
                    new GudiaoEnemy(156, i * 40 + 1, 100, i * 40 + 1, currentChapter, 30 + rand() % 11);
                g_entityManager.addRole(enemyGudiao);
            }
            break;
        default:
            break;
        }
    }

    // 绘图展示功能
    // 绘制开场动画 - 增强版
    void drawOpeningCG() {
        if (openingCGTimer >= 2 * controlDelayTime)
            openingCGTimer -= 2 * controlDelayTime;
        else {
            // 动画播放完毕，进入等待按键状态
            isPlayingOpeningCG = false;
            isWaitingStartKey  = true;
        }

        uint16_t elapsed = 2000 - openingCGTimer; // 已经过的时间
        uint8_t phase = elapsed / 400; // 分为5个阶段 (0-4)

        // 阶段0-1: 多层同心圆从中心向外扩散
        if (phase <= 1) {
            uint8_t r1 = elapsed / 25;      // 最快的圆
            uint8_t r2 = elapsed / 35;      // 中速圆
            uint8_t r3 = elapsed / 50;      // 慢速圆
            
            if (r1 > 0 && r1 < 60) OLED_DrawCircle(64, 32, r1, OLED_COLOR_NORMAL);
            if (r2 > 0 && r2 < 50) OLED_DrawCircle(64, 32, r2, OLED_COLOR_NORMAL);
            if (r3 > 0 && r3 < 40) OLED_DrawCircle(64, 32, r3, OLED_COLOR_NORMAL);
        }
        
        // 阶段2: 圆圈收缩 + 四角装饰线出现
        if (phase == 2) {
            uint8_t shrink = (elapsed - 800) / 10;
            uint8_t r = (shrink < 40) ? (40 - shrink) : 0;
            if (r > 5) OLED_DrawCircle(64, 32, r, OLED_COLOR_NORMAL);
            
            // 四角装饰线渐入
            uint8_t lineLen = shrink / 2;
            if (lineLen > 15) lineLen = 15;
            OLED_DrawLine(0, 0, lineLen, 0, OLED_COLOR_NORMAL);
            OLED_DrawLine(0, 0, 0, lineLen, OLED_COLOR_NORMAL);
            OLED_DrawLine(127, 0, 127 - lineLen, 0, OLED_COLOR_NORMAL);
            OLED_DrawLine(127, 0, 127, lineLen, OLED_COLOR_NORMAL);
            OLED_DrawLine(0, 63, lineLen, 63, OLED_COLOR_NORMAL);
            OLED_DrawLine(0, 63, 0, 63 - lineLen, OLED_COLOR_NORMAL);
            OLED_DrawLine(127, 63, 127 - lineLen, 63, OLED_COLOR_NORMAL);
            OLED_DrawLine(127, 63, 127, 63 - lineLen, OLED_COLOR_NORMAL);
        }
        
        // 阶段3-4: 标题显示 + 闪烁边框 + 装饰元素
        if (phase >= 3) {
            // 四角装饰线保持
            OLED_DrawLine(0, 0, 15, 0, OLED_COLOR_NORMAL);
            OLED_DrawLine(0, 0, 0, 15, OLED_COLOR_NORMAL);
            OLED_DrawLine(127, 0, 112, 0, OLED_COLOR_NORMAL);
            OLED_DrawLine(127, 0, 127, 15, OLED_COLOR_NORMAL);
            OLED_DrawLine(0, 63, 15, 63, OLED_COLOR_NORMAL);
            OLED_DrawLine(0, 63, 0, 48, OLED_COLOR_NORMAL);
            OLED_DrawLine(127, 63, 112, 63, OLED_COLOR_NORMAL);
            OLED_DrawLine(127, 63, 127, 48, OLED_COLOR_NORMAL);
            
            // 标题上下装饰线
            OLED_DrawLine(20, 24, 107, 24, OLED_COLOR_NORMAL);
            OLED_DrawLine(20, 40, 107, 40, OLED_COLOR_NORMAL);
            
            // 标题文字 "CHIP TANKS"
            OLED_PrintString(30, 28, "CHIP TANKS", &font8x6, OLED_COLOR_NORMAL);
            
            // 闪烁的小坦克装饰 (交替显示)
            if ((openingCGTimer / 100) % 2 == 0) {
                OLED_DrawFilledRectangle(10, 30, 6, 4, OLED_COLOR_NORMAL);  // 左侧小方块
                OLED_DrawFilledRectangle(111, 30, 6, 4, OLED_COLOR_NORMAL); // 右侧小方块
            }
            
            // 底部提示文字闪烁
            if (phase == 4 && (openingCGTimer / 150) % 2 == 0) {
                OLED_PrintString(28, 52, "PRESS TO START", &font8x6, OLED_COLOR_NORMAL);
            }
        }
    }

    // 绘制等待开始界面
    void drawWaitingStart() {
        // 四角装饰线保持
        OLED_DrawLine(0, 0, 15, 0, OLED_COLOR_NORMAL);
        OLED_DrawLine(0, 0, 0, 15, OLED_COLOR_NORMAL);
        OLED_DrawLine(127, 0, 112, 0, OLED_COLOR_NORMAL);
        OLED_DrawLine(127, 0, 127, 15, OLED_COLOR_NORMAL);
        OLED_DrawLine(0, 63, 15, 63, OLED_COLOR_NORMAL);
        OLED_DrawLine(0, 63, 0, 48, OLED_COLOR_NORMAL);
        OLED_DrawLine(127, 63, 112, 63, OLED_COLOR_NORMAL);
        OLED_DrawLine(127, 63, 127, 48, OLED_COLOR_NORMAL);
        
        // 标题上下装饰线
        OLED_DrawLine(20, 24, 107, 24, OLED_COLOR_NORMAL);
        OLED_DrawLine(20, 40, 107, 40, OLED_COLOR_NORMAL);
        
        // 标题文字
        OLED_PrintString(30, 28, "CHIP TANKS", &font8x6, OLED_COLOR_NORMAL);
        
        // 左右装饰方块
        OLED_DrawFilledRectangle(10, 30, 6, 4, OLED_COLOR_NORMAL);
        OLED_DrawFilledRectangle(111, 30, 6, 4, OLED_COLOR_NORMAL);
        
        // 底部闪烁提示文字
        static uint8_t blinkCounter = 0;
        blinkCounter++;
        if ((blinkCounter / 10) % 2 == 0) {
            OLED_PrintString(28, 52, "PRESS TO START", &font8x6, OLED_COLOR_NORMAL);
        }
    }

    // 绘制通关动画 - 增强版
    void drawClearCG() {
        if (clearCGTimer >= 2 * controlDelayTime)
            clearCGTimer -= 2 * controlDelayTime;
        else {
            isPlayingClearCG           = false;
            g_entityManager.isGameOver = true; // 游戏结束
        }

        uint16_t elapsed = 2000 - clearCGTimer; // 已经过的时间
        uint8_t phase = elapsed / 500; // 分为4个阶段 (0-3)
        
        // 阶段0: 胜利光芒从中心扩散
        if (phase == 0) {
            uint8_t rayLen = elapsed / 10;
            // 八方向射线扩散
            if (rayLen > 2) {
                OLED_DrawLine(64, 32, 64 + rayLen, 32, OLED_COLOR_NORMAL);           // 右
                OLED_DrawLine(64, 32, 64 - rayLen, 32, OLED_COLOR_NORMAL);           // 左
                OLED_DrawLine(64, 32, 64, 32 + rayLen/2, OLED_COLOR_NORMAL);         // 下
                OLED_DrawLine(64, 32, 64, 32 - rayLen/2, OLED_COLOR_NORMAL);         // 上
                OLED_DrawLine(64, 32, 64 + rayLen*7/10, 32 + rayLen*7/20, OLED_COLOR_NORMAL);  // 右下
                OLED_DrawLine(64, 32, 64 - rayLen*7/10, 32 + rayLen*7/20, OLED_COLOR_NORMAL);  // 左下
                OLED_DrawLine(64, 32, 64 + rayLen*7/10, 32 - rayLen*7/20, OLED_COLOR_NORMAL);  // 右上
                OLED_DrawLine(64, 32, 64 - rayLen*7/10, 32 - rayLen*7/20, OLED_COLOR_NORMAL);  // 左上
            }
        }
        
        // 阶段1: VICTORY 文字 + 星星装饰
        if (phase >= 1) {
            // 顶部大标题
            OLED_PrintString(34, 8, "VICTORY!", &font8x6, OLED_COLOR_NORMAL);
            
            // 闪烁星星效果 (用小十字表示)
            if ((clearCGTimer / 80) % 2 == 0) {
                // 左侧星星
                OLED_DrawLine(15, 10, 15, 14, OLED_COLOR_NORMAL);
                OLED_DrawLine(13, 12, 17, 12, OLED_COLOR_NORMAL);
                // 右侧星星
                OLED_DrawLine(112, 10, 112, 14, OLED_COLOR_NORMAL);
                OLED_DrawLine(110, 12, 114, 12, OLED_COLOR_NORMAL);
            }
            if ((clearCGTimer / 80) % 2 == 1) {
                // 交替显示另一组星星
                OLED_DrawLine(25, 5, 25, 9, OLED_COLOR_NORMAL);
                OLED_DrawLine(23, 7, 27, 7, OLED_COLOR_NORMAL);
                OLED_DrawLine(102, 5, 102, 9, OLED_COLOR_NORMAL);
                OLED_DrawLine(100, 7, 104, 7, OLED_COLOR_NORMAL);
            }
        }
        
        // 阶段2-3: 感谢文字 + 持续星星动画
        if (phase >= 2) {
            // 分隔线
            OLED_DrawLine(10, 22, 117, 22, OLED_COLOR_NORMAL);
            
            // 致谢文字
            OLED_PrintString(16, 28, "THANK YOU FOR", &font8x6, OLED_COLOR_NORMAL);
            OLED_PrintString(10, 40, "PLAYING MY GAME!", &font8x6, OLED_COLOR_NORMAL);
            
            // 底部装饰线
            OLED_DrawLine(10, 52, 117, 52, OLED_COLOR_NORMAL);
        }
        
        // 阶段3: 额外的庆祝装饰
        if (phase >= 3) {
            // 底部闪烁三角形装饰
            if ((clearCGTimer / 100) % 2 == 0) {
                OLED_DrawTriangle(20, 60, 25, 55, 30, 60, OLED_COLOR_NORMAL);
                OLED_DrawTriangle(97, 60, 102, 55, 107, 60, OLED_COLOR_NORMAL);
            }
            // 四角小方块装饰
            OLED_DrawFilledRectangle(2, 2, 4, 4, OLED_COLOR_NORMAL);
            OLED_DrawFilledRectangle(121, 2, 4, 4, OLED_COLOR_NORMAL);
            OLED_DrawFilledRectangle(2, 57, 4, 4, OLED_COLOR_NORMAL);
            OLED_DrawFilledRectangle(121, 57, 4, 4, OLED_COLOR_NORMAL);
        }
    }

    // 绘制展示Boss海报
    void drawShowBoss() {
        if (showBossTimer >= 2 * controlDelayTime)
            showBossTimer -= 2 * controlDelayTime;
        else {
             showBoss = false;
             chatpter4Warning = false;
             return;
        }
            
        if(chatpter4Warning) {
            OLED_PrintString(1, 40, "WARNING!", &font8x6, OLED_COLOR_NORMAL);
            OLED_PrintString(1, 52, "CHALLENGE!", &font8x6, OLED_COLOR_NORMAL);
        }

        switch (showWhichBoss) {
        case BOSS_TYPE::XIANG_LIU:
            OLED_PrintString(1, 20, "XIANG_LIU", &font8x6, OLED_COLOR_NORMAL);
            OLED_DrawImage(64, 1, &XiangliuImg, OLED_COLOR_NORMAL);
            break;
        case BOSS_TYPE::TAO_TIE:
            OLED_PrintString(1, 20, "TAO_TIE", &font8x6, OLED_COLOR_NORMAL);
            OLED_DrawImage(64, 1, &TaotieImg, OLED_COLOR_NORMAL);
            break;

        case BOSS_TYPE::TAO_WU:
            OLED_PrintString(1, 20, "TAO_WU", &font8x6, OLED_COLOR_NORMAL);
            OLED_DrawImage(64, 1, &TaowuImg, OLED_COLOR_NORMAL);
            break;

        default:
            break;
        }
    }
};

#endif // GAMEPROGRESSMANAGER_HPP
