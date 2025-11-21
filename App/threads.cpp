#include "main.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h"

#include "etl/vector.h"

#include "Bullet/bullet.hpp"
#include "Role/leadingRole.hpp"
#include "Role/enemyRole.hpp"
#include "gameEntityManager.hpp"
#include "font.h"

GameEntityManager g_entityManager ;
LeadingRole* pLeadingRole = new LeadingRole();

uint8_t controlDelayTime = 10 ; // 控制线程延时，单位ms


/*******************************oled*********************************/
#include "oled.h"
/********************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

void oledTaskThread(void *argument) {
    osDelay(30); // OLED 初始化需要等待一段时间后
    OLED_Init();// 初始化OLED

    for (;;) {
        OLED_NewFrame();

        //显示角色信息
        char infoStr[32];
        sprintf(infoStr, "HP:(%d/%d) Lv:%d", pLeadingRole->getData()->healthData.currentHealth, pLeadingRole->getData()->healthData.maxHealth, pLeadingRole->getData()->level);
        OLED_PrintString(0,56, infoStr, &font8x6 , OLED_COLOR_NORMAL);

        // 绘制游戏界面
        g_entityManager.drawAllRoles();
        g_entityManager.drawAllBullets();


         // 调试信息显示
        OLED_ShowFrame();
        osDelay(controlDelayTime);
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
    key.init();
    for (;;) {
        key.scan();
        if (key.m_keyButton[15] == 1 ) {
            pLeadingRole->getData()->actionData.currentState = ActionState::MOVING;
            pLeadingRole->getData()->actionData.moveMode = MoveMode::LEFT; // Move left
        }
        else if (key.m_keyButton[11] == 1 ) {
            pLeadingRole->getData()->actionData.currentState = ActionState::MOVING;
            pLeadingRole->getData()->actionData.moveMode = MoveMode::DOWN; // Move down
        }
        else if (key.m_keyButton[10] == 1 ) {
            pLeadingRole->getData()->actionData.currentState = ActionState::MOVING;
            pLeadingRole->getData()->actionData.moveMode = MoveMode::UP; // Move up
        }
        else if (key.m_keyButton[7] == 1 ) {
            pLeadingRole->getData()->actionData.currentState = ActionState::MOVING;
            pLeadingRole->getData()->actionData.moveMode = MoveMode::RIGHT; // Move right
        }
        osDelay(40);
    }
}


#ifdef __cplusplus
}
#endif
/********************************************************************/



/****************************gameControl*****************************/
#include "enemyRole.hpp"
#include "leadingRole.hpp"

uint8_t debugCurrentPosX = 0 ;
uint8_t debugCurrentPosY = 0 ;

uint8_t debugEnemyPosX[9] = {} ;
uint8_t debugEnemyPosY[9] = {} ;

/********************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

void gameControlThread(void *argument) {
     
     g_entityManager.addRole(pLeadingRole);
    
    // 添加一些敌人角色进行测试

    for (;;) {
        if(g_entityManager.m_roles.size() == 1 ) {
            // 全部敌人被消灭，重新添加敌人
            for(int i=0; i< 9 ; i++) {
                IRole* enemyFeilian = new FeilianEnemy(124 + (i/3)*30, (i%3)*24 + 1, 80 + (i/3)*15, (i%3)*24);
                if(!g_entityManager.addRole(enemyFeilian)) {
                    delete enemyFeilian ;
                }
            }
        }

        g_entityManager.updateAllRolesActions();
        g_entityManager.updateAllBulletsActions();
        g_entityManager.updateAllRoles();
        g_entityManager.updateAllBullets();
        g_entityManager.cleanupInvalidRoles();
        g_entityManager.cleanupInvalidBullets();

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



