#include "main.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h"

#include "etl/vector.h"
#include "etl/algorithm.h"

#include "font.h"

#include "Bullet/bullet.hpp"
#include "Role/leadingRole.hpp"
#include "Role/enemyRole.hpp"

#include "gameEntityManager.hpp"
#include "gamePerkCardManager.hpp"
#include "gameProgressManager.hpp"

/******************************************************************/

GameEntityManager   g_entityManager;
GamePerkCardManager g_perkCardManager;
GameProgressManager g_progressManager;
LeadingRole        *pLeadingRole = nullptr; // 全局主角指针

uint8_t controlDelayTime = 10; // 控制线程延时，单位ms

/*******************************oled*********************************/
#include "oled.h"
/********************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

void oledTaskThread(void *argument) {
    osDelay(50); // OLED 初始化需要等待电源稳定（OLED_Init内部还会再等100ms）
    OLED_Init(); // 初始化OLED

    for (;;) {
        OLED_NewFrame();
        if (!g_entityManager.isGameOver) {
            if (g_progressManager.isPlayingOpeningCG) {
                // 播放开场动画
                g_progressManager.drawOpeningCG();
            }

            else if (g_progressManager.isWaitingStartKey) {
                // 等待按键开始游戏
                g_progressManager.drawWaitingStart();
            }

            else if (g_progressManager.isPlayingClearCG) {
                // 播放通关动画
                g_progressManager.drawClearCG();
            }

            else if (g_progressManager.showBoss) {
                // 展示Boss海报
                g_progressManager.drawShowBoss();
            }

            else if (g_perkCardManager.m_isSelecting && g_perkCardManager.isInited) {
                // 处于选卡状态，显示选卡界面
                g_perkCardManager.drawSelectionUI();
            }
            else if(g_progressManager.PauseGame) {
                // 游戏暂停状态，显示详细玩家属性界面
                g_progressManager.drawPauseUI();
            }
            else {
                // 非选卡状态，显示游戏界面

                //显示角色信息
                char infoStr[32];
                if (pLeadingRole != nullptr) {
                    sprintf(
                        infoStr, "HP:%d/%d Lv.%d", pLeadingRole->getData()->healthData.currentHealth,
                        pLeadingRole->getData()->healthData.maxHealth, pLeadingRole->getData()->level

                    );
                    OLED_PrintString(0, 56, infoStr, &font8x6, OLED_COLOR_NORMAL);
                }

                // 绘制游戏界面
                g_entityManager.drawAllRoles();
                g_entityManager.drawAllBullets();
            }
        }

        // 调试信息显示
        OLED_ShowFrame();
        osDelay(controlDelayTime * 2); // 控制刷新频率
    }
}

#ifdef __cplusplus
}
#endif
/********************************************************************/

/*****************************keyScan********************************/
#include "Peripheral/KEY/key.hpp"
/********************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

void keyScanThread(void *argument) {
    uint16_t scanDelayTime = 40; // 按键扫描线程延时，单位ms
    key.init();
    for (;;) {
        key.scan();

        // 等待按键开始游戏的处理
        if (g_progressManager.isWaitingStartKey) {
            // 检测任意按键按下
            bool anyKeyPressed = false;
            for (uint8_t i = 0; i < 4; ++i) {
                if (key.m_leftKeyButton[i] == 1 || key.m_rightKeyButton[i] == 1) {
                    anyKeyPressed = true;
                    break;
                }
            }
            if (anyKeyPressed) {
                g_progressManager.isWaitingStartKey = false; // 开始游戏
            }
        }

        if (!g_entityManager.isGameOver && !g_progressManager.isPlayingOpeningCG && !g_progressManager.showBoss
            && !g_progressManager.isPlayingClearCG && !g_progressManager.isWaitingStartKey) {
            // 游戏进行中才响应按键

            //测试触发选卡
            // if (key.m_keyButton[14] == 1) {
            //     g_perkCardManager.triggerPerkSelection();
            // }

            if (!g_perkCardManager.m_isSelecting) {
                scanDelayTime = 40; // 非选卡时恢复正常扫描频率
                pLeadingRole  = (LeadingRole *)g_entityManager.getPlayerRole();
                if (pLeadingRole != nullptr) {
                    //MOVING
                    if (key.m_leftKeyButton[static_cast<uint8_t>(LeftKeyState::KEY_LEFT)] == 1) {
                        pLeadingRole->getData()->actionData.currentState = ActionState::MOVING;
                        pLeadingRole->getData()->actionData.moveMode     = MoveMode::LEFT; // Move left
                    }
                    if (key.m_leftKeyButton[static_cast<uint8_t>(LeftKeyState::KEY_DOWN)] == 1) {
                        pLeadingRole->getData()->actionData.currentState = ActionState::MOVING;
                        pLeadingRole->getData()->actionData.moveMode     = MoveMode::DOWN; // Move down
                    }
                    if (key.m_leftKeyButton[static_cast<uint8_t>(LeftKeyState::KEY_UP)] == 1) {
                        pLeadingRole->getData()->actionData.currentState = ActionState::MOVING;
                        pLeadingRole->getData()->actionData.moveMode     = MoveMode::UP; // Move up
                    }
                    if (key.m_leftKeyButton[static_cast<uint8_t>(LeftKeyState::KEY_RIGHT)] == 1) {
                        pLeadingRole->getData()->actionData.currentState = ActionState::MOVING;
                        pLeadingRole->getData()->actionData.moveMode     = MoveMode::RIGHT; // Move right
                    }

                    //PAUSE
                    if (key.m_rightKeyButton[static_cast<uint8_t>(RightKeyState::KEY_DOWN)] == 1 ||
                        key.m_rightKeyButton[static_cast<uint8_t>(RightKeyState::KEY_UP)] == 1 ||
                        key.m_rightKeyButton[static_cast<uint8_t>(RightKeyState::KEY_LEFT)] == 1 ||
                        key.m_rightKeyButton[static_cast<uint8_t>(RightKeyState::KEY_RIGHT)] == 1) {
                        g_progressManager.PauseTimer += scanDelayTime;
                        if (g_progressManager.PauseTimer >= 1200) {
                            g_progressManager.PauseGame = !g_progressManager.PauseGame;
                            g_progressManager.PauseTimer = 0;
                        }
                    }
                    
                }
            } else {// 处于选卡状态，响应选卡按键
                scanDelayTime                     = 100; // 选卡时降低扫描频率，节省资源
                g_perkCardManager.m_selectedIndex = etl::min(
                    (uint8_t)(g_perkCardManager.m_selectedIndex), (uint8_t)(g_perkCardManager.m_selectedSize - 1)
                );
                g_perkCardManager.m_selectedIndex = etl::max((int16_t)(g_perkCardManager.m_selectedIndex), (int16_t)0);
                
                if (key.m_leftKeyButton[static_cast<uint8_t>(LeftKeyState::KEY_DOWN)] == 1)
                    g_perkCardManager.m_selectedIndex = etl::min(
                        (uint8_t)(g_perkCardManager.m_selectedIndex + 1),
                        (uint8_t)(g_perkCardManager.m_selectedSize - 1)
                    );
                if (key.m_leftKeyButton[static_cast<uint8_t>(LeftKeyState::KEY_UP)] == 1)
                    g_perkCardManager.m_selectedIndex =
                        etl::max((int16_t)((int16_t)g_perkCardManager.m_selectedIndex - 1), (int16_t)0);

                if (key.m_rightKeyButton[static_cast<uint8_t>(RightKeyState::KEY_RIGHT)] == 1 ||
                    key.m_rightKeyButton[static_cast<uint8_t>(RightKeyState::KEY_LEFT)] == 1 ||
                    //key.m_rightKeyButton[static_cast<uint8_t>(RightKeyState::KEY_DOWN)] == 1 //||
                    key.m_rightKeyButton[static_cast<uint8_t>(RightKeyState::KEY_UP)] == 1
                ) {
                    g_perkCardManager.selectCard(g_perkCardManager.m_selectedIndex);
                }
            }
        }
        osDelay(scanDelayTime);
    }
}

#ifdef __cplusplus
}
#endif
/********************************************************************/

/****************************gameControl*****************************/
#include "enemyRole.hpp"
#include "leadingRole.hpp"

/***********/
/************************** 调试模式配置 **************************/
// 设置为 1 启用调试模式，0 为正常游戏模式
#define DEBUG_MODE_ENABLED 1

#if DEBUG_MODE_ENABLED
// 调试敌人类型枚举
enum class DebugEnemyType {
    CHIMEI = 0,    // 魑魅（普通小怪）
    FEILIAN,       // 飞廉（普通小怪）
    GUDIAO,        // 古雕（普通小怪）
    BOSS_TAOTIE,   // Boss 饕餮
    BOSS_TAOWU,    // Boss 梼杌
    BOSS_XIANGLIU, // Boss 相柳
    BOSS_HUNDUN,   // Boss 混沌（四凶之首）
    MIXED_NORMAL,  // 混合普通敌人
};

// 调试模式配置
struct DebugConfig {
    DebugEnemyType enemyType = DebugEnemyType::BOSS_HUNDUN; // 当前测试的敌人类型
    uint8_t enemyCount       = 1;                       // 生成敌人数量（普通敌人有效）
    bool autoRespawn         = true;                    // 敌人全灭后是否自动重新生成
};

static DebugConfig g_debugConfig;

// 调试模式下生成敌人
static void debugSpawnEnemies() {
    if (!g_debugConfig.autoRespawn) return;
    if (g_entityManager.m_roles.size() > 1 || g_entityManager.isGameOver) return;
    
    switch (g_debugConfig.enemyType) {
        case DebugEnemyType::CHIMEI: {
            for (int i = 0; i < g_debugConfig.enemyCount; i++) {
                IRole* enemy = new ChiMeiEnemy(124 + (i/3)*30, (i%3)*24+1, 90 + (i/3)*15, (i%3)*24+1);
                if (!g_entityManager.addRole(enemy)) delete enemy;
            }
            break;
        }
        case DebugEnemyType::FEILIAN: {
            for (int i = 0; i < g_debugConfig.enemyCount; i++) {
                IRole* enemy = new FeilianEnemy(140 + (i/3)*30, (i%3)*24+1, 90 + (i/3)*15, (i%3)*24+1);
                if (!g_entityManager.addRole(enemy)) delete enemy;
            }
            break;
        }
        case DebugEnemyType::GUDIAO: {
            IRole* enemy = new GudiaoEnemy(156, 32, 100, 26);
            if (!g_entityManager.addRole(enemy)) delete enemy;
            break;
        }
        case DebugEnemyType::BOSS_TAOTIE: {
            IRole* enemy = new TaotieEnemy(180, 0, 64, 0);
            if (!g_entityManager.addRole(enemy)) delete enemy;
            break;
        }
        case DebugEnemyType::BOSS_TAOWU: {
            IRole* enemy = new TaowuEnemy(180, 0, 64, 0);
            if (!g_entityManager.addRole(enemy)) delete enemy;
            break;
        }
        case DebugEnemyType::BOSS_XIANGLIU: {
            IRole* enemy = new XiangliuEnemy(180, 0, 64, 0);
            if (!g_entityManager.addRole(enemy)) delete enemy;
            break;
        }
        case DebugEnemyType::BOSS_HUNDUN: {
            IRole* enemy = new HundunEnemy(180, 0, 60, 0);
            if (!g_entityManager.addRole(enemy)) delete enemy;
            break;
        }
        case DebugEnemyType::MIXED_NORMAL: {
            for (int i = 0; i < 2; i++) {
                IRole* chimei = new ChiMeiEnemy(124 + i*20, i*24+1, 90 + i*10, i*24+1);
                if (!g_entityManager.addRole(chimei)) delete chimei;
            }
            for (int i = 0; i < 2; i++) {
                IRole* feilian = new FeilianEnemy(140 + i*20, (i+1)*20, 100 + i*10, (i+1)*20);
                if (!g_entityManager.addRole(feilian)) delete feilian;
            }
            IRole* gudiao = new GudiaoEnemy(160, 32, 110, 32);
            if (!g_entityManager.addRole(gudiao)) delete gudiao;
            break;
        }
    }
}
#endif // DEBUG_MODE_ENABLED
/***********/

/********************************************************************/
#ifdef __cplusplus
extern "C" {
#endif


void gameControlThread(void *argument) {
    //初始是Game Over状态
    g_entityManager.isGameOver = true;

    for (;;) {
        if (g_entityManager.isGameOver) {
            //游戏结束，重置游戏进度
            g_progressManager.resetGameProgress();
        } else {
            //游戏进行中
            if (!g_perkCardManager.m_isSelecting && !g_progressManager.isPlayingOpeningCG && !g_progressManager.showBoss
                && !g_progressManager.isPlayingClearCG && !g_progressManager.PauseGame && !g_progressManager.isWaitingStartKey ) {
                // 非选卡状态且非开场动画且非展示Boss海报，更新游戏逻辑
                // 更新游戏进度
                g_progressManager.updateGameProgress();
                // 更新所有角色和子弹的动作和状态
                g_entityManager.updateAllRolesActions();
                g_entityManager.updateAllBulletsActions();
                g_entityManager.updateAllRolesState();
                g_entityManager.updateAllBulletsState();
                // 清除需要回收的角色和子弹
                g_entityManager.cleanupInvalidRoles();
                g_entityManager.cleanupInvalidBullets();
            }
        }

#if DEBUG_MODE_ENABLED
        // 调试模式：敌人全灭后自动重新生成
        debugSpawnEnemies();
#endif

        osDelay(controlDelayTime);
    }
}

#ifdef __cplusplus
}
#endif
/********************************************************************/
