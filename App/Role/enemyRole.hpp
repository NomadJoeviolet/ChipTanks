#ifndef ENEMYROLE_HPP
#define ENEMYROLE_HPP

#include "role.hpp"
//#include "task.h"

/***************小型敌人***************/

/**
 * @brief FeilianEnemy class
 * @note  中文：飞廉 ｜ 英文：Feilian,神话典故：中国古代神话中的风神，形如鹿、头生角、有翼，行走如飞，负责掌管八面来风。
 * @note  高速移动的小型敌人，体型小巧（12x12 像素），移动轨迹飘忽（呼应 “风” 的特性），单次攻击伤害低，但成群出现时压迫感强。
 */
class FeilianEnemy : public IRole {
public:
    uint8_t action_count = 0;

public:
    FeilianEnemy(uint8_t startX = 164, uint8_t startY = 32, uint8_t initPosX = 96, uint8_t initPosY = 0) {
        m_pdata->identity = RoleIdentity::ENEMY;
        m_pdata->img      = &feilianImg;

        m_pdata->spatialData.currentPosX = startX; // Starting X position
        m_pdata->spatialData.currentPosY = startY; // Starting Y position
        m_pdata->initData.posX           = initPosX;
        m_pdata->initData.posY           = initPosY;

        m_pdata->spatialData.refPosX = startX;
        m_pdata->spatialData.refPosY = startY;

        m_pdata->spatialData.sizeX = m_pdata->img->w;
        m_pdata->spatialData.sizeY = m_pdata->img->h;

        m_pdata->spatialData.moveSpeed = 1; // Set mo1ement speed

        m_pdata->isActive = true;

        m_pdata->attackData.attackPower = 5 ;
        m_pdata->attackData.shootCooldownSpeed = 50 ;
        m_pdata->attackData.shootCooldownTimer = 0 ;
        m_pdata->attackData.shootCooldownResetTime = 10000  ;
        m_pdata->attackData.shootCooldownResetTime = 500 / controlDelayTime ; // 500ms cooldown
        m_pdata->attackData.bulletSpeed = 1 ;
        
        m_pdata->heatData.maxHeat = 100 ;
        m_pdata->heatData.currentHeat = 0 ;
        m_pdata->heatData.heatPerShot = 20 ;
        m_pdata->heatData.heatCoolDownRate = 10 ;

        // Initialize other enemy-specific data here
    }
    
    ~FeilianEnemy() override = default;

    
    void init() ;  // 只保留声明
    void think() ;  // 只保留声明
    void doAction() ;  // 只保留声明
    void die() ;  // 只保留声明
    void shoot() ;  // 只保留声明

};

/**
 * @brief   HarpyEnemy class
 * @note    中文：哈耳庇厄 ｜ 英文：Harpy 神话典故：希腊神话中的鹰身女妖，人身鹰翼，以掠夺食物、袭击旅人闻名，常成群结队行动。
 * @note    游戏内设定：外形带翅膀的小型敌方单位，横向飞行（左右穿梭），会发射零散子弹，移动速度中等。
 */
class HarpyEnemy : public IRole {
public:
    HarpyEnemy() {
        m_pdata->identity = RoleIdentity::ENEMY;
        // Initialize other enemy-specific data here
    }
    ~HarpyEnemy() override = default;

    void init() override {
        // Initialize enemy role specifics
    }

    // void move(int8_t dirX, int8_t dirY) override {
    //     // Implement enemy movement logic
    // }

    void shoot() override {
        // Implement enemy shooting logic
    }
};

class ChiMeiEnemy : public IRole {
public:
    ChiMeiEnemy() {
        m_pdata->identity = RoleIdentity::ENEMY;
        // Initialize other enemy-specific data here
    }
    ~ChiMeiEnemy() override = default;

    void init() override {
        // Initialize enemy role specifics
    }

    // void move(int8_t dirX, int8_t dirY) override {
    //     // Implement enemy movement logic
    // }

    void shoot() override {
        // Implement enemy shooting logic
    }
};

class TaotieEnemy : public IRole {
public:
    TaotieEnemy() {
        m_pdata->identity = RoleIdentity::ENEMY;
        // Initialize other enemy-specific data here
    }
    ~TaotieEnemy() override = default;

    void init() override {
        // Initialize enemy role specifics
    }

    // void move(int8_t dirX, int8_t dirY) override {
    //     // Implement enemy movement logic
    // }

    void shoot() override {
        // Implement enemy shooting logic
    }
};

class TyphonEnemy : public IRole {
public:
    TyphonEnemy() {
        m_pdata->identity = RoleIdentity::ENEMY;
        // Initialize other enemy-specific data here
    }
    ~TyphonEnemy() override = default;

    void init() override {
        // Initialize enemy role specifics
    }

    // void move(int8_t dirX, int8_t dirY) override {
    //     // Implement enemy movement logic
    // }

    void shoot() override {
        // Implement enemy shooting logic
    }
};

class XiangliuEnemy : public IRole {
public:
    XiangliuEnemy() {
        m_pdata->identity = RoleIdentity::ENEMY;
        // Initialize other enemy-specific data here
    }
    ~XiangliuEnemy() override = default;

    void init() override {
        // Initialize enemy role specifics
    }

    // void move(int8_t dirX, int8_t dirY) override {
    //     // Implement enemy movement logic
    // }

    void shoot() override {
        // Implement enemy shooting logic

    }
};

#endif // ENEMYROLE_HPP
