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
        // 绘制游戏界面
        for(auto rolePtr : g_entityManager.m_roles) {
            if(rolePtr != nullptr && rolePtr->getData() != nullptr && rolePtr->getData()->img != nullptr) {
                OLED_DrawImage(rolePtr->getData()->spatialData.currentPosX,
                               rolePtr->getData()->spatialData.currentPosY,
                               rolePtr->getData()->img,
                               OLED_COLOR_NORMAL);
            }
        }
        OLED_ShowFrame();
        osDelay(20);
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
        if (key.m_keyButton[15] == 1 )
            pLeadingRole->move(-1, 0); // Move left
        if (key.m_keyButton[11] == 1 )
            pLeadingRole->move(0, 1); // Move down
        if (key.m_keyButton[10] == 1 )
            pLeadingRole->move(0, -1); // Move up
        if (key.m_keyButton[7] == 1 )
            pLeadingRole->move(1, 0); // Move right
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
    FeilianEnemy* enemyFeilian[9] = {new FeilianEnemy(124,1,80,0),
                                    new FeilianEnemy(124,24,80,24),
                                    new FeilianEnemy(124,48,80,48),
                                    new FeilianEnemy(124+30,24,95,24),
                                    new FeilianEnemy(124+30,1,95,0),
                                    new FeilianEnemy(124+30,48,95,48),
                                    new FeilianEnemy(124+30*2,24,110,24),
                                    new FeilianEnemy(124+30*2,1,110,0),
                                    new FeilianEnemy(124+30*2,48,110,48)};

    for(int i=0; i<9; i++)
        g_entityManager.addRole(enemyFeilian[i]);    

    for (;;) {
        if(g_entityManager.m_roles.size() == 1 ) {
            // 全部敌人被消灭，重新添加敌人
            for(int i=0; i<9; i++) {
                enemyFeilian[i] = new FeilianEnemy(124 + (i/3)*30, (i%3)*24 + 1, 80 + (i/3)*15, (i%3)*24);
                g_entityManager.addRole(enemyFeilian[i]);
            }
        }

        g_entityManager.updateAllRoles();
        g_entityManager.cleanupInvalidRoles();

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



