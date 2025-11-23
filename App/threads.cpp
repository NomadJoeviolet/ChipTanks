#include "main.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h"

#include "etl/vector.h"
#include "etl/algorithm.h"

#include "font.h"

#include "Bullet/bullet.hpp"
#include "Role/leadingRole.hpp"
#include "Role/enemyRole.hpp"

#include "gamePerkCardManager.hpp"
#include "gameEntityManager.hpp"

GameEntityManager   g_entityManager;
GamePerkCardManager g_perkCardManager;
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

        if (g_perkCardManager.m_isSelecting && g_perkCardManager.isInited) {
            // 处于选卡状态，显示选卡界面
            g_perkCardManager.drawSelectionUI();

        } else {
            // 非选卡状态，显示游戏界面

            //显示角色信息
            char infoStr[32];
            sprintf(
                infoStr, "HP:%d/%d Lv.%d EXP:%d", pLeadingRole->getData()->healthData.currentHealth,
                pLeadingRole->getData()->healthData.maxHealth, pLeadingRole->getData()->level,
                pLeadingRole->experiencePoints
            );
            OLED_PrintString(0, 56, infoStr, &font8x6, OLED_COLOR_NORMAL);

            // 绘制游戏界面
            g_entityManager.drawAllRoles();
            g_entityManager.drawAllBullets();
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
        if (key.m_keyButton[14] == 1) {
            g_perkCardManager.triggerPerkSelection();
        }
        if (!g_perkCardManager.m_isSelecting) {
            pLeadingRole = (LeadingRole *)g_entityManager.getPlayerRole();
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
            scanDelayTime = 100; // 选卡时降低扫描频率，节省资源
            if (key.m_keyButton[11] == 1)
                g_perkCardManager.m_selectedIndex = etl::min(
                    (uint8_t)(g_perkCardManager.m_selectedIndex + 1), (uint8_t)(g_perkCardManager.m_selectedSize - 1)
                );
            if (key.m_keyButton[10] == 1)
                g_perkCardManager.m_selectedIndex =
                    etl::max((int16_t)(g_perkCardManager.m_selectedIndex - 1), (int16_t)0);
            if(key.m_keyButton[3] == 1) {
                g_perkCardManager.selectCard(g_perkCardManager.m_selectedIndex);
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

uint8_t debugCurrentPosX = 0;
uint8_t debugCurrentPosY = 0;

uint8_t debugEnemyPosX[9] = {};
uint8_t debugEnemyPosY[9] = {};
IRole  *debugRole         = nullptr;

TaowuEnemy   *enemyTaotu   = new TaowuEnemy;
TaotieEnemy  *enemyTaotie  = new TaotieEnemy;
FeilianEnemy *enemyFeilian = new FeilianEnemy;
GudiaoEnemy  *enemyGudiao  = new GudiaoEnemy;
ChiMeiEnemy  *enemyChiMei  = new ChiMeiEnemy;

/********************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

void gameControlThread(void *argument) {
    // 创建主角角色
    pLeadingRole = new LeadingRole();
    g_entityManager.addRole(pLeadingRole);

    // 初始化Perk卡片管理器
    g_perkCardManager.initWarehouse();

    // 添加一些敌人角色进行测试
    for (;;) {
        if (g_entityManager.m_roles.size() == 1) {
            // 全部敌人被消灭，重新添加敌人

            // 普通敌人测试
            // for(int i=0; i< 3 ; i++) {
            //     IRole* enemyChiMei = new ChiMeiEnemy(124 + (i/3)*30, (i%3)*24+1 , 90 + (i/3)*15, (i%3)*24+1 );
            //     if(!g_entityManager.addRole(enemyChiMei)) {
            //         delete enemyChiMei ;
            //     }
            // }

            // for(int i=0; i< 3 ; i++) {
            //     IRole* enemyFeilian = new FeilianEnemy(140 + (i/3)*30, (i%3)*24+1 , 90 + (i/3)*15, (i%3)*24+1 );
            //     if(!g_entityManager.addRole(enemyFeilian)) {
            //         delete enemyFeilian ;
            //     }
            // }
            // IRole* enemyGudiao = new GudiaoEnemy(156, 32, 100 , 26 );
            // if(!g_entityManager.addRole(enemyGudiao)) {
            //     delete enemyGudiao ;
            // }

            // BOSS饕餮测试
            IRole *enemyTaotie = new TaotieEnemy(180, 0, 64, 0);
            if (!g_entityManager.addRole(enemyTaotie)) {
                delete enemyTaotie;
            }

            // //BOSS梼杌测试
            // IRole *enemyTaowu = new TaowuEnemy(180, 0, 64, 0);
            // if (!g_entityManager.addRole(enemyTaowu)) {
            //     delete enemyTaowu;
            // }

            // // BOSS相柳测试
            // IRole *enemyXiangliu = new XiangliuEnemy(180, 0, 64, 0);
            // if (!g_entityManager.addRole(enemyXiangliu)) {
            //     delete enemyXiangliu;
            // }

            //debugRole = enemyTaowu;
        }

        g_entityManager.updateAllRolesActions();
        g_entityManager.updateAllBulletsActions();
        g_entityManager.updateAllRolesState();
        g_entityManager.updateAllBulletsState();
        g_entityManager.cleanupInvalidRoles();
        g_entityManager.cleanupInvalidBullets();

        if (debugRole != nullptr) {
            debugCurrentPosX = debugRole->getData()->spatialData.currentPosX;
            debugCurrentPosY = debugRole->getData()->spatialData.currentPosY;
        }

        // debugCurrentPosX = pLeadingRole->getData()->spatialData.currentPosX ;
        // debugCurrentPosY = pLeadingRole->getData()->spatialData.currentPosY;
        // for(int i=0; i<9; i++) {
        //     debugEnemyPosX[i] = enemyFeilian[i]->getData()->spatialData.currentPosX ;
        //     debugEnemyPosY[i] = enemyFeilian[i]->getData()->spatialData.currentPosY ;
        // }

        osDelay(controlDelayTime);
    }
}

#ifdef __cplusplus
}
#endif
/********************************************************************/
