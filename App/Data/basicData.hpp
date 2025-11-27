#ifndef BASICDATA_HPP
#define BASICDATA_HPP

#include "font.h"

enum class RoleIdentity {
    UNKNOWN = 0,
    ENEMY   = 1,
    Player  = 2,
};

enum class BulletType {
    BASIC          = 0,
    FIRE_BALL      = 1,
    LIGHTNING_LINE = 2,
};

class HeatData {
public:
    uint8_t currentHeat{};
    uint8_t maxHeat{};
    uint8_t heatCoolDownRate{};
    uint8_t heatCoolDownTimer{};
    uint8_t heatPerShot{};

public:
    HeatData() = default;
    HeatData(uint8_t current, uint8_t max, uint8_t coolDownRate, uint8_t perShot)
    : currentHeat(current)
    , maxHeat(max)
    , heatCoolDownRate(coolDownRate)
    , heatPerShot(perShot) { }
};

// 血量数据结构体
// 每隔一段时间自动回复一定量的血量
// 每 healResetTime/healSpeed 毫秒回复 healValue 点血量
class HealthData {
public:
    uint16_t currentHealth{};
    uint16_t maxHealth{};

    uint8_t  healValue       = 3;
    uint16_t healTimeCounter = 0;
    uint16_t healResetTime   = 15000;
    uint8_t  healSpeed       = 5;
};

/**
 * @brief 空间移动数据结构体
 * @note  包含位置尺寸和移动速度
 * @param moveSpeed 移动速度
 * @param currentPosX 初始X位置，是物体左上角的X坐标
 * @param currentPosY 初始Y位置，是物体左上角的Y坐标
 * @param refPosX 参考X位置，用于碰撞检测后的回退
 * @param refPosY 参考Y位置，用于碰撞检测后的回退
 * @param sizeX 宽度
 * @param sizeY 高度
 */
class SpatialMovementData {
public:
    //能否穿越边界
    bool canCrossBorder = false;

    //连续触发碰撞次数（用来碰撞检测后的处理）
    uint8_t consecutiveCollisionCount = 0;

    //移动速度
    int8_t moveSpeed{};
    //位置尺寸
    int16_t currentPosX{};
    int16_t currentPosY{};

    int16_t refPosX{};
    int16_t refPosY{};

    uint8_t sizeX{};
    uint8_t sizeY{};

public:
    SpatialMovementData() = default;

    /**
     * @brief 构造函数
     * @param speed 移动速度，可以取负值表示反方向移动
     * @param currentPosX 现在X位置
     * @param currentPosY 现在Y位置
     * @param refPosX 期望X位置
     * @param refPosY 期望Y位置
     * @param sx 宽度
     * @param sy 高度
     */
    SpatialMovementData(
        bool canCrossBorder, int8_t speed, uint8_t currentPosX, uint8_t currentPosY, uint8_t refPosX, uint8_t refPosY,
        uint8_t sx, uint8_t sy
    )
    : canCrossBorder(canCrossBorder)
    , moveSpeed(speed)
    , currentPosX(currentPosX)
    , currentPosY(currentPosY)
    , refPosX(refPosX)
    , refPosY(refPosY)
    , sizeX(sx)
    , sizeY(sy) { }
};

class RoleAttackData {
public:
    //攻击速度
    uint8_t  shootSpeed;
    uint16_t shootCooldownTimer;
    uint16_t shootCooldownResetTime;
    uint8_t  shootCooldownSpeed;
    //攻击力
    uint8_t attackPower;
    uint8_t collisionPower;
    //子弹速度
    int8_t bulletSpeed;
    //子弹伤害范围
    uint8_t bulletRange;//只对火球弹生效
    //子弹伤害倍率
    float bulletDamageMultiplier;//只对闪电链弹生效
};

class InitData {
public:
    bool    isInited = false;
    uint8_t posX{};
    uint8_t posY{};
    uint8_t init_count{};
};

enum class ActionState { IDLE = 0, MOVING = 1, ATTACKING = 2 };

enum class AttackMode { NONE = 0, MODE_1 = 1, MODE_2 = 2, MODE_3 = 3 , MODE_4 = 4 , MODE_5 = 5 , MODE_6 = 6 };

enum class MoveMode { NONE = 0, UP = 1, DOWN = 2, LEFT = 3, RIGHT = 4 };

class ActionData {
public:
    ActionState currentState{};
    AttackMode  attackMode{};
    MoveMode    moveMode{};
};

class DeathData {
public:
    bool     isDead     = false;
    uint16_t deathTimer = 500; //死亡动画时间计时器，单位ms
    uint16_t dropExperiencePoints = 0;
};

class RoleData {
public:
    //初始化位置
    InitData initData;

    //基本信息
    RoleIdentity identity;
    bool         isActive = false;

    //死亡状态信息
    DeathData deathData;

    //动作信息
    ActionData actionData;

    //血量信息
    HealthData healthData;

    //等级信息
    uint8_t level;

    //热量信息
    HeatData heatData;

    //攻击信息
    RoleAttackData attackData;

    //空间移动信息
    SpatialMovementData spatialData;

    //图片信息
    const Image *img = nullptr; //易出错的地方，有问题优先检查
    //角色图片，指向同一片内存，利用浅拷贝的特性
public:
    RoleData() {
        identity = RoleIdentity::UNKNOWN;

        //初始化信息
        initData.posX       = 0;
        initData.posY       = 0;
        initData.init_count = 0;

        //血量信息初始化
        healthData.currentHealth   = 100;
        healthData.maxHealth       = 100;
        healthData.healValue       = 3;
        healthData.healTimeCounter = 0;
        healthData.healResetTime   = 15000;
        healthData.healSpeed       = 5;

        //等级初始化
        level = 1;

        //热量信息初始化
        heatData = HeatData(0, 100, 10, 10);

        //射击后，cooldown计时器增加，只有当计时器达到0时才能再次射击
        attackData.shootSpeed             = 5;
        attackData.shootCooldownTimer     = 0;
        attackData.shootCooldownSpeed     = 5;
        attackData.attackPower            = 10;
        attackData.collisionPower         = 5;
        attackData.shootCooldownResetTime = 1000; // Added initialization for shootCooldownResetTime

        //子弹速度
        attackData.bulletSpeed            = 1;
        //火球弹伤害范围
        attackData.bulletRange            = 1;
        //闪电链伤害倍率
        attackData.bulletDamageMultiplier = 1.5f;


        //空间移动信息初始化
        spatialData = SpatialMovementData(0, 1, 0, 0, 0, 0, 0, 0);

        //死亡状态信息初始化
        deathData.deathTimer = 500;
        deathData.isDead     = false;
        deathData.dropExperiencePoints = 0;

        //图片信息初始化
        img = nullptr;
    }
};

class BulletData {
public:
    //来自
    RoleIdentity fromIdentity{RoleIdentity::UNKNOWN};

    //死亡信息
    DeathData deathData;

    //空间移动信息
    SpatialMovementData spatialData;
    //伤害值
    uint8_t damage{};
    uint8_t range{};
    //子弹类型
    BulletType type{};
    //是否激活
    bool isActive{true};
    //图片信息
    const Image *img = nullptr;

public:
    BulletData() = default;

    BulletData(int8_t speed, uint8_t currentPosX, uint8_t currentPosY, uint8_t dmg, uint8_t rg, BulletType type , float dmgMultiplier = 1.0f) {
        //根据类型进行初始化
        this->type = type;
        switch (this->type) {
        case BulletType::BASIC:
            //img = &bulletImg;
            damage = dmg;
            spatialData =
                SpatialMovementData(false, speed, currentPosX, currentPosY, currentPosX, currentPosY, img->w, img->h);
            isActive = true;
            range    = rg;
            break;
        case BulletType::FIRE_BALL:
            //img = &fireBallImg;
            damage = dmg;
            spatialData =
                SpatialMovementData(false, speed, currentPosX, currentPosY, currentPosX, currentPosY, img->w, img->h);
            isActive = true;
            range    = rg;
            break;
        case BulletType::LIGHTNING_LINE:
            //img = &lightningLineImg;
            damage = dmg;
            spatialData =
                SpatialMovementData(true, speed, currentPosX, currentPosY, currentPosX, currentPosY, img->w, img->h);
            isActive = true;
            range    = rg;
            break;
        }
    }
};

enum class CollisionDirection { NONE = 0, LEFT = 1, RIGHT = 2, UP = 3, DOWN = 4 };

typedef struct {
    bool               isCollision;
    CollisionDirection direction;
} CollisionResult;

#endif // BASICDATA_HPP