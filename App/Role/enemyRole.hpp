#ifndef ENEMYROLE_HPP
#define ENEMYROLE_HPP

#include "role.hpp"

//controlDelayTime 由 threads.cpp 控制线程定义
//controlDelayTime = 10
//设计冷却和热量机制查看role.cpp
//射击冷却时间=resetTime/ (Speed) ms
//热量冷却速率= heatCoolDownRate 每次冷却时间间隔由200ms

//普通子弹热量消耗倍率 1
//火球弹热量消耗倍率 2
//闪电链弹热量消耗倍率 1.5

//role.cpp中的createBullet决定发射子弹的数值和机制
//普通子弹击中敌人后造成伤害， attackPower 点伤害
//火球弹击中敌人后对击中的敌人造成一次伤害，并在一定范围内造成范围伤害（击中的敌人也会受到范围伤害）
//两次伤害均为 attackPower +10 点伤害

//闪电链弹一束条的范围穿透伤害，mul*attackPower+10 点伤害

uint8_t const boundary_deadzone = 5; // 左侧边界
/***************小型敌人***************/
/**
 * @brief FeilianEnemy class
 * @note  中文：飞廉 ｜ 英文：Feilian,神话典故：中国古代神话中的风神，形如鹿、头生角、有翼，行走如飞，负责掌管八面来风。
 * @note  高速移动的小型敌人，体型小巧（12x12 像素），移动轨迹飘忽（呼应 “风” 的特性），单次攻击伤害低，但成群出现时压迫感强。
 * @note  只会发射普通子弹，但行动方式飘忽不定。
 */
class FeilianEnemy : public IRole {
public:
    uint16_t              think_count          = 0;
    static const uint16_t feilianEnemyDeadTime = 500; // 死亡动画时间，单位ms
public:
    FeilianEnemy(
        uint8_t startX = 164, uint8_t startY = 32, uint8_t initPosX = 96, uint8_t initPosY = 0, uint8_t level = 1 , uint16_t dropExp = 0
    );
    ~FeilianEnemy() = default;

    void drawRole() override;                                   // 只保留声明
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
 * @note  攻击方式为发射高伤害普通子弹，每次从身体中心的两侧发射，攻击速度低。
 * @note  只会发射普通子弹，但行动方式较为直接，死亡后会发出一颗火球弹。
 */
class GudiaoEnemy : public IRole {
public:
    uint16_t              think_count         = 0;
    static const uint16_t gudiaoEnemyDeadTime = 500; // 死亡动画时间，单位ms

public:
    GudiaoEnemy(
        uint8_t startX = 164, uint8_t startY = 32, uint8_t initPosX = 96, uint8_t initPosY = 0, uint8_t level = 1 , uint16_t dropExp = 0
    );
    ~GudiaoEnemy() = default;

    void drawRole() override;                                   // 只保留声明
    void init() override;                                       // 只保留声明
    void think() override;                                      // 只保留声明
    void doAction() override;                                   // 只保留声明
    void die() override;                                        // 只保留声明
    void shoot(uint8_t x, uint8_t y, BulletType type) override; // 只保留声明
};

/**
 * @brief ChiMeiEnemy class
 * @note  中文：魑魅 ｜ 英文：ChiMei,神话典故：传说中息于山林间的妖怪，善于迷惑人心，引诱迷路的旅人深入山林，最终将其吞噬。
 * @note  高速移动的小型敌人，体型极小（8x8 像素），自杀式冲撞。
 */

class ChiMeiEnemy : public IRole {
    uint16_t              think_count         = 0;
    static const uint16_t chimeiEnemyDeadTime = 300; // 死亡动画时间，单位ms

public:
    ChiMeiEnemy(
        uint8_t startX = 164, uint8_t startY = 32, uint8_t initPosX = 96, uint8_t initPosY = 0, uint8_t level = 1 , uint16_t dropExp = 0
    );
    ~ChiMeiEnemy() = default;

    void drawRole() override;                                   // 只保留声明
    void init() override;                                       // 只保留声明
    void think() override;                                      // 只保留声明
    void doAction() override;                                   // 只保留声明
    void die() override;                                        // 只保留声明
    void shoot(uint8_t x, uint8_t y, BulletType type) override; // 只保留声明
};

/**
 * @brief TaotieEnemy class
 * @note  中文：饕餮 ｜ 英文：Taotie,神话典故：四凶之一，羊身人面、眼在腋下、虎齿人爪，声音似婴儿；
 * @note  上古 “四凶” 之一，贪婪无度，能吞食天地万物，专食人类与牲畜，象征极致贪欲。
 * @note  BOSS级大型敌人，体型巨大（64x64 像素），高血量，高攻击力，低速移动，攻击方式多样且具有威胁性，擅长近战。
 * @note  攻击方式1，将玩家向自己拉近，进行吞噬攻击 
 * @note  攻击方式2，发射三排普通子弹
 * @note  攻击方式3，向前冲撞，进行撞击攻击
 * @note  攻击方式4, 向后碾压，从玩家左侧出现，进行碾压攻击
 * @note  攻击方式5, 将玩家向自己拉近，同时向前冲撞
 */

class TaotieEnemy : public IRole {
public:
    uint16_t              think_count         = 0;
    static const uint16_t TaotieEnemyDeadTime = 700; // 死亡动画时间，单位ms

    uint16_t action_timer   = 0; //倒计时
    uint16_t action_MaxTime = 0; //记录动作最大持续时间
    uint16_t action_count   = 0; //记录动作持续时间

    //拉近玩家相关参数
    static const uint8_t pullDistance = 30; // 拉近距离

    // 冲锋相关参数
    static const uint8_t chargeDistance = 30; // 冲锋移动距离

    // 碾压记录
    static const uint8_t crushChargeDistance = 100; // 碾压出现位置距离玩家左侧距离
    bool                 appearedForCrush    = false;
    bool                 comeBackForCrush    = false;

    // 攻击模式5
    // 冲锋并冲锋相关参数
    static const uint8_t pullAndChargeDistance = 50; // 拉近并冲锋移动距离
    //实际向前冲锋距离为 pullAndChargeDistance/2

public:
    TaotieEnemy(
        uint8_t startX = 164, uint8_t startY = 32, uint8_t initPosX = 96, uint8_t initPosY = 0, uint8_t level = 1 , uint16_t dropExp = 0
    );
    ~TaotieEnemy() = default;

    void drawRole() override;                                   // 只保留声明
    void init() override;                                       // 只保留声明
    void think() override;                                      // 只保留声明
    void doAction() override;                                   // 只保留声明
    void die() override;                                        // 只保留声明
    void shoot(uint8_t x, uint8_t y, BulletType type) override; // 只保留声明

    void pullPlayerAndDevourAttack();        // 吸引玩家并进行吞噬攻击，攻击方式1
    void fireThreeRowsBasicBullets();        // 发射三排普通子弹，攻击方式2
    void chargeForwardAndRamAttack();        // 向前冲撞并进行撞击攻击，攻击方式3
    void appearLeftAndRollBackCrushAttack(); // 从左侧出现并进行碾压攻击，攻击方式4
    void pullPlayerAndChargeForwardAttack(); // 吸引玩家并向前冲撞攻击，攻击方式5
};

/**
 * @brief TaowuEnemy class
 * @note  中文：饕餮 ｜ 英文：Taowu,神话典故：四凶之一，虎形犬毛、人面猪口、尾长一丈八尺；
 * @note  上古 “四凶” 之一，性格顽劣不可教化，在荒野中搅乱秩序、捕食人类，代表凶暴与叛逆。
 * @note  BOSS级大型敌人，体型巨大（64x64 像素），低血量，高攻击力，高速移动，攻击方式多样且具有威胁性，擅长闪现移动与大量弹幕。
 * @note  攻击方式1，闪现至中间位置，随机位置发射大量普通子弹，血量越低，发射持续时间越长，基础时间为3秒，最多为6秒，每秒发射5发
 * @note  攻击方式2，随机位置发射5个火球弹，持续时间3s
 * @note  攻击方式3，中间发射一颗火球弹，最边缘两侧发射各两颗普通子弹
 * @note  攻击方式4，发射一排特殊阵型的子弹，只有中间有缺口
 * @note  攻击方式5，原地留下火球弹并消失
 * @note  攻击方式6，定向闪现，对齐玩家位置闪现，同时消除CD
 */

class TaowuEnemy : public IRole {
    uint16_t              think_count        = 0;
    static const uint16_t TaowuEnemyDeadTime = 700; // 死亡动画时间，单位ms

    uint16_t action_timer   = 0; //倒计时
    uint16_t action_MaxTime = 0; //记录动作最大持续时间
    uint16_t action_count   = 0; //记录动作持续时间

    bool positionChange = false;
    // 攻击方式1相关参数
    static const uint16_t MassiveBasicBulletFireTime = 3000; // 最短发射时间，单位ms
    static const uint8_t  BulletsPerSecond           = 5;    // 发射频率，单位ms

    // 攻击方式2相关参数
    static const uint16_t FiveFireballBulletFireTime = 3000; // 发射时间，单位ms
    static const uint8_t  FireballCount              = 5;    // 发射数量

    // 攻击方式3相关参数
    static const uint8_t CenterFireballAttackTime = 100; // 发射时间，单位ms

public:
    TaowuEnemy(
        uint8_t startX = 164, uint8_t startY = 32, uint8_t initPosX = 96, uint8_t initPosY = 0, uint8_t level = 1 , uint16_t dropExp = 0
    );
    ;
    ~TaowuEnemy() = default;

    void drawRole() override;                                   // 只保留声明
    void init() override;                                       // 只保留声明
    void think() override;                                      // 只保留声明
    void doAction() override;                                   // 只保留声明
    void die() override;                                        // 只保留声明
    void shoot(uint8_t x, uint8_t y, BulletType type) override; // 只保留声明

    void fireContinuousMassiveBasicBullets();     // 攻击方式1，随机位置发射大量普通子弹
    void fireFiveFireballBulletsAtRandom();       // 攻击方式2，随机位置发射5个火球弹
    void fireCenterFireballAndSideBasicBullets(); // 攻击方式3，中间发射一颗火球弹，最边缘两侧发射各两颗普通子弹
    void fireSingleRowNotchedBasicBullets();      // 攻击方式4，发射一排特殊阵型的子弹，只有中间有缺口
    void blinkToRandomPosition();                 // 攻击方式5，原地留下火球弹并消失
    void blinkToPlayerAlignedPosition();          // 攻击方式6，定向闪现，对齐玩家位置闪现
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
    uint16_t              think_count           = 0;
    static const uint16_t XiangliuEnemyDeadTime = 700; // 死亡动画时间，单位ms

    uint16_t action_timer   = 0; //倒计时
    uint16_t action_MaxTime = 0; //记录动作最大持续时间
    uint16_t action_count   = 0; //记录动作持续时间


public:
    XiangliuEnemy(
        uint8_t startX = 164, uint8_t startY = 32, uint8_t initPosX = 96, uint8_t initPosY = 0, uint8_t level = 1 , uint16_t dropExp = 0
    );
    ~XiangliuEnemy() = default;

    void drawRole() override;                                   // 只保留声明
    void init() override;                                       // 只保留声明
    void think() override;                                      // 只保留声明
    void doAction() override;                                   // 只保留声明
    void die() override;                                        // 只保留声明
    void shoot(uint8_t x, uint8_t y, BulletType type) override; // 只保留声明


    void fireNineRowsBasicBullets();      // 攻击方式1，发射九排普通子弹
    void fireThreeRowsLightningBullets(); // 攻击方式2，发射三排闪电
    void fireThreeRowsFireballBullets();  // 攻击方式3，发射三排火球弹
    void summonThreeChiMeiMinions();       // 攻击方式4，生成3只ChiMeiEnemy作为召唤物协同作战
    void summonTwoFeilianMinions();        // 攻击方式5，生成2只FeilianEnemy作为召唤物协同作战
    void summonOneGudiaoMinion();          // 攻击方式6，生成1只GudiaoEnemy作为召唤物协同作战

};

#endif // ENEMYROLE_HPP
