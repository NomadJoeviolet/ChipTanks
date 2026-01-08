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
//两次伤害均为 attackPower +30 点伤害

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
    static const uint16_t feilianEnemyDeadTime = 250; // 死亡动画时间，单位ms
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
    static const uint16_t gudiaoEnemyDeadTime = 250; // 死亡动画时间，单位ms

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
    static const uint16_t chimeiEnemyDeadTime = 250; // 死亡动画时间，单位ms

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


/*******************************************/
/***************精英级中型敌人***************/

/**
 * @brief BoEnemy class - 驳 精英敌人
 * @note  中文：驳 ｜ 英文：Bo
 * @note  神话典故：《山海经·西山经》记载："中曲之山，有兽焉，其状如马而白身黑尾，一角，虎牙爪，
 *                音如鼓音，其名曰驳，是食虎豹，可以御兵。"
 * @note  驳是一种马形神兽，白身黑尾，头生独角，有虎牙虎爪，叫声如鼓，能捕食虎豹，佩戴可御刀兵。
 * 
 * @note  精英级中型敌人，体型中等（24x24 像素），中等血量，较高攻击力，中速移动。
 * @note  会与普通敌人一同出现，攻击方式直接凶猛，无瞬移技能。
 * 
 * @note  === 攻击方式 ===
 * @note  MODE_1: 冲锋践踏 - 向玩家方向快速直线冲锋，造成碰撞伤害
 *               冲锋距离 ChargeDistance=40，冲锋持续时间 ChargeTime=800ms
 * @note  MODE_2: 虎牙利爪 - 呼应"虎牙爪"，发射3发呈扇形的普通子弹
 *               持续时间 ClawAttackTime=200ms
 * @note  MODE_3: 鼓音震荡 - 呼应"音如鼓音"，发射一排横向冲击波子弹
 *               持续时间 DrumSoundTime=300ms
 */

//数值设定参考（精英级，比普通敌人强，比BOSS弱）
//血量：80 + level * 80（中等血量）
//攻击力：6 + level * 2（较高攻击力）
//移动速度：2（中速移动）
//碰撞伤害：10 + level * 3（较高碰撞伤害）

class BoEnemy : public IRole {
public:
    uint16_t              think_count     = 0;
    static const uint16_t boEnemyDeadTime = 300; // 死亡动画时间，单位ms

    uint16_t action_timer   = 0; // 倒计时
    uint16_t action_MaxTime = 0; // 记录动作最大持续时间
    uint16_t action_count   = 0; // 记录动作持续时间

    // 攻击方式1相关参数 - 冲锋践踏
    static const uint16_t ChargeTime     = 800; // 冲锋持续时间，单位ms
    static const uint8_t  ChargeDistance = 40;  // 冲锋距离，单位像素
    int8_t                chargeDirectionX = 0; // 冲锋方向X
    int8_t                chargeDirectionY = 0; // 冲锋方向Y
    bool                  chargeStarted    = false;

    // 攻击方式2相关参数 - 虎牙利爪
    static const uint16_t ClawAttackTime = 200; // 爪击持续时间，单位ms

    // 攻击方式3相关参数 - 鼓音震荡
    static const uint16_t DrumSoundTime = 300; // 鼓音持续时间，单位ms

public:
    BoEnemy(
        uint8_t startX = 164, uint8_t startY = 32, uint8_t initPosX = 96, uint8_t initPosY = 0, uint8_t level = 1, uint16_t dropExp = 0
    );
    ~BoEnemy() = default;

    void drawRole() override;                                   // 绘制角色
    void init() override;                                       // 初始化
    void think() override;                                      // 思考决策
    void doAction() override;                                   // 执行动作
    void die() override;                                        // 死亡处理
    void shoot(uint8_t x, uint8_t y, BulletType type) override; // 发射子弹

    void chargeTowardsPlayer(); // 攻击方式1，冲锋践踏 - 向玩家方向冲锋
    void tigerClawAttack();     // 攻击方式2，虎牙利爪 - 发射扇形子弹
    void drumSoundWave();       // 攻击方式3，鼓音震荡 - 发射横向冲击波
};

/**
 * @brief LiliEnemy class - 狸力 精英敌人
 * @note  中文：狸力 ｜ 英文：Lili
 * @note  神话典故：《山海经·南山经》记载："柜山，有兽焉，其状如豚，有距，其音如狗吠，
 *                其名曰狸力，见则其县多土功。"
 * @note  狸力是一种猪形神兽，长有利爪（距），叫声如狗吠，出现则预示当地将有大量土木工程。
 * 
 * @note  精英级中型敌人，体型中等，中等血量，中等攻击力，较快移动。
 * @note  会与普通敌人一同出现，攻击方式以土系和声波为主。
 * 
 * @note  === 攻击方式 ===
 * @note  MODE_1: 土涌突刺 - 呼应"土功"，在前方间隔发射火球弹（土块爆炸）
 *               持续时间 EarthSurgeTime=1200ms，每 EarthSurgeInterval=300ms 发射一发火球
 * @note  MODE_2: 獠吠震波 - 呼应"其音如狗吠"，发射5发扇形普通子弹阵
 *               持续时间 BarkWaveTime=400ms，一次性发射
 * @note  MODE_3: 穴地陷阱 - 呼应"见则其县多土功"，在随机位置挖掘陷阱后爆炸
 *               持续时间 BurrowTrapTime=800ms，先标记2个陷阱位置，延迟后发射火球
 */

//数值设定参考（精英级，比普通敌人强，比BOSS弱）
//血量：60 + level * 60（中等血量，比驳稍弱）
//攻击力：5 + level * 2（中等攻击力）
//移动速度：2（中速移动）
//碰撞伤害：8 + level * 2（中等碰撞伤害）

class LiliEnemy : public IRole {
public:
    uint16_t              think_count       = 0;
    static const uint16_t liliEnemyDeadTime = 300; // 死亡动画时间，单位ms

    uint16_t action_timer   = 0; // 倒计时
    uint16_t action_MaxTime = 0; // 记录动作最大持续时间
    uint16_t action_count   = 0; // 记录动作持续时间

    // 攻击方式1相关参数 - 土涌突刺
    static const uint16_t EarthSurgeTime     = 1200; // 土涌持续时间，单位ms
    static const uint16_t EarthSurgeInterval = 400;  // 每发间隔，单位ms
    uint8_t               earthSurgeCount    = 0;    // 已发射火球数

    // 攻击方式2相关参数 - 獠吠震波
    static const uint16_t BarkWaveTime = 400; // 吠声持续时间，单位ms
    bool                  barkFired    = false;

    // 攻击方式3相关参数 - 穴地陷阱
    static const uint16_t BurrowTrapTime     = 800;  // 陷阱技能持续时间，单位ms
    static const uint16_t TrapExplodeDelay   = 500;  // 陷阱爆炸延迟，单位ms
    static const uint8_t  TrapCount          = 2;    // 陷阱数量
    uint8_t               trapPosX[TrapCount] = {0}; // 陷阱X位置
    uint8_t               trapPosY[TrapCount] = {0}; // 陷阱Y位置
    bool                  trapPlaced         = false;// 陷阱是否已放置
    bool                  trapExploded       = false;// 陷阱是否已爆炸

public:
    LiliEnemy(
        uint8_t startX = 164, uint8_t startY = 32, uint8_t initPosX = 96, uint8_t initPosY = 0, uint8_t level = 1, uint16_t dropExp = 0
    );
    ~LiliEnemy() = default;

    void drawRole() override;                                   // 绘制角色
    void init() override;                                       // 初始化
    void think() override;                                      // 思考决策
    void doAction() override;                                   // 执行动作
    void die() override;                                        // 死亡处理
    void shoot(uint8_t x, uint8_t y, BulletType type) override; // 发射子弹

    void earthSurge();  // 攻击方式1，土涌突刺 - 发射火球弹
    void barkWave();    // 攻击方式2，獠吠震波 - 发射扇形子弹
    void burrowTrap();  // 攻击方式3，穴地陷阱 - 随机位置爆炸陷阱
};


/*******************************************/
/***************BOSS级大型敌人***************/

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
    static const uint16_t TaotieEnemyDeadTime = 500; // 死亡动画时间，单位ms

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
 * @brief TaowuEnemy class - 梼杌 BOSS
 * @note  中文：梼杌 ｜ 英文：Taowu
 * @note  神话典故：四凶之一，虎形犬毛、人面猪口、尾长一丈八尺；
 * @note  上古 "四凶" 之一，性格顽劣不可教化，在荒野中搅乱秩序、捕食人类，代表凶暴与叛逆。
 * 
 * @note  BOSS级大型敌人，体型巨大（64x64 像素），低血量，高攻击力，高速移动，
 * @note  攻击方式多样且具有威胁性，擅长闪现移动与大量弹幕。
 * 
 * @note  === 攻击方式 === 
 * @note  MODE_1: 闪现至中间位置(63,1)，随机位置发射大量普通子弹
 *               血量越低发射持续时间越长，基础时间 MassiveBasicBulletFireTime=3000ms，最多6000ms
 *               发射频率：每 1000/BulletsPerSecond=125ms 一发
 * @note  MODE_2: 闪现至中间位置(63,1)，随机位置发射多个火球弹
 *               持续时间 FiveFireballBulletFireTime=3000ms
 *               发射频率：每 3000/FireballCount=375ms 一发，共 FireballCount=8 发
 * @note  MODE_3: 原地瞬发，中间发射一颗火球弹，最边缘两侧各发射两颗普通子弹
 *               持续时间 CenterFireballAttackTime=100ms
 * @note  MODE_4: 发射一排特殊阵型的子弹，只有中间有缺口（缺口范围 ±12像素）
 *               持续时间 NotchedBulletsAttackTime=500ms
 * @note  MODE_5: 闪现至远处(140,1)，发射3颗火球弹（随机Y位置），然后返回(63,1)
 *               持续时间 BlinkRandomTime=1000ms，分三阶段执行
 * @note  MODE_6: 定向闪现，对齐玩家Y位置，X位置随机(30-80)，攻击结束后清除CD
 *               持续时间 BlinkAlignedTime=100ms
 */

class TaowuEnemy : public IRole {
    uint16_t              think_count        = 0;
    static const uint16_t TaowuEnemyDeadTime = 500; // 死亡动画时间，单位ms

    uint16_t action_timer   = 0; //倒计时
    uint16_t action_MaxTime = 0; //记录动作最大持续时间
    uint16_t action_count   = 0; //记录动作持续时间

    bool positionChange = false;
    // 攻击方式1相关参数
    static const uint16_t MassiveBasicBulletFireTime = 3000; // 最短发射时间，单位ms
    static const uint8_t  BulletsPerSecond           = 8;    // 发射频率，单位ms

    // 攻击方式2相关参数
    static const uint16_t FiveFireballBulletFireTime = 3000; // 发射时间，单位ms
    static const uint8_t  FireballCount              = 8 ;    // 发射数量

    // 攻击方式3相关参数
    static const uint16_t CenterFireballAttackTime = 100; // 发射时间，单位ms

    // 攻击方式4相关参数
    static const uint16_t NotchedBulletsAttackTime = 500; // 发射时间，单位ms

    // 攻击方式5相关参数
    static const uint16_t BlinkRandomTime = 1000; // 闪现持续时间，单位ms（包含闪现+留火球+返回）

    // 攻击方式6相关参数
    static const uint16_t BlinkAlignedTime = 100; // 定向闪现时间，单位ms

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
    static const uint16_t XiangliuEnemyDeadTime = 500; // 死亡动画时间，单位ms

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

/**
 * @brief HundunEnemy class - 混沌 BOSS（四凶之首）
 * @note  中文：混沌 ｜ 英文：Hundun
 * @note  神话典故：四凶之一，《山海经》记载其形如黄囊，赤如丹火，六足四翼，无面目；
 * @note  《庄子》有"七窍凿而混沌死"的典故，象征混乱、无序、未分化的原始状态。
 * 
 * @note  BOSS级大型敌人，体型巨大（64x64 像素），最高血量（四凶之首），中等攻击力，
 * @note  低速移动但可瞬移，攻击方式以干扰和混乱为主。
 * 
 * @note  === 攻击方式 ===
 * @note  MODE_1: 混沌涌动 - 随机闪烁移动，同时向4个方向发射普通子弹
 *               持续时间 ChaosSurgeTime=3000ms，每 ChaosSurgeInterval=500ms 闪烁并发射一轮
 * @note  MODE_2: 七窍封印 - 在屏幕上生成7个闪烁干扰区域遮挡视野
 *               持续时间 SealAperturesTime=2000ms，呼应"七窍凿而混沌死"典故
 * @note  MODE_3: 虚空牵引 - 将玩家向混沌位置缓慢拉近，同时发射追踪火球弹
 *               持续时间 VoidPullTime=2500ms，每 VoidPullInterval=300ms 拉近一次
 * @note  MODE_4: 混沌漩涡 - 螺旋式发射子弹，Y位置按正弦波扫射
 *               持续时间 ChaoticBarrageTime=2000ms，每 ChaoticBarrageInterval=150ms 发射一发
 * @note  MODE_5: 时空裂隙 - 快速发射带随机缺口的弹幕墙，缺口位置每轮变化
 *               持续时间 TemporalRiftTime=2500ms，每 TemporalRiftInterval=400ms 发射一轮
 * @note  MODE_6: 归于混沌 - 全屏弹幕攻击，中间有安全缺口，血量越低缺口越小
 *               警告时间 ReturnToChaosWarning=500ms，发射时间 ReturnToChaosTime=100ms
 */

class HundunEnemy : public IRole {
public:
    uint16_t              think_count         = 0;
    static const uint16_t HundunEnemyDeadTime = 500; // 死亡动画时间，单位ms

    uint16_t action_timer   = 0; // 倒计时
    uint16_t action_MaxTime = 0; // 记录动作最大持续时间
    uint16_t action_count   = 0; // 记录动作持续时间

    bool positionChange = false;

    // 攻击方式1相关参数 - 混沌涌动
    static const uint16_t ChaosSurgeTime     = 3000; // 混沌涌动持续时间，单位ms
    static const uint16_t ChaosSurgeInterval = 500;  // 每次闪烁间隔，单位ms

    // 攻击方式2相关参数 - 七窍封印
    static const uint16_t SealAperturesTime = 2000; // 七窍封印持续时间，单位ms
    static const uint8_t  ApertureCount     = 7;    // 干扰区域数量（七窍）
    uint8_t               aperturePositions[7][2];  // 存储7个干扰区域的位置 [x, y]
    bool                  aperturesGenerated = false;

    // 攻击方式3相关参数 - 虚空牵引
    static const uint16_t VoidPullTime     = 2500; // 虚空牵引持续时间，单位ms
    static const uint16_t VoidPullInterval = 300;  // 每次拉近间隔，单位ms
    static const uint8_t  VoidPullDistance = 8;    // 每次拉近距离，单位像素

    // 攻击方式4相关参数 - 混沌漩涡
    static const uint16_t ChaoticBarrageTime     = 2000; // 混沌漩涡持续时间，单位ms
    static const uint16_t ChaoticBarrageInterval = 150;  // 每次发射间隔，单位ms
    uint8_t               spiralPhase            = 0;    // 螺旋相位，用于计算Y偏移

    // 攻击方式5相关参数 - 时空裂隙
    static const uint16_t TemporalRiftTime     = 2500; // 时空裂隙持续时间，单位ms
    static const uint16_t TemporalRiftInterval = 550;  // 每次发射间隔，单位ms（延长给玩家反应时间）
    static const uint8_t  RiftGapSize          = 18;   // 缺口大小，单位像素（增大缺口）
    uint8_t               riftWaveCount        = 0;    // 裂隙波次计数

    // 攻击方式6相关参数 - 归于混沌
    static const uint16_t ReturnToChaosWarning = 500; // 警告时间，单位ms
    static const uint16_t ReturnToChaosTime    = 100; // 发射时间，单位ms
    bool                  warningDisplayed     = false;

public:
    HundunEnemy(
        uint8_t startX = 164, uint8_t startY = 32, uint8_t initPosX = 96, uint8_t initPosY = 0, uint8_t level = 1, uint16_t dropExp = 0
    );
    ~HundunEnemy() = default;

    void drawRole() override;                                   // 绘制角色（包含分身和干扰效果）
    void init() override;                                       // 初始化
    void think() override;                                      // 思考决策
    void doAction() override;                                   // 执行动作
    void die() override;                                        // 死亡处理
    void shoot(uint8_t x, uint8_t y, BulletType type) override; // 发射子弹

    void chaosSurge();                // 攻击方式1，混沌涌动 - 随机闪烁移动并向4方向发射子弹
    void sealSevenApertures();        // 攻击方式2，七窍封印 - 生成7个干扰区域遮挡视野
    void voidPull();                  // 攻击方式3，虚空牵引 - 拉近玩家并发射追踪火球
    void chaoticBarrage();            // 攻击方式4，混沌漩涡 - 螺旋式发射子弹
    void temporalRift();              // 攻击方式5，时空裂隙 - 发射带随机缺口的弹幕墙
    void returnToChaos();             // 攻击方式6，归于混沌 - 全屏弹幕攻击
};


#endif // ENEMYROLE_HPP
