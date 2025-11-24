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
    osDelay(30); // OLED 初始化需要等待一段时间后
    OLED_Init(); // 初始化OLED

    for (;;) {
        OLED_NewFrame();
        if (!g_entityManager.isGameOver) {
            if (g_progressManager.isPlayingOpeningCG) {
                // 播放开场动画
                g_progressManager.drawOpeningCG();
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
            } else {
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

        if (!g_entityManager.isGameOver && !g_progressManager.isPlayingOpeningCG && !g_progressManager.showBoss
            && !g_progressManager.isPlayingClearCG) {
            // 游戏进行中才响应按键

            //测试触发选卡
            // if (key.m_keyButton[14] == 1) {
            //     g_perkCardManager.triggerPerkSelection();
            // }

            if (!g_perkCardManager.m_isSelecting) {
                scanDelayTime = 40; // 非选卡时恢复正常扫描频率
                pLeadingRole  = (LeadingRole *)g_entityManager.getPlayerRole();
                if (pLeadingRole != nullptr) {
                    if (key.m_keyButton[15] == 1) {
                        pLeadingRole->getData()->actionData.currentState = ActionState::MOVING;
                        pLeadingRole->getData()->actionData.moveMode     = MoveMode::LEFT; // Move left
                    } else if (key.m_keyButton[11] == 1) {
                        pLeadingRole->getData()->actionData.currentState = ActionState::MOVING;
                        pLeadingRole->getData()->actionData.moveMode     = MoveMode::DOWN; // Move down
                    } else if (key.m_keyButton[10] == 1) {
                        pLeadingRole->getData()->actionData.currentState = ActionState::MOVING;
                        pLeadingRole->getData()->actionData.moveMode     = MoveMode::UP; // Move up
                    } else if (key.m_keyButton[7] == 1) {
                        pLeadingRole->getData()->actionData.currentState = ActionState::MOVING;
                        pLeadingRole->getData()->actionData.moveMode     = MoveMode::RIGHT; // Move right
                    }
                }
            } else {
                scanDelayTime                     = 100; // 选卡时降低扫描频率，节省资源
                g_perkCardManager.m_selectedIndex = etl::min(
                    (uint8_t)(g_perkCardManager.m_selectedIndex), (uint8_t)(g_perkCardManager.m_selectedSize - 1)
                );
                g_perkCardManager.m_selectedIndex = etl::max((int16_t)(g_perkCardManager.m_selectedIndex), (int16_t)0);
                if (key.m_keyButton[11] == 1)
                    g_perkCardManager.m_selectedIndex = etl::min(
                        (uint8_t)(g_perkCardManager.m_selectedIndex + 1),
                        (uint8_t)(g_perkCardManager.m_selectedSize - 1)
                    );
                if (key.m_keyButton[10] == 1)
                    g_perkCardManager.m_selectedIndex =
                        etl::max((int16_t)(g_perkCardManager.m_selectedIndex - 1), (int16_t)0);
                if (key.m_keyButton[3] == 1) {
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
                && !g_progressManager.isPlayingClearCG) {
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
