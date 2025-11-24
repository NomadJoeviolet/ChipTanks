#include "leadingRole.hpp"
#include "../gameEntityManager.hpp"
#include "../gamePerkCardManager.hpp"
#include "../Peripheral/OLED/oled.h"

#include "FreeRTOS.h"
#include "task.h"

extern GameEntityManager g_entityManager;
extern GamePerkCardManager g_perkCardManager;

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

//闪电链弹一束条的范围穿透伤害，mul*attackPower+30 点伤害


/*********************************************************************/
/**
 * @brief LeadingRole class
 * @note  中文：主角 ｜ 英文：LeadingRole,玩家操控的主要角色，具备平衡的属性和多样的攻击方式。
 * @note  中等体型（16x16 像素），中等血量，适中的移动速度，能够发射多种类型的子弹，适合各种战斗场景。
 */

 //升级系统
 //每升1级，增加20点最大血量，增加1点攻击力，增加5点热量上限，增加1点生命值回复量
 //每升2级，增加1点热量冷却量，降低1点热量消耗
 //每升5级，增加1点攻击速度


LeadingRole::LeadingRole()
: IRole() { //会优先执行 基类构造函数
    //图片信息
    m_pdata->img = &tankImg;

    //身份信息
    m_pdata->identity          = RoleIdentity::Player;
    m_pdata->isActive          = true;
    m_pdata->initData.isInited = false;

    //等级信息
    m_pdata->level = 1;

    //血量信息
    m_pdata->healthData.currentHealth = 180;
    m_pdata->healthData.maxHealth     = 180;

    //回血信息
    m_pdata->healthData.healValue       = 5;
    m_pdata->healthData.healTimeCounter = 0;
    m_pdata->healthData.healResetTime   = 15000;//ms
    m_pdata->healthData.healSpeed       = 3;//  15000/5 = 3000ms 恢复一次血量

    //空间移动信息
    m_pdata->spatialData.canCrossBorder = false;
    m_pdata->spatialData.currentPosX    = -16; // Starting X position
    m_pdata->spatialData.currentPosY    = 16;  // Starting Y position
    m_pdata->spatialData.refPosX        = -16;
    m_pdata->spatialData.refPosY        = 16;
    m_pdata->spatialData.sizeX          = m_pdata->img->w; // Width of the role
    m_pdata->spatialData.sizeY          = m_pdata->img->h; // Height of the role
    m_pdata->spatialData.moveSpeed      = 1 ;               // Movement speed
    m_pdata->spatialData.consecutiveCollisionCount = 0;

    //初始化位置
    m_pdata->initData.posX = 0;
    m_pdata->initData.posY = 16;

    //攻击信息
    m_pdata->attackData.attackPower            = 15 ;

    //初始值为4，最大值16
    m_pdata->attackData.shootCooldownSpeed     = 4 ; //每controlDelayTime减少5*controlDelayTime 点冷却时间

    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 4000; //ms
    m_pdata->attackData.bulletSpeed            = 1;

    m_pdata->attackData.bulletRange = 10; //只对火球弹生效
    m_pdata->attackData.bulletDamageMultiplier = 1.5f; //只对闪电链弹生效

    m_pdata->attackData.collisionPower = 30;

    //热量信息
    m_pdata->heatData.maxHeat          = 100;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      =  0 ;//初始15
    m_pdata->heatData.heatCoolDownRate = 4 ; //每次冷却4点热量，每次冷却时间间隔由200ms

    //死亡状态信息
    m_pdata->deathData.deathTimer = 500;
    m_pdata->deathData.isDead     = false;
}

void LeadingRole::init() {
    static uint8_t init_count = 0;
    if (m_pdata->spatialData.currentPosX < m_pdata->initData.posX) {
        init_count += controlDelayTime;
        if (init_count >= 100) { // 每100ms移动一次
            m_pdata->spatialData.currentPosX++;
            init_count = 0;
        }
    } else {
        m_pdata->initData.isInited   = true;
        m_pdata->spatialData.refPosX = m_pdata->spatialData.currentPosX;
        m_pdata->spatialData.refPosY = m_pdata->spatialData.currentPosY;
    }
}

void LeadingRole::doAction() {
    if(m_pdata->initData.isInited == false) {
        return;
    }

    // Implement action logic for the leading role

    switch (m_pdata->actionData.currentState) {
    case ActionState::IDLE:
        // Do nothing
        break;
    case ActionState::MOVING:
        switch (m_pdata->actionData.moveMode) {
        case MoveMode::LEFT:
            move(-1, 0);
            break;
        case MoveMode::RIGHT:
            move(1, 0);
            break;
        case MoveMode::UP:
            move(0, -1);
            break;
        case MoveMode::DOWN:
            move(0, 1);
            break;
        default:
            break;
        }
        m_pdata->actionData.currentState = ActionState::IDLE;
        m_pdata->actionData.moveMode     = MoveMode::NONE;
        break;
    }

    if(m_pdata->attackData.shootCooldownTimer > 0)
        return; // 冷却中，无法射击

    uint8_t m_x                = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t m_y                = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
    uint8_t whichBulletToShoot = rand() % bulletTypeOwned.BulletOwnedTypeCount;
    switch (whichBulletToShoot) {
    case 0:
        if (bulletTypeOwned.basicBulletOwed)
            shoot(m_x, m_y, BulletType::BASIC);
        break;
    case 1:
        if (bulletTypeOwned.fireBallBulletOwed)
            shoot(m_x, m_y, BulletType::FIRE_BALL);
        else
            shoot(m_x, m_y, BulletType::BASIC);
        break;
    case 2:
        if (bulletTypeOwned.lightningLineBulletOwed)
            shoot(m_x, m_y, BulletType::LIGHTNING_LINE);
        else
            shoot(m_x, m_y, BulletType::BASIC);
        break;
    default:
        shoot(m_x, m_y, BulletType::BASIC); 
        break;
    }
}

void LeadingRole::die() {
    // m_pdata->deathData.isDead         = true;
    // m_pdata->isActive                = false;
    // m_pdata->deathData.deathTimer     = 500;

    m_pdata->deathData.isDead     = false;
    m_pdata->isActive             = true;
    m_pdata->healthData.currentHealth = m_pdata->healthData.maxHealth;
}

void LeadingRole::think() {
    // Implement player-specific logic if needed
}

void LeadingRole::shoot(uint8_t x, uint8_t y, BulletType type) {
    taskENTER_CRITICAL();

    // Create bullet based on type
    switch (type) {
    case BulletType::BASIC:
        {
            if(m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot > m_pdata->heatData.maxHeat)
                return; // 超过最大热量，无法射击
             if (m_pdata->attackData.shootCooldownTimer > 0)
                return; // 冷却中，无法射击
                
            IBullet *newBullet = createBullet(x, y, BulletType::BASIC);
            if (newBullet != nullptr) {
                if (g_entityManager.addBullet(newBullet)) {
                    // Successfully added bullet
                    m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
                    m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot;
                } else {
                    delete[] newBullet; // Clean up if not added
                }
            }
        }
        break;
    case BulletType::FIRE_BALL:
        {
            if(m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot*2 > m_pdata->heatData.maxHeat)
                return; // 超过最大热量，无法射击
             if (m_pdata->attackData.shootCooldownTimer > 0)
                return; // 冷却中，无法射击
                
            IBullet *newBullet = createBullet(x, y, BulletType::FIRE_BALL);
            if (newBullet != nullptr) {
                if (g_entityManager.addBullet(newBullet)) {
                    // Successfully added bullet
                    m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
                    m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot;
                } else {
                    delete[] newBullet; // Clean up if not added
                }
            }
        }
        break;
    case BulletType::LIGHTNING_LINE:
        {
            if(m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot*1.5 > m_pdata->heatData.maxHeat)
                return; // 超过最大热量，无法射击
             if (m_pdata->attackData.shootCooldownTimer > 0)
                return; // 冷却中，无法射击

            IBullet *newBullet = createBullet(x, y, BulletType::LIGHTNING_LINE);
            if (newBullet != nullptr) {
                if (g_entityManager.addBullet(newBullet)) {
                    // Successfully added bullet
                    m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
                    m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot;
                } else {
                    delete[] newBullet; // Clean up if not added
                }
            }
        }
    }

    taskEXIT_CRITICAL();
}

void LeadingRole::drawRole() {
    if (m_pdata->img != nullptr && m_pdata->isActive && !m_pdata->deathData.isDead) {
        OLED_DrawImage(
            m_pdata->spatialData.currentPosX, m_pdata->spatialData.currentPosY, m_pdata->img, OLED_COLOR_NORMAL
        );
    }
}

void LeadingRole::levelUp() {
    if( m_pdata->level >= 10 ) {
        return; // 已经达到最高等级
    }

    if(experiencePoints >= experienceToLevelUp[m_pdata->level]) {
       experiencePoints -= experienceToLevelUp[m_pdata->level];
        m_pdata->level++;

        // Increase max health by 20
        m_pdata->healthData.maxHealth += 20;
        m_pdata->healthData.currentHealth = m_pdata->healthData.maxHealth; // Heal to full on level up

        // Increase attack power by 1
        m_pdata->attackData.attackPower += 1;

        // Increase heat max by 5
        m_pdata->heatData.maxHeat += 5;

        // Increase heal value by 1
        m_pdata->healthData.healValue += 1;

        // Every 2 levels, increase heat cool down rate by 1 and decrease heat per shot by 1
        if (m_pdata->level % 2 == 0) {
            m_pdata->heatData.heatCoolDownRate += 1;
            if (m_pdata->heatData.heatPerShot > 1) {
                m_pdata->heatData.heatPerShot -= 1;
            }

            //触发选卡机制
            g_perkCardManager.triggerPerkSelection();
        }

        // Every 5 levels, increase shoot cooldown speed by 1
        if (m_pdata->level % 5 == 0) {
            m_pdata->attackData.shootCooldownSpeed += 2;
        }

    }
}

/*********************************************************************/

