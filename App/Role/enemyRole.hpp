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
    FeilianEnemy(uint8_t startX = 164, uint8_t startY = 32, uint8_t initPosX = 96, uint8_t initPosY = 0, uint8_t level = 1);
    ~FeilianEnemy() override = default;

    void init();                                       // 只保留声明
    void think();                                      // 只保留声明
    void doAction();                                   // 只保留声明
    void die();                                        // 只保留声明
    void shoot(uint8_t x, uint8_t y, BulletType type); // 只保留声明
};

/**
 * @brief   HarpyEnemy class
 * @note    中文：哈耳庇厄 ｜ 英文：Harpy 神话典故：希腊神话中的鹰身女妖，人身鹰翼，以掠夺食物、袭击旅人闻名，常成群结队行动。
 * @note    游戏内设定：外形带翅膀的小型敌方单位，横向飞行（左右穿梭），会发射零散子弹，移动速度中等。
 */
class HarpyEnemy : public IRole {
public:
    HarpyEnemy(uint8_t startX = 164, uint8_t startY = 32, uint8_t initPosX = 96, uint8_t initPosY = 0);
    ~HarpyEnemy() override = default;

    void init();                                       // 只保留声明
    void think();                                      // 只保留声明
    void doAction();                                   // 只保留声明
    void die();                                        // 只保留声明
    void shoot(uint8_t x, uint8_t y, BulletType type); // 只保留声明
};

class ChiMeiEnemy : public IRole {
public:
    ChiMeiEnemy(uint8_t startX = 164, uint8_t startY = 32, uint8_t initPosX = 96, uint8_t initPosY = 0);
    ~ChiMeiEnemy() override = default;

    void init();                                       // 只保留声明
    void think();                                      // 只保留声明
    void doAction();                                   // 只保留声明
    void die();                                        // 只保留声明
    void shoot(uint8_t x, uint8_t y, BulletType type); // 只保留声明
};

class TaotieEnemy : public IRole {
public:
    TaotieEnemy(uint8_t startX = 164, uint8_t startY = 32, uint8_t initPosX = 96, uint8_t initPosY = 0);
    ~TaotieEnemy() override = default;

    void init();                                       // 只保留声明
    void think();                                      // 只保留声明
    void doAction();                                   // 只保留声明
    void die();                                        // 只保留声明
    void shoot(uint8_t x, uint8_t y, BulletType type); // 只保留声明
};

class TyphonEnemy : public IRole {
public:
    TyphonEnemy(uint8_t startX = 164, uint8_t startY = 32, uint8_t initPosX = 96, uint8_t initPosY = 0);
    ;
    ~TyphonEnemy() override = default;

    void init();                                       // 只保留声明
    void think();                                      // 只保留声明
    void doAction();                                   // 只保留声明
    void die();                                        // 只保留声明
    void shoot(uint8_t x, uint8_t y, BulletType type); // 只保留声明
};

class XiangliuEnemy : public IRole {
public:
    XiangliuEnemy(uint8_t startX = 164, uint8_t startY = 32, uint8_t initPosX = 96, uint8_t initPosY = 0);
    ~XiangliuEnemy() override = default;

    void init();                                       // 只保留声明
    void think();                                      // 只保留声明
    void doAction();                                   // 只保留声明
    void die();                                        // 只保留声明
    void shoot(uint8_t x, uint8_t y, BulletType type); // 只保留声明
};

#endif // ENEMYROLE_HPP
