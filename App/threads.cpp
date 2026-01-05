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
                        if (g_progressManager.PauseTimer >= 500) {
                            g_progressManager.PauseGame = !g_progressManager.PauseGame;
                            g_progressManager.PauseTimer = 0;
                        }
                    }
                    
                }
            } else {
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
//测试用代码
// uint8_t debugCurrentPosX = 0;
// uint8_t debugCurrentPosY = 0;

// uint8_t debugEnemyPosX[9] = {};
// uint8_t debugEnemyPosY[9] = {};
// IRole  *debugRole         = nullptr;

// TaowuEnemy   *enemyTaotu   = new TaowuEnemy;
// TaotieEnemy  *enemyTaotie  = new TaotieEnemy;
// FeilianEnemy *enemyFeilian = new FeilianEnemy;
// GudiaoEnemy  *enemyGudiao  = new GudiaoEnemy;
// ChiMeiEnemy  *enemyChiMei  = new ChiMeiEnemy;
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

        // // 添加一些敌人角色进行测试
        // if (g_entityManager.m_roles.size() == 1 && !g_entityManager.isGameOver ) {
        //     // 全部敌人被消灭，重新添加敌人

        //     // 普通敌人测试
        //     // for(int i=0; i< 3 ; i++) {
        //     //     IRole* enemyChiMei = new ChiMeiEnemy(124 + (i/3)*30, (i%3)*24+1 , 90 + (i/3)*15, (i%3)*24+1 );
        //     //     if(!g_entityManager.addRole(enemyChiMei)) {
        //     //         delete enemyChiMei ;
        //     //     }
        //     // }

        //     // for(int i=0; i< 3 ; i++) {
        //     //     IRole* enemyFeilian = new FeilianEnemy(140 + (i/3)*30, (i%3)*24+1 , 90 + (i/3)*15, (i%3)*24+1 );
        //     //     if(!g_entityManager.addRole(enemyFeilian)) {
        //     //         delete enemyFeilian ;
        //     //     }
        //     // }
        //     // IRole* enemyGudiao = new GudiaoEnemy(156, 32, 100 , 26 );
        //     // if(!g_entityManager.addRole(enemyGudiao)) {
        //     //     delete enemyGudiao ;
        //     // }

        //     // // BOSS饕餮测试
        //     // IRole *enemyTaotie = new TaotieEnemy(180, 0, 64, 0);
        //     // if (!g_entityManager.addRole(enemyTaotie)) {
        //     //     delete enemyTaotie;
        //     // }

        //     // //BOSS梼杌测试
        //     // IRole *enemyTaowu = new TaowuEnemy(180, 0, 64, 0);
        //     // if (!g_entityManager.addRole(enemyTaowu)) {
        //     //     delete enemyTaowu;
        //     // }

        //     // // BOSS相柳测试
        //     // IRole *enemyXiangliu = new XiangliuEnemy(180, 0, 64, 0);
        //     // if (!g_entityManager.addRole(enemyXiangliu)) {
        //     //     delete enemyXiangliu;
        //     // }

        //     //debugRole = enemyTaowu;
        // }

        osDelay(controlDelayTime);
    }
}

#ifdef __cplusplus
}
#endif
/********************************************************************/
