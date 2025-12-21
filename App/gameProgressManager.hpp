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
            showBossTimer = 2000; // 播放2秒Boss海报
            return;
        }
        if (currentChapter == 2 && currentWave == currentChapterMaxWaves) {
            // 第二关最后一波，添加Boss相柳
            IRole *enemyXiangliu = new XiangliuEnemy(156, 1 , 64, 1 , currentChapter, 130 + rand() % 21);
            g_entityManager.addRole(enemyXiangliu);

            // 标记展示Boss海报
            showWhichBoss = BOSS_TYPE::XIANG_LIU; // 相柳Boss
            showBoss      = true;
            showBossTimer = 2000; // 播放2秒Boss海报
            return;
        }
        if (currentChapter == 3 && currentWave == currentChapterMaxWaves) {
            // 第三关最后一波，添加Boss梼杌
            IRole *enemyTaowu = new TaowuEnemy(156, 1 , 64, 1 , currentChapter, 160 + rand() % 21);
            g_entityManager.addRole(enemyTaowu);

            // 标记展示Boss海报
            showWhichBoss = BOSS_TYPE::TAO_WU; // 梼杌Boss
            showBoss      = true;
            showBossTimer = 2000; // 播放2秒Boss海报
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
                    showBossTimer = 2000; // 播放2秒Boss海报
                    break;
                }
            case BOSS_TYPE::XIANG_LIU:
                {
                    IRole *enemyXiangliu = new XiangliuEnemy(156, 1, 64, 1, currentChapter + 2 , 230 + rand() % 51);
                    g_entityManager.addRole(enemyXiangliu);

                    // 标记展示Boss海报
                    showWhichBoss = BOSS_TYPE::XIANG_LIU; // 相柳Boss
                    showBoss      = true;
                    showBossTimer = 2000; // 播放2秒Boss海报
                    break;
                }
            case BOSS_TYPE::TAO_WU:
                {
                    IRole *enemyTaowu = new TaowuEnemy(156, 1 , 64, 1 , currentChapter + 2 , 260 + rand() % 51);
                    g_entityManager.addRole(enemyTaowu);

                    // 标记展示Boss海报
                    showWhichBoss = BOSS_TYPE::TAO_WU; // 梼杌Boss
                    showBoss      = true;
                    showBossTimer = 2000; // 播放2秒Boss海报
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
            // 四角装饰线保持（古风卷轴边框）
            OLED_DrawLine(0, 0, 18, 0, OLED_COLOR_NORMAL);
            OLED_DrawLine(0, 0, 0, 18, OLED_COLOR_NORMAL);
            OLED_DrawLine(127, 0, 109, 0, OLED_COLOR_NORMAL);
            OLED_DrawLine(127, 0, 127, 18, OLED_COLOR_NORMAL);
            OLED_DrawLine(0, 63, 18, 63, OLED_COLOR_NORMAL);
            OLED_DrawLine(0, 63, 0, 45, OLED_COLOR_NORMAL);
            OLED_DrawLine(127, 63, 109, 63, OLED_COLOR_NORMAL);
            OLED_DrawLine(127, 63, 127, 45, OLED_COLOR_NORMAL);
            
            // 内层装饰框（卷轴感）
            OLED_DrawLine(5, 5, 15, 5, OLED_COLOR_NORMAL);
            OLED_DrawLine(5, 5, 5, 15, OLED_COLOR_NORMAL);
            OLED_DrawLine(122, 5, 112, 5, OLED_COLOR_NORMAL);
            OLED_DrawLine(122, 5, 122, 15, OLED_COLOR_NORMAL);
            OLED_DrawLine(5, 58, 15, 58, OLED_COLOR_NORMAL);
            OLED_DrawLine(5, 58, 5, 48, OLED_COLOR_NORMAL);
            OLED_DrawLine(122, 58, 112, 58, OLED_COLOR_NORMAL);
            OLED_DrawLine(122, 58, 122, 48, OLED_COLOR_NORMAL);
            
            // 标题上下装饰线
            OLED_DrawLine(15, 18, 112, 18, OLED_COLOR_NORMAL);
            OLED_DrawLine(15, 44, 112, 44, OLED_COLOR_NORMAL);
            
            // 主标题 "CHIP TANKS"
            OLED_PrintString(30, 22, "CHIP TANKS", &font8x6, OLED_COLOR_NORMAL);
            
            // 副标题 "In SHAN HAI JING" (山海经)
            OLED_PrintString(20, 34, "- SHAN HAI JING -", &font8x6, OLED_COLOR_NORMAL);
            
            // 闪烁的小装饰 (交替显示 - 模拟古籍符文)
            if ((openingCGTimer / 100) % 2 == 0) {
                // 左右小方块装饰
                OLED_DrawFilledRectangle(8, 28, 4, 4, OLED_COLOR_NORMAL);
                OLED_DrawFilledRectangle(115, 28, 4, 4, OLED_COLOR_NORMAL);
            } else {
                // 交替的菱形装饰
                OLED_DrawLine(10, 28, 10, 32, OLED_COLOR_NORMAL);
                OLED_DrawLine(117, 28, 117, 32, OLED_COLOR_NORMAL);
            }
            
            // 底部提示文字闪烁
            if (phase == 4 && (openingCGTimer / 150) % 2 == 0) {
                OLED_PrintString(28, 52, "PRESS TO START", &font8x6, OLED_COLOR_NORMAL);
            }
        }
    }

    // 绘制等待开始界面 - 山海经主题
    void drawWaitingStart() {
        // 四角装饰线（古风卷轴边框）
        OLED_DrawLine(0, 0, 18, 0, OLED_COLOR_NORMAL);
        OLED_DrawLine(0, 0, 0, 18, OLED_COLOR_NORMAL);
        OLED_DrawLine(127, 0, 109, 0, OLED_COLOR_NORMAL);
        OLED_DrawLine(127, 0, 127, 18, OLED_COLOR_NORMAL);
        OLED_DrawLine(0, 63, 18, 63, OLED_COLOR_NORMAL);
        OLED_DrawLine(0, 63, 0, 45, OLED_COLOR_NORMAL);
        OLED_DrawLine(127, 63, 109, 63, OLED_COLOR_NORMAL);
        OLED_DrawLine(127, 63, 127, 45, OLED_COLOR_NORMAL);
        
        // 内层装饰框（卷轴感）
        OLED_DrawLine(5, 5, 15, 5, OLED_COLOR_NORMAL);
        OLED_DrawLine(5, 5, 5, 15, OLED_COLOR_NORMAL);
        OLED_DrawLine(122, 5, 112, 5, OLED_COLOR_NORMAL);
        OLED_DrawLine(122, 5, 122, 15, OLED_COLOR_NORMAL);
        OLED_DrawLine(5, 58, 15, 58, OLED_COLOR_NORMAL);
        OLED_DrawLine(5, 58, 5, 48, OLED_COLOR_NORMAL);
        OLED_DrawLine(122, 58, 112, 58, OLED_COLOR_NORMAL);
        OLED_DrawLine(122, 58, 122, 48, OLED_COLOR_NORMAL);
        
        // 标题上下装饰线
        OLED_DrawLine(15, 18, 112, 18, OLED_COLOR_NORMAL);
        OLED_DrawLine(15, 44, 112, 44, OLED_COLOR_NORMAL);
        
        // 主标题 "CHIP TANKS"
        OLED_PrintString(30, 22, "CHIP TANKS", &font8x6, OLED_COLOR_NORMAL);
        
        // 副标题 "In SHAN HAI JING" (山海经)
        OLED_PrintString(20, 34, "- SHAN HAI JING -", &font8x6, OLED_COLOR_NORMAL);
        
        // 左右装饰方块
        OLED_DrawFilledRectangle(8, 28, 4, 4, OLED_COLOR_NORMAL);
        OLED_DrawFilledRectangle(115, 28, 4, 4, OLED_COLOR_NORMAL);
        
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

    // ========== Boss海报展示函数 ==========
    
    // 饕餮 - 贪婪吞噬效果
    // 特性：无尽的饥饿、吞噬万物、黑暗压迫
    void drawBossTaotie(uint16_t elapsed) {
        uint8_t phase = elapsed / 250; // 分为8阶段 (0-7)
        
        // 阶段0-1: 黑暗从四边向内收缩（压迫感）
        if (phase <= 1) {
            uint8_t shrink = elapsed / 8; // 0-62
            // 上下黑暗收缩（多层线条增强压迫感）
            for (uint8_t i = 0; i < shrink && i < 25; i += 3) {
                OLED_DrawLine(0, i, 127, i, OLED_COLOR_NORMAL);
                OLED_DrawLine(0, 63 - i, 127, 63 - i, OLED_COLOR_NORMAL);
            }
            // 左右收缩
            for (uint8_t i = 0; i < shrink / 2 && i < 15; i += 4) {
                OLED_DrawLine(i, 0, i, 63, OLED_COLOR_NORMAL);
                OLED_DrawLine(127 - i, 0, 127 - i, 63, OLED_COLOR_NORMAL);
            }
        }
        
        // 阶段2: 中心爆发 - 圆环扩散
        if (phase == 2) {
            uint8_t r = (elapsed - 500) / 8;
            if (r > 5 && r < 50) {
                OLED_DrawCircle(96, 32, r, OLED_COLOR_NORMAL);
                if (r > 10) OLED_DrawCircle(96, 32, r - 8, OLED_COLOR_NORMAL);
            }
        }
        
        // 阶段3+: Boss图像出现
        if (phase >= 3) {
            OLED_DrawImage(64, 1, &TaotieImg, OLED_COLOR_NORMAL);
        }
        
        // 阶段3-4: 名字逐字闪烁出现
        if (phase == 3) {
            uint8_t charShow = (elapsed - 750) / 60;
            if (charShow >= 1) OLED_PrintString(1, 5, "T", &font8x6, OLED_COLOR_NORMAL);
            if (charShow >= 2) OLED_PrintString(7, 5, "A", &font8x6, OLED_COLOR_NORMAL);
            if (charShow >= 3) OLED_PrintString(13, 5, "O", &font8x6, OLED_COLOR_NORMAL);
        }
        if (phase >= 4) {
            OLED_PrintString(1, 5, "TAO TIE", &font8x6, OLED_COLOR_NORMAL);
        }
        
        // 阶段4-5: "吞噬"效果 - 从四角向Boss涌动的线条
        if (phase >= 4 && phase <= 5) {
            uint8_t flowLen = ((elapsed - 1000) / 20) % 30;
            // 四角吞噬流
            OLED_DrawLine(0, 0, flowLen, flowLen / 2, OLED_COLOR_NORMAL);
            OLED_DrawLine(0, 63, flowLen, 63 - flowLen / 2, OLED_COLOR_NORMAL);
            OLED_DrawLine(60, 0, 60 - flowLen / 3, flowLen / 2, OLED_COLOR_NORMAL);
            OLED_DrawLine(60, 63, 60 - flowLen / 3, 63 - flowLen / 2, OLED_COLOR_NORMAL);
        }
        
        // 阶段5+: 底部獠牙锯齿（饕餮之口）
        if (phase >= 5) {
            // 上下獠牙
            for (uint8_t i = 0; i < 60; i += 10) {
                OLED_DrawFilledTriangle(i, 63, i + 5, 55, i + 10, 63, OLED_COLOR_NORMAL);
                OLED_DrawFilledTriangle(i, 0, i + 5, 8, i + 10, 0, OLED_COLOR_NORMAL);
            }
        }
        
        // 阶段6+: 贪婪标签 + 脉动效果
        if (phase >= 6) {
            OLED_PrintString(1, 18, "DEVOUR", &font8x6, OLED_COLOR_NORMAL);
            OLED_PrintString(1, 30, "ALL!", &font8x6, OLED_COLOR_NORMAL);
            
            // 脉动的饥饿圈
            uint8_t pulse = (elapsed / 50) % 2;
            if (pulse == 0) {
                OLED_DrawCircle(30, 50, 8, OLED_COLOR_NORMAL);
            } else {
                OLED_DrawCircle(30, 50, 6, OLED_COLOR_NORMAL);
            }
        }
        
        // 阶段7: 最终强化 - 全屏压迫边框
        if (phase >= 7) {
            OLED_DrawRectangle(0, 0, 60, 64, OLED_COLOR_NORMAL);
            // 闪烁警示
            if ((elapsed / 40) % 2 == 0) {
                OLED_DrawLine(1, 45, 58, 45, OLED_COLOR_NORMAL);
            }
        }
    }
    
    // 相柳 - 九头蛇效果
    // 特性：九个头、水属性、蜿蜒狡诈、剧毒
    void drawBossXiangliu(uint16_t elapsed) {
        uint8_t phase = elapsed / 250; // 分为8阶段
        
        // 阶段0-1: 水波纹从中心扩散
        if (phase <= 1) {
            uint8_t waveR = elapsed / 6;
            // 多层水波
            if (waveR > 5) OLED_DrawCircle(64, 32, waveR, OLED_COLOR_NORMAL);
            if (waveR > 15) OLED_DrawCircle(64, 32, waveR - 10, OLED_COLOR_NORMAL);
            if (waveR > 25) OLED_DrawCircle(64, 32, waveR - 20, OLED_COLOR_NORMAL);
            if (waveR > 35) OLED_DrawCircle(64, 32, waveR - 30, OLED_COLOR_NORMAL);
        }
        
        // 阶段2: 九条蛇形线从左侧涌入
        if (phase == 2) {
            uint8_t snakeLen = (elapsed - 500) / 3;
            for (uint8_t row = 0; row < 9; row++) {
                uint8_t y = 3 + row * 7;
                for (uint8_t x = 0; x < snakeLen && x < 55; x += 2) {
                    int8_t wave = ((x / 3 + row) % 3) - 1; // -1, 0, 1
                    if (y + wave > 0 && y + wave < 64) {
                        OLED_SetPixel(x, y + wave, OLED_COLOR_NORMAL);
                        OLED_SetPixel(x + 1, y + wave, OLED_COLOR_NORMAL);
                    }
                }
            }
        }
        
        // 阶段3+: Boss图像出现
        if (phase >= 3) {
            OLED_DrawImage(64, 1, &XiangliuImg, OLED_COLOR_NORMAL);
        }
        
        // 阶段3-4: 名字蛇形出现
        if (phase >= 3 && phase <= 4) {
            uint8_t showChars = (elapsed - 750) / 80;
            char name[] = "XIANG LIU";
            char buf[2] = {0, 0};
            for (uint8_t i = 0; i < showChars && i < 9; i++) {
                buf[0] = name[i];
                int8_t yOffset = (i % 2 == 0) ? -1 : 1;
                OLED_PrintString(1 + i * 6, 5 + yOffset, buf, &font8x6, OLED_COLOR_NORMAL);
            }
        }
        if (phase >= 5) {
            OLED_PrintString(1, 5, "XIANG LIU", &font8x6, OLED_COLOR_NORMAL);
        }
        
        // 阶段4+: 持续的九头蛇波浪（左侧装饰）
        if (phase >= 4) {
            uint8_t waveOffset = (elapsed / 30) % 12;
            for (uint8_t row = 0; row < 9; row++) {
                uint8_t y = 3 + row * 7;
                for (uint8_t x = 0; x < 58; x += 2) {
                    int8_t wave = ((x + waveOffset + row * 2) / 4) % 3 - 1;
                    if (y + wave > 0 && y + wave < 64) {
                        OLED_SetPixel(x, y + wave, OLED_COLOR_NORMAL);
                    }
                }
            }
        }
        
        // 阶段5+: "9 HEADS"标签 + 九个头的暗示
        if (phase >= 5) {
            OLED_PrintString(1, 20, "9 HEADS", &font8x6, OLED_COLOR_NORMAL);
            
            // 九个小圆点代表九个头
            uint8_t headShow = (phase >= 6) ? 9 : (elapsed - 1250) / 50;
            for (uint8_t i = 0; i < headShow && i < 9; i++) {
                uint8_t hx = 5 + (i % 3) * 12;
                uint8_t hy = 35 + (i / 3) * 8;
                OLED_DrawFilledCircle(hx, hy, 2, OLED_COLOR_NORMAL);
            }
        }
        
        // 阶段6+: 毒液滴落效果
        if (phase >= 6) {
            OLED_PrintString(1, 55, "VENOM", &font8x6, OLED_COLOR_NORMAL);
            // 滴落的毒液点
            uint8_t dropY = (elapsed / 40) % 20;
            for (uint8_t i = 0; i < 4; i++) {
                uint8_t dx = 40 + i * 5;
                uint8_t dy = 10 + dropY + i * 3;
                if (dy < 63) OLED_SetPixel(dx, dy, OLED_COLOR_NORMAL);
            }
        }
        
        // 阶段7: 蛇形边框
        if (phase >= 7) {
            for (uint8_t x = 0; x < 60; x += 3) {
                int8_t wave = ((x + elapsed / 30) / 5) % 2;
                OLED_SetPixel(x, wave, OLED_COLOR_NORMAL);
                OLED_SetPixel(x, 63 - wave, OLED_COLOR_NORMAL);
            }
        }
    }
    
    // 梼杌 - 凶猛狂暴效果
    // 特性：凶猛野兽、破坏、狂暴、不可阻挡
    void drawBossTaowu(uint16_t elapsed) {
        uint8_t phase = elapsed / 250; // 分为8阶段
        
        // 阶段0-1: 强烈震动 + 裂痕出现
        if (phase <= 1) {
            // 震动条纹
            uint8_t shakeIntensity = elapsed / 40;
            uint8_t offset = (elapsed / 20) % 4;
            for (uint8_t y = offset; y < 64; y += 4) {
                uint8_t xShift = (y / 8) % 2 == 0 ? 2 : -2;
                OLED_DrawLine(xShift, y, 127 + xShift, y, OLED_COLOR_NORMAL);
            }
            
            // 裂痕从中心向外扩展
            if (shakeIntensity > 5) {
                uint8_t crackLen = shakeIntensity - 5;
                OLED_DrawLine(64, 32, 64 + crackLen, 32 - crackLen / 2, OLED_COLOR_NORMAL);
                OLED_DrawLine(64, 32, 64 - crackLen, 32 + crackLen / 2, OLED_COLOR_NORMAL);
                OLED_DrawLine(64, 32, 64 + crackLen / 2, 32 + crackLen, OLED_COLOR_NORMAL);
                OLED_DrawLine(64, 32, 64 - crackLen / 2, 32 - crackLen, OLED_COLOR_NORMAL);
            }
        }
        
        // 阶段2: 爆炸效果 - 碎片飞溅
        if (phase == 2) {
            // 爆炸中心闪光
            OLED_DrawFilledCircle(96, 32, 15 - (elapsed - 500) / 30, OLED_COLOR_NORMAL);
            
            // 飞溅碎片
            uint8_t fragDist = (elapsed - 500) / 8;
            for (uint8_t i = 0; i < 8; i++) {
                int8_t dx = (i % 2 == 0 ? 1 : -1) * (fragDist + i * 3);
                int8_t dy = (i / 2 % 2 == 0 ? 1 : -1) * (fragDist / 2 + i * 2);
                if (96 + dx > 64 && 96 + dx < 127 && 32 + dy > 0 && 32 + dy < 64) {
                    OLED_DrawFilledRectangle(96 + dx, 32 + dy, 3, 3, OLED_COLOR_NORMAL);
                }
            }
        }
        
        // 阶段3+: Boss图像（持续震动）
        if (phase >= 3) {
            int8_t shakeX = ((elapsed / 30) % 5) - 2; // -2 到 2
            int8_t shakeY = ((elapsed / 40) % 3) - 1; // -1 到 1
            if (phase >= 5) { shakeX /= 2; shakeY /= 2; } // 后期减弱震动
            OLED_DrawImage(64 + shakeX, 1 + shakeY, &TaowuImg, OLED_COLOR_NORMAL);
        }
        
        // 阶段3-4: 名字闪电般出现
        if (phase == 3 || phase == 4) {
            if ((elapsed / 40) % 3 != 0) { // 闪烁效果
                OLED_PrintString(1, 5, "TAO WU", &font8x6, OLED_COLOR_NORMAL);
            }
        }
        if (phase >= 5) {
            OLED_PrintString(1, 5, "TAO WU", &font8x6, OLED_COLOR_NORMAL);
        }
        
        // 阶段4+: 双侧闪电
        if (phase >= 4) {
            uint8_t lightningPhase = (elapsed / 50) % 4;
            
            // 左侧主闪电
            if (lightningPhase == 0 || lightningPhase == 2) {
                OLED_DrawLine(5, 15, 18, 28, OLED_COLOR_NORMAL);
                OLED_DrawLine(18, 28, 8, 28, OLED_COLOR_NORMAL);
                OLED_DrawLine(8, 28, 22, 45, OLED_COLOR_NORMAL);
                OLED_DrawLine(22, 45, 12, 45, OLED_COLOR_NORMAL);
                OLED_DrawLine(12, 45, 28, 60, OLED_COLOR_NORMAL);
            }
            
            // 右侧副闪电
            if (lightningPhase == 1 || lightningPhase == 3) {
                OLED_DrawLine(55, 20, 45, 32, OLED_COLOR_NORMAL);
                OLED_DrawLine(45, 32, 52, 32, OLED_COLOR_NORMAL);
                OLED_DrawLine(52, 32, 40, 48, OLED_COLOR_NORMAL);
                OLED_DrawLine(40, 48, 48, 48, OLED_COLOR_NORMAL);
                OLED_DrawLine(48, 48, 35, 62, OLED_COLOR_NORMAL);
            }
        }
        
        // 阶段5+: 狂暴标签 + 爪痕
        if (phase >= 5) {
            OLED_PrintString(1, 18, "FURY!", &font8x6, OLED_COLOR_NORMAL);
            
            // 爪痕效果
            OLED_DrawLine(2, 30, 15, 45, OLED_COLOR_NORMAL);
            OLED_DrawLine(6, 30, 19, 45, OLED_COLOR_NORMAL);
            OLED_DrawLine(10, 30, 23, 45, OLED_COLOR_NORMAL);
        }
        
        // 阶段6+: 破坏标签 + 碎裂边框
        if (phase >= 6) {
            OLED_PrintString(1, 32, "DESTROY", &font8x6, OLED_COLOR_NORMAL);
            
            // 碎裂边框效果
            // 左上角碎裂
            OLED_DrawLine(0, 0, 12, 0, OLED_COLOR_NORMAL);
            OLED_DrawLine(0, 0, 0, 10, OLED_COLOR_NORMAL);
            OLED_DrawLine(12, 0, 8, 6, OLED_COLOR_NORMAL);
            OLED_DrawLine(0, 10, 5, 7, OLED_COLOR_NORMAL);
            
            // 左下角碎裂
            OLED_DrawLine(0, 63, 10, 63, OLED_COLOR_NORMAL);
            OLED_DrawLine(0, 63, 0, 53, OLED_COLOR_NORMAL);
            OLED_DrawLine(10, 63, 7, 57, OLED_COLOR_NORMAL);
            OLED_DrawLine(0, 53, 6, 56, OLED_COLOR_NORMAL);
        }
        
        // 阶段7: 全屏狂暴脉动
        if (phase >= 7) {
            // 脉动的狂暴光环
            uint8_t pulseR = 20 + (elapsed / 60) % 10;
            OLED_DrawCircle(30, 48, pulseR, OLED_COLOR_NORMAL);
            
            // 底部警告闪烁
            if ((elapsed / 50) % 2 == 0) {
                OLED_PrintString(1, 55, "UNSTOP!", &font8x6, OLED_COLOR_NORMAL);
            }
        }
    }
    
    // ========== 主展示函数 ==========
    void drawShowBoss() {
        if (showBossTimer >= 2 * controlDelayTime)
            showBossTimer -= 2 * controlDelayTime;
        else {
            showBoss         = false;
            chatpter4Warning = false;
            return;
        }

        uint16_t elapsed = 2000 - showBossTimer; // 已经过的时间（2秒总时长）
        
        // 第四章挑战警告（覆盖在所有内容之上）
        if (chatpter4Warning) {
            // 闪烁的双层警告框
            if ((showBossTimer / 60) % 2 == 0) {
                OLED_DrawRectangle(0, 38, 62, 26, OLED_COLOR_NORMAL);
                OLED_DrawRectangle(2, 40, 58, 22, OLED_COLOR_NORMAL);
            }
            // 交替显示警告文字
            if ((showBossTimer / 100) % 2 == 0) {
                OLED_PrintString(4, 44, "WARNING!", &font8x6, OLED_COLOR_NORMAL);
            } else {
                OLED_PrintString(4, 44, "DANGER!!", &font8x6, OLED_COLOR_NORMAL);
            }
            OLED_PrintString(4, 54, "CHALLENGE", &font8x6, OLED_COLOR_NORMAL);
        }

        // 根据Boss类型调用对应的展示函数
        switch (showWhichBoss) {
        case BOSS_TYPE::TAO_TIE:
            drawBossTaotie(elapsed);
            break;
        case BOSS_TYPE::XIANG_LIU:
            drawBossXiangliu(elapsed);
            break;
        case BOSS_TYPE::TAO_WU:
            drawBossTaowu(elapsed);
            break;
        default:
            break;
        }
    }
};

#endif // GAMEPROGRESSMANAGER_HPP
