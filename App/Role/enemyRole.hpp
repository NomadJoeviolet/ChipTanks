#ifndef ENEMYROLE_HPP
#define ENEMYROLE_HPP

#include "role.hpp"
//#include "task.h"

/***************小型敌人***************/

/**
 * @brief FeilianEnemy class
 * @note  中文：飞廉 ｜ 英文：Feilian,神话典故：中国古代神话中的风神，形如鹿、头生角、有翼，行走如飞，负责掌管八面来风。
 * @note  高速移动的小型敌人，体型小巧（12x12 像素），移动轨迹飘忽（呼应 “风” 的特性），单次攻击伤害低，但成群出现时压迫感强。
 * @note  只会发射普通子弹，但行动方式飘忽不定。
 */
class FeilianEnemy : public IRole {
public:
    uint8_t action_count = 0;

public:
    FeilianEnemy(uint8_t startX = 164, uint8_t startY = 32, uint8_t initPosX = 96, uint8_t initPosY = 0, uint8_t level = 1);
    ~FeilianEnemy() override = default;

    static const uint16_t feilianEnemyDeadTime = 500 ; // 死亡动画时间，单位ms

    void drawRole() override;                          // 只保留声明
    void init() override;                                       // 只保留声明
    void think() override;                                      // 只保留声明
    void doAction() override;                                   // 只保留声明
    void die() override;                                        // 只保留声明
    void shoot(uint8_t x, uint8_t y, BulletType type) override; // 只保留声明
};

/**
 * @brief GudiaoEnemy class
 * @note  中文：蛊雕 ｜ 英文：Gudiao,神话典故：山海经中大型猛禽凶兽，以哭声诱捕人类，擅长飞行捕猎，是山中食人恶兽的代表。
 * @note  中速飞行的中型敌人，体型居中（15x15 像素），攻击力较高，适合伏击玩家。
 * @note  攻击方式为发射高伤害普通子弹，每次从身体两侧发射。
 */
class GudiaoEnemy : public IRole {
public:
    GudiaoEnemy(uint8_t startX = 164, uint8_t startY = 32, uint8_t initPosX = 96, uint8_t initPosY = 0);
    ~GudiaoEnemy() override = default;

    void init();                                       // 只保留声明
    void think();                                      // 只保留声明
    void doAction();                                   // 只保留声明
    void die();                                        // 只保留声明
    void shoot(uint8_t x, uint8_t y, BulletType type); // 只保留声明
};

/**
 * @brief ChiMeiEnemy class
 * @note  中文：魑魅 ｜ 英文：ChiMei,神话典故：传说中息于山林间的妖怪，善于迷惑人心，引诱迷路的旅人深入山林，最终将其吞噬。
 * @note  高速移动的小型敌人，体型极小（8x8 像素），自杀式冲撞。
 */

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

/**
 * @brief TaotieEnemy class
 * @note  中文：饕餮 ｜ 英文：Taotie,神话典故：四凶之一，羊身人面、眼在腋下、虎齿人爪，声音似婴儿；
 * @note  上古 “四凶” 之一，贪婪无度，能吞食天地万物，专食人类与牲畜，象征极致贪欲。
 * @note  BOSS级大型敌人，体型巨大（64x64 像素），高血量，高攻击力，中速移动，攻击方式多样且具有威胁性。
 * @note  攻击方式1，将玩家向自己拉近，进行吞噬攻击 
 * @note  攻击方式2，发射三排普通子弹
 * @note  攻击方式3，向前冲撞，进行撞击攻击
 * @note  攻击方式4, 向后碾压，从玩家左侧出现，进行碾压攻击
 * @note  攻击方式5, 将玩家向自己拉近，同时向前冲撞
 */

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


/**
 * @brief TaowuEnemy class
 * @note  中文：饕餮 ｜ 英文：Taowu,神话典故：四凶之一，虎形犬毛、人面猪口、尾长一丈八尺；
 * @note  上古 “四凶” 之一，性格顽劣不可教化，在荒野中搅乱秩序、捕食人类，代表凶暴与叛逆。
 * @note  BOSS级大型敌人，体型巨大（64x64 像素），高血量，高攻击力，中速移动，攻击方式多样且具有威胁性。
 * @note  攻击方式1，随机位置发射大量普通子弹
 * @note  攻击方式2，随机位置发射5个火球弹
 * @note  攻击方式3，闪现移动
 
 */
class TaowuEnemy : public IRole {
public:
    TaowuEnemy(uint8_t startX = 164, uint8_t startY = 32, uint8_t initPosX = 96, uint8_t initPosY = 0);
    ;
    ~TaowuEnemy() override = default;
    void init();                                       // 只保留声明
    void think();                                      // 只保留声明
    void doAction();                                   // 只保留声明
    void die();                                        // 只保留声明
    void shoot(uint8_t x, uint8_t y, BulletType type); // 只保留声明
};


/**
 * @brief XiangliuEnemy class
 * @note  中文：相柳 ｜ 英文：Xiangliu,神话典故：九头蛇形怪兽，居于洪水之中，毒气弥漫，所到之处草木皆枯，河流干涸。
 * @note  九头蛇形怪兽，能喷射剧毒，所到之处草木皆枯，河流干涸，象征灾难与毁灭。
 * @note  BOSS级大型敌人，体型巨大（64x64 像素），高血量，高攻击力，中速移动，攻击方式多样且具有威胁性。
 * @note  攻击方式1，发射九排普通子弹
 * @note  攻击方式2，发射三排闪电
 * @note  攻击方式3，发射三排火球弹
 * @note  攻击方式4，生成3只ChiMeiEnemy作为召唤物协同作战
 * @note  攻击方式5，生成2只FeilianEnemy作为召唤物协同作战
 * @note  攻击方式6，生成1只GudiaoEnemy作为召唤物协同作战
 */
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
