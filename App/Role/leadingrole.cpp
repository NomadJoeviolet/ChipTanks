#include "leadingRole.hpp"
#include "../gameEntityManager.hpp"
#include "../gamePerkCardManager.hpp"
#include "../Peripheral/OLED/oled.h"

#include "FreeRTOS.h"
#include "task.h"

extern GameEntityManager   g_entityManager;
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
    m_pdata->img = &BITtankImg;

    // phoenixWingmanOwned = true;
    // kuiniuWingmanOwned  = true;
    // magicTimOwned       = true;
    // bulletTypeOwned.fireBallBulletOwed = true;
    // bulletTypeOwned.lightningLineBulletOwed = true;

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
    m_pdata->healthData.healResetTime   = 15000; //ms
    m_pdata->healthData.healSpeed       = 3;     //  15000/5 = 3000ms 恢复一次血量

    //空间移动信息
    m_pdata->spatialData.canCrossBorder            = false;
    m_pdata->spatialData.currentPosX               = -16; // Starting X position
    m_pdata->spatialData.currentPosY               = 16;  // Starting Y position
    m_pdata->spatialData.refPosX                   = -16;
    m_pdata->spatialData.refPosY                   = 16;
    m_pdata->spatialData.sizeX                     = m_pdata->img->w; // Width of the role
    m_pdata->spatialData.sizeY                     = m_pdata->img->h; // Height of the role
    m_pdata->spatialData.moveSpeed                 = 1;               // Movement speed
    m_pdata->spatialData.consecutiveCollisionCount = 0;

    //初始化位置
    m_pdata->initData.posX = 0;
    m_pdata->initData.posY = 16;

    //攻击信息
    m_pdata->attackData.attackPower = 15;

    //初始值为4，最大值16
    m_pdata->attackData.shootCooldownSpeed = 4; //每controlDelayTime减少5*controlDelayTime 点冷却时间

    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 4000; //ms
    m_pdata->attackData.bulletSpeed            = 1;

    m_pdata->attackData.bulletRange            = 10;   //只对火球弹生效
    m_pdata->attackData.bulletDamageMultiplier = 1.5f; //只对闪电链弹生效

    m_pdata->attackData.collisionPower = 30;

    //热量信息
    m_pdata->heatData.maxHeat          = 100;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 15; //初始15
    m_pdata->heatData.heatCoolDownRate = 4;  //每次冷却4点热量，每次冷却时间间隔由200ms

    //死亡状态信息
    m_pdata->deathData.deathTimer = 2000;
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
    if (m_pdata->initData.isInited == false) {
        return;
    }

    if (m_pdata->deathData.isDead) {
        return; // 死亡状态下不执行动作
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

    if (m_pdata->attackData.shootCooldownTimer > 0) return; // 冷却中，无法射击

    uint8_t m_x                = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t m_y                = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
    uint8_t whichBulletToShoot = rand() % bulletTypeOwned.BulletOwnedTypeCount;
    switch (whichBulletToShoot) {
    case 0:
        if (bulletTypeOwned.basicBulletOwed) shoot(m_x, m_y, BulletType::BASIC);
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
    if (m_pdata->deathData.deathTimer > 0) {
        m_pdata->deathData.deathTimer -= controlDelayTime;
        m_pdata->deathData.deathTimer = etl::max(m_pdata->deathData.deathTimer, uint16_t(0));
        return;
    }

    m_pdata->isActive = false;

    // 无敌模式，暂不实现死亡，测试用
    // m_pdata->deathData.isDead     = false;
    // m_pdata->isActive             = true;
    // m_pdata->healthData.currentHealth = m_pdata->healthData.maxHealth;
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
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot > m_pdata->heatData.maxHeat) {
                taskEXIT_CRITICAL();
                return; // 超过最大热量，无法射击
            }
            if (m_pdata->attackData.shootCooldownTimer > 0) {
                taskEXIT_CRITICAL();
                return; // 冷却中，无法射击
            }

            IBullet *newBullet = createBullet(x, y, BulletType::BASIC);
            if (newBullet != nullptr) {
                if (g_entityManager.addBullet(newBullet)) {
                    // Successfully added bullet
                    m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
                    m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot;
                } else {
                    delete newBullet; // 修复: 使用delete而不是delete[]
                }
            }
        }
        break;
    case BulletType::FIRE_BALL:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 2 > m_pdata->heatData.maxHeat) {
                taskEXIT_CRITICAL();
                return; // 超过最大热量，无法射击
            }
            if (m_pdata->attackData.shootCooldownTimer > 0) {
                taskEXIT_CRITICAL();
                return; // 冷却中，无法射击
            }

            IBullet *newBullet = createBullet(x, y, BulletType::FIRE_BALL);
            if (newBullet != nullptr) {
                if (g_entityManager.addBullet(newBullet)) {
                    // Successfully added bullet
                    m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
                    m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot * 2;
                } else {
                    delete newBullet; // 修复: 使用delete而不是delete[]
                }
            }
        }
        break;
    case BulletType::LIGHTNING_LINE:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 1.5 > m_pdata->heatData.maxHeat) {
                taskEXIT_CRITICAL();
                return; // 超过最大热量，无法射击
            }
            if (m_pdata->attackData.shootCooldownTimer > 0) {
                taskEXIT_CRITICAL();
                return; // 冷却中，无法射击
            }

            IBullet *newBullet = createBullet(x, y, BulletType::LIGHTNING_LINE);
            if (newBullet != nullptr) {
                if (g_entityManager.addBullet(newBullet)) {
                    // Successfully added bullet
                    m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
                    m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot * 2;
                } else {
                    delete newBullet; // 修复: 使用delete而不是delete[]
                }
            }
        }
        break;
    }

    taskEXIT_CRITICAL();
}

void LeadingRole::drawRole() {
    if (m_pdata->img != nullptr && m_pdata->isActive && !m_pdata->deathData.isDead) {
        OLED_DrawImage(
            m_pdata->spatialData.currentPosX, m_pdata->spatialData.currentPosY, m_pdata->img, OLED_COLOR_NORMAL
        );

        // 绘制子弹类型特效（仅在玩家初始化完成后绘制）
        if (m_pdata->initData.isInited) {
            int16_t px = m_pdata->spatialData.currentPosX;
            int16_t py = m_pdata->spatialData.currentPosY;
            int16_t sizeX = m_pdata->spatialData.sizeX;
            int16_t sizeY = m_pdata->spatialData.sizeY;

            // 火焰特效：玩家尾部（X负方向）- 如果拥有火球子弹
            if (bulletTypeOwned.fireBallBulletOwed && px > 10 && py+sizeY/2 > 8 && py + sizeY < 56) {
                static uint8_t firePhase = 0;
                firePhase = (firePhase + 1) % 6;
                
                int16_t tailX = px;  // 玩家尾部X位置
                int16_t tailY = py + sizeY / 2;  // Y方向中心
                
                // 主火焰核心 - 类似图片中心的火焰形状
                OLED_DrawFilledRectangle(tailX - 2, tailY - 1, 2, 3, OLED_COLOR_NORMAL);  // 火焰核心
                
                // 主火焰舌头 - 向后弯曲延伸（类似图片主火焰）
                OLED_DrawLine(tailX - 2, tailY, tailX - 5, tailY - 2, OLED_COLOR_NORMAL);   // 主舌向后上弯
                OLED_DrawLine(tailX - 5, tailY - 2, tailX - 7, tailY - 1, OLED_COLOR_NORMAL); // 主舌回弯
                OLED_DrawLine(tailX - 2, tailY, tailX - 5, tailY + 2, OLED_COLOR_NORMAL);   // 主舌向后下弯
                OLED_DrawLine(tailX - 5, tailY + 2, tailX - 7, tailY + 1, OLED_COLOR_NORMAL); // 主舌回弯
                
                // 上方副火焰 - 凤凰羽毛弧形（向后上方优雅延伸，X方向缩短）
                if (firePhase < 3) {
                    OLED_DrawLine(tailX - 1, tailY - 2, tailX - 3, tailY - 4, OLED_COLOR_NORMAL);  // 羽毛主轴
                    OLED_DrawLine(tailX - 3, tailY - 4, tailX - 5, tailY - 5, OLED_COLOR_NORMAL);  // 羽毛延伸
                    OLED_DrawLine(tailX - 5, tailY - 5, tailX - 6, tailY - 4, OLED_COLOR_NORMAL);  // 羽毛尖回勾
                    // 羽毛分叉
                    OLED_DrawLine(tailX - 4, tailY - 4, tailX - 5, tailY - 3, OLED_COLOR_NORMAL);
                } else {
                    OLED_DrawLine(tailX - 1, tailY - 2, tailX - 3, tailY - 3, OLED_COLOR_NORMAL);  // 羽毛主轴
                    OLED_DrawLine(tailX - 3, tailY - 3, tailX - 5, tailY - 4, OLED_COLOR_NORMAL);  // 羽毛延伸
                    OLED_DrawLine(tailX - 5, tailY - 4, tailX - 6, tailY - 3, OLED_COLOR_NORMAL);  // 羽毛尖回勾
                    // 羽毛分叉飘动
                    OLED_DrawLine(tailX - 4, tailY - 3, tailX - 5, tailY - 2, OLED_COLOR_NORMAL);
                }
                
                // 下方副火焰 - 凤凰羽毛弧形（向后下方对称，X方向缩短）
                if (firePhase < 3) {
                    OLED_DrawLine(tailX - 1, tailY + 2, tailX - 3, tailY + 5, OLED_COLOR_NORMAL);  // 羽毛主轴
                    OLED_DrawLine(tailX - 3, tailY + 5, tailX - 5, tailY + 6, OLED_COLOR_NORMAL);  // 羽毛延伸
                    OLED_DrawLine(tailX - 5, tailY + 6, tailX - 6, tailY + 5, OLED_COLOR_NORMAL);  // 羽毛尖回勾
                    // 羽毛分叉
                    OLED_DrawLine(tailX - 4, tailY + 5, tailX - 5, tailY + 3, OLED_COLOR_NORMAL);
                } else {
                    OLED_DrawLine(tailX - 1, tailY + 2, tailX - 3, tailY + 4, OLED_COLOR_NORMAL);  // 羽毛主轴
                    OLED_DrawLine(tailX - 3, tailY + 4, tailX - 5, tailY + 5, OLED_COLOR_NORMAL);  // 羽毛延伸
                    OLED_DrawLine(tailX - 5, tailY + 5, tailX - 6, tailY + 4, OLED_COLOR_NORMAL);  // 羽毛尖回勾
                    // 羽毛分叉飘动
                    OLED_DrawLine(tailX - 4, tailY + 4, tailX - 5, tailY + 3, OLED_COLOR_NORMAL);
                }
                
                // 火焰尖端闪烁效果
                if (firePhase < 2) {
                    OLED_DrawFilledRectangle(tailX - 8, tailY - 4, 1, 1, OLED_COLOR_NORMAL);
                    OLED_DrawFilledRectangle(tailX - 8, tailY + 4, 1, 1, OLED_COLOR_NORMAL);
                } else if (firePhase < 4) {
                    OLED_DrawFilledRectangle(tailX - 6, tailY - 1, 1, 1, OLED_COLOR_NORMAL);
                    OLED_DrawFilledRectangle(tailX - 6, tailY + 1, 1, 1, OLED_COLOR_NORMAL);
                }
            }

            // 闪电特效：玩家头部（X正方向）- 如果拥有闪电子弹
            if (bulletTypeOwned.lightningLineBulletOwed && px + sizeX < 122) {
                static uint8_t lightningPhase = 0;
                lightningPhase = (lightningPhase + 1) % 6;
                
                int16_t headX = px + sizeX;  // 玩家头部X位置
                int16_t headY = py + sizeY / 2;  // Y方向中心
                
                // 动态闪电效果 - X方向扩展3像素，更酷炫
                if (lightningPhase < 2) {
                    // 帧1：向上的锯齿电弧
                    OLED_DrawLine(headX + 1, headY, headX + 3, headY - 2, OLED_COLOR_NORMAL);
                    OLED_DrawLine(headX + 3, headY - 2, headX + 4, headY - 1, OLED_COLOR_NORMAL);
                    OLED_DrawLine(headX + 4, headY - 1, headX + 6, headY - 3, OLED_COLOR_NORMAL);
                    OLED_DrawFilledRectangle(headX + 1, headY + 1, 1, 1, OLED_COLOR_NORMAL);
                    OLED_DrawFilledRectangle(headX + 5, headY, 1, 1, OLED_COLOR_NORMAL);
                } else if (lightningPhase < 4) {
                    // 帧2：水平分叉电弧
                    OLED_DrawLine(headX + 1, headY, headX + 4, headY, OLED_COLOR_NORMAL);
                    OLED_DrawLine(headX + 4, headY, headX + 6, headY - 2, OLED_COLOR_NORMAL);
                    OLED_DrawLine(headX + 4, headY, headX + 6, headY + 2, OLED_COLOR_NORMAL);
                    OLED_DrawFilledRectangle(headX + 3, headY - 1, 1, 1, OLED_COLOR_NORMAL);
                    OLED_DrawFilledRectangle(headX + 3, headY + 1, 1, 1, OLED_COLOR_NORMAL);
                } else {
                    // 帧3：向下的锯齿电弧
                    OLED_DrawLine(headX + 1, headY, headX + 3, headY + 2, OLED_COLOR_NORMAL);
                    OLED_DrawLine(headX + 3, headY + 2, headX + 4, headY + 1, OLED_COLOR_NORMAL);
                    OLED_DrawLine(headX + 4, headY + 1, headX + 6, headY + 3, OLED_COLOR_NORMAL);
                    OLED_DrawFilledRectangle(headX + 1, headY - 1, 1, 1, OLED_COLOR_NORMAL);
                    OLED_DrawFilledRectangle(headX + 5, headY, 1, 1, OLED_COLOR_NORMAL);
                }
            }

            // 绘制僚机
            if (phoenixWingmanOwned) {
                drawPhoenixWingman();
            }
            if (kuiniuWingmanOwned) {
                drawKuiniuWingman();
            }
        }
    }
    if (m_pdata->deathData.isDead) {
        // 绘制死亡提示
        OLED_PrintString(40, 28, "YOU DIED", &font8x6, OLED_COLOR_NORMAL);
    }
}

// ===== 僚机系统实现 =====

void LeadingRole::updateWingmans() {
    if (m_pdata->deathData.isDead || !m_pdata->isActive || !m_pdata->initData.isInited) {
        return;
    }

    // 凤凰僚机计时器更新
    if (phoenixWingmanOwned) {
        phoenixShootTimer += controlDelayTime;
        if (phoenixShootTimer >= WINGMAN_SHOOT_INTERVAL) {
            phoenixShootTimer = 0;
            phoenixShoot();
        }
    }

    // 夔牛僚机计时器更新
    if (kuiniuWingmanOwned) {
        kuiniuShootTimer += controlDelayTime;
        if (kuiniuShootTimer >= WINGMAN_SHOOT_INTERVAL) {
            kuiniuShootTimer = 0;
            kuiniuShoot();
        }
    }

    // 魔法时间计时器更新
    if (magicTimOwned) {
        magicTimTimer += controlDelayTime;
        if (magicTimTimer >= WINGMAN_SHOOT_INTERVAL) {
            magicTimTimer = 0;
            magicTimeShoot();
        }
    }
}

void LeadingRole::drawPhoenixWingman() {
    // 凤凰僚机绘制在玩家后方（X负方向），羽毛向后延伸
    // 基准点：玩家左侧边缘
    int16_t px = m_pdata->spatialData.currentPosX+m_pdata->spatialData.sizeX+3;
    int16_t py = m_pdata->spatialData.currentPosY+3 ;

    // 边界检查：确保所有绘制坐标为正数（最左侧绘制点为 px-10）
    if (px < 11 || py < 8 || py > 64) {
        return;
    }

    // 凤凰身体核心（小圆点）
    OLED_DrawFilledRectangle(px - 2, py - 1, 2, 3, OLED_COLOR_NORMAL);

    // 上方羽毛 - 向后上方优雅弯曲延伸（参考凤凰羽毛图）
    // 主羽轴
    OLED_DrawLine(px - 2, py - 1, px - 6, py - 4, OLED_COLOR_NORMAL);  // 羽轴向后上
    OLED_DrawLine(px - 6, py - 4, px - 8, py - 6, OLED_COLOR_NORMAL);  // 羽轴延伸
    // 羽毛分叉（火焰状）
    OLED_DrawLine(px - 5, py - 3, px - 7, py - 2, OLED_COLOR_NORMAL);  // 分叉1
    OLED_DrawLine(px - 7, py - 5, px - 9, py - 4, OLED_COLOR_NORMAL);  // 分叉2

    // // 下方羽毛 - 向后下方优雅弯曲延伸
    // // 主羽轴
    // OLED_DrawLine(px - 2, py + 1, px - 6, py + 4, OLED_COLOR_NORMAL);  // 羽轴向后下
    // OLED_DrawLine(px - 6, py + 4, px - 8, py + 6, OLED_COLOR_NORMAL);  // 羽轴延伸
    // // 羽毛分叉
    // OLED_DrawLine(px - 5, py + 3, px - 7, py + 2, OLED_COLOR_NORMAL);  // 分叉1
    // OLED_DrawLine(px - 7, py + 5, px - 9, py + 4, OLED_COLOR_NORMAL);  // 分叉2

    // 闪烁的火焰尖端效果
    static uint8_t flamePhase = 0;
    flamePhase = (flamePhase + 1) % 4;
    if (flamePhase < 2) {
        OLED_DrawFilledRectangle(px - 9, py - 7, 1, 1, OLED_COLOR_NORMAL);  // 上羽毛火焰尖
        OLED_DrawFilledRectangle(px - 9, py + 7, 1, 1, OLED_COLOR_NORMAL);  // 下羽毛火焰尖
    } else {
        OLED_DrawFilledRectangle(px - 10, py - 5, 1, 1, OLED_COLOR_NORMAL); // 上火焰飘动
        OLED_DrawFilledRectangle(px - 10, py + 5, 1, 1, OLED_COLOR_NORMAL); // 下火焰飘动
    }
}

void LeadingRole::drawKuiniuWingman() {
    // 夔牛僚机绘制在玩家后方（X负方向），牛角向后延伸
    // 基准点：玩家左下侧
    int16_t px = m_pdata->spatialData.currentPosX+m_pdata->spatialData.sizeX+3;
    int16_t py = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY-3;

    // 边界检查：确保所有绘制坐标为正数（最左侧绘制点为 px-11）
    if (px < 12 || py < 6 || py > 58) {
        return;
    }

    // 夔牛身体核心
    OLED_DrawFilledRectangle(px - 2, py - 1, 2, 3, OLED_COLOR_NORMAL);

    // 下方牛角 - 向后下方弯曲延伸（类似凤凰羽毛的优雅弧线）
    // 主角轴
    OLED_DrawLine(px - 2, py + 2, px - 5, py + 4, OLED_COLOR_NORMAL);  // 角根部
    OLED_DrawLine(px - 5, py + 4, px - 8, py + 5, OLED_COLOR_NORMAL);  // 角中部弯曲
    OLED_DrawLine(px - 8, py + 5, px - 10, py + 4, OLED_COLOR_NORMAL); // 角尖回勾
    // 角的厚度
    OLED_DrawLine(px - 3, py + 3, px - 6, py + 5, OLED_COLOR_NORMAL);  // 平行线增加厚度

    // 闪烁的雷电效果
    static uint8_t thunderPhase = 0;
    thunderPhase = (thunderPhase + 1) % 4;
    if (thunderPhase < 2) {
        OLED_DrawFilledRectangle(px - 11, py - 4, 1, 1, OLED_COLOR_NORMAL); // 上角雷电
        OLED_DrawFilledRectangle(px - 11, py + 4, 1, 1, OLED_COLOR_NORMAL); // 下角雷电
    } else {
        OLED_DrawFilledRectangle(px - 10, py - 3, 1, 1, OLED_COLOR_NORMAL); // 上角闪烁
        OLED_DrawFilledRectangle(px - 10, py + 3, 1, 1, OLED_COLOR_NORMAL); // 下角闪烁
    }
}

void LeadingRole::phoenixShoot() {
    // 凤凰从玩家左侧发射火球
    taskENTER_CRITICAL();

    int16_t px = m_pdata->spatialData.currentPosX;
    int16_t py = m_pdata->spatialData.currentPosY;

    // 边界检查：如果玩家太靠近左边界，不发射
    if (px < 5) {
        taskEXIT_CRITICAL();
        return;
    }

    // 从凤凰位置发射（玩家左侧靠内1像素，Y方向组左侧位置）
    uint16_t shootX = px ;
    uint16_t shootY = py ;

    IBullet *newBullet = createBullet(shootX, shootY, BulletType::FIRE_BALL);
    if (newBullet != nullptr) {
        if (!g_entityManager.addBullet(newBullet)) {
            delete newBullet;  // 修复: 使用delete而不是delete[]
        }
    }

    taskEXIT_CRITICAL();
}

void LeadingRole::kuiniuShoot() {
    // 夔牛从玩家右侧发射雷电
    taskENTER_CRITICAL();

    int16_t px    = m_pdata->spatialData.currentPosX+m_pdata->spatialData.sizeX;
    int16_t py    = m_pdata->spatialData.currentPosY+m_pdata->spatialData.sizeY;

    // 边界检查：如果玩家太靠近右边界，不发射
    if (px + 5 > 127) {
        taskEXIT_CRITICAL();
        return;
    }

    // 从夔牛位置发射（玩家右侧+1像素，Y方向右侧位置）
    uint16_t shootX = px ;
    uint16_t shootY = py ;

    IBullet *newBullet = createBullet(shootX, shootY, BulletType::LIGHTNING_LINE);
    if (newBullet != nullptr) {
        if (!g_entityManager.addBullet(newBullet)) {
            delete newBullet;  // 修复: 使用delete而不是delete[]
        }
    }

    taskEXIT_CRITICAL();
}

void LeadingRole::magicTimeShoot() {
    // 魔法时间：在X=5的位置，随机Y位置发射3排普通子弹
    taskENTER_CRITICAL();

    uint8_t shootX = 5;
    uint8_t baseY  = rand() % 48 + 8; // 随机Y位置（8-56之间，避免边缘）

    // 发射3排子弹（上中下）
    for (int8_t i = -1; i <= 1; i++) {
        uint8_t shootY = baseY + i * 8;
        if (shootY < 5 || shootY > 58) continue; // 边界检查

        IBullet *newBullet = createBullet(shootX, shootY, BulletType::BASIC);
        if (newBullet != nullptr) {
            if (!g_entityManager.addBullet(newBullet)) {
                delete newBullet;  // 修复: 使用delete而不是delete[]
            }
        }
    }

    taskEXIT_CRITICAL();
}

void LeadingRole::levelUp() {
    if (m_pdata->deathData.isDead) return;
    if (m_pdata->level >= 15) {
        return; // 已经达到最高等级
    }

    if (experiencePoints >= experienceToLevelUp[m_pdata->level]) {
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
