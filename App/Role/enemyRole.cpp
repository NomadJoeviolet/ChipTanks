#include "enemyRole.hpp"
#include "bullet.hpp"

#include "etl/algorithm.h"
#include "../Peripheral/OLED/oled.h"
#include "../gameEntityManager.hpp"

#include "FreeRTOS.h"
#include "task.h"

extern GameEntityManager g_entityManager;

/************************************************************
// controlDelayTime 由 threads.cpp 控制线程定义
// controlDelayTime = 10
// 设计冷却和热量机制查看role.cpp
// 射击冷却时间=resetTime/ (Speed) ms
// 热量冷却速率= heatCoolDownRate 每次冷却时间间隔由2000ms
// 普通子弹热量消耗倍率 1
// 火球弹热量消耗倍率 2
// 闪电链弹热量消耗倍率 1.5
// role.cpp中的createBullet决定发射子弹的数值和机制
// 普通子弹击中敌人后造成伤害， attackPower 点伤害
// 火球弹击中敌人后对击中的敌人造成一次伤害，并在一定范围内造成范围伤害（击中的敌人也会受到范围伤害）
// 两次伤害均为 attackPower + 10 点伤害
// 闪电链弹一束条的范围穿透伤害，mul*attackPower + 30 点伤害
************************************************************/
/*******************************************************************/
/**
 * @brief FeilianEnemy class
 * @note 飞廉（Feilian）敌人：小型快速移动单位，使用 12x12 图像。
 * @note 移动速度较快，适合短距离穿插与规避。仅使用普通子弹进行攻击。
 * @note 关卡中作为低中等级常见小怪出现，血量与攻击力随等级线性增长。
 */

// 飞廉敌人数值设定
// 血量: 10 + level * 20
// 攻击力: 2 + level * 1
// 移动速度: 3 （最快）
// 射击冷却时间: 4000/5 = 800ms
// 热量信息: 每次射击消耗 20，最大储存 100，冷却速率 10/2000ms
// 碰撞伤害: 4 + level * 1

FeilianEnemy::FeilianEnemy(
    uint8_t startX, uint8_t startY, uint8_t initPosX, uint8_t initPosY, uint8_t level, uint16_t dropExp
)
: IRole() {
    // 图片信息
    m_pdata->img = &feilianImg;

    // 身份信息
    m_pdata->identity          = RoleIdentity::ENEMY;
    m_pdata->isActive          = true;
    m_pdata->initData.isInited = false;

    // 等级信息
    m_pdata->level = level;

    // 血量信息
    m_pdata->healthData.currentHealth = 10 + level * 20;
    m_pdata->healthData.maxHealth     = 10 + level * 20;
    // 回血信息
    // 攻击相关信息
    m_pdata->healthData.healValue       = 0;
    m_pdata->healthData.healTimeCounter = 0;
    m_pdata->healthData.healResetTime   = 15000;
    m_pdata->healthData.healSpeed       = 0;

    m_pdata->spatialData.canCrossBorder            = false;
    m_pdata->spatialData.currentPosX               = startX; // Starting X position
    m_pdata->spatialData.currentPosY               = startY; // Starting Y position
    m_pdata->spatialData.refPosX                   = startX;
    m_pdata->spatialData.refPosY                   = startY;
    m_pdata->spatialData.sizeX                     = m_pdata->img->w;
    m_pdata->spatialData.sizeY                     = m_pdata->img->h;
    m_pdata->spatialData.moveSpeed                 = 3; // Set movement speed
    m_pdata->spatialData.consecutiveCollisionCount = 0;

    m_pdata->initData.posX = initPosX;
    m_pdata->initData.posY = initPosY;

    m_pdata->attackData.attackPower            = 2 + level * 1;
    m_pdata->attackData.shootCooldownSpeed     = 5;
    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 4000;
    m_pdata->attackData.bulletSpeed            = 1;

    m_pdata->attackData.bulletRange            = 0;
    m_pdata->attackData.bulletDamageMultiplier = 1.5f;

    m_pdata->attackData.collisionPower = 4 + level * 1;

    m_pdata->heatData.maxHeat          = 100;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 20;
    m_pdata->heatData.heatCoolDownRate = 10;

    m_pdata->deathData.deathTimer           = feilianEnemyDeadTime;
    m_pdata->deathData.isDead               = false;
    m_pdata->deathData.dropExperiencePoints = dropExp;

    // Initialize other enemy-specific data here
}

// 初始化实现
void FeilianEnemy::init() {
    m_pdata->initData.init_count += controlDelayTime;
    // Initialize enemy role specifics

    if (m_pdata->spatialData.currentPosX > m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 30) { // 每30ms移动一次
            m_pdata->spatialData.currentPosX -= 1;
            m_pdata->initData.init_count = 0;
        }
    } else if (m_pdata->spatialData.currentPosX < m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 30) { // 每30ms移动一次
            m_pdata->spatialData.currentPosX += 1;
            m_pdata->initData.init_count = 0;
        }
    } else {
        m_pdata->initData.isInited   = true;
        m_pdata->spatialData.refPosX = m_pdata->spatialData.currentPosX;
        m_pdata->spatialData.refPosY = m_pdata->spatialData.currentPosY;
        m_pdata->initData.init_count = 0;
    }
}

void FeilianEnemy::think() {
    // Implement enemy AI logic
    think_count += controlDelayTime;
    if (think_count < 100) // 每100ms执行一次思考
        return;

    think_count = 0;

    uint8_t randomAction = rand() % 6;
    // Random action: 0 - move left, 1 - move right, 2 - move down, 3 - move up, 4 - stay still, 5 - shoot
    if (m_pdata->actionData.currentState == ActionState::IDLE) {
        // 移动
        if (randomAction == 0) {
            m_pdata->actionData.moveMode     = MoveMode::LEFT;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 1) {
            m_pdata->actionData.moveMode     = MoveMode::RIGHT;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 2) {
            m_pdata->actionData.moveMode     = MoveMode::DOWN;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 3) {
            m_pdata->actionData.moveMode     = MoveMode::UP;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 4) {
            // Stay still
            m_pdata->actionData.moveMode     = MoveMode::NONE;
            m_pdata->actionData.currentState = ActionState::MOVING;
        }

        // 攻击
        else if (randomAction == 5) {
            m_pdata->actionData.attackMode   = AttackMode::MODE_1;
            m_pdata->actionData.currentState = ActionState::ATTACKING;
        }
    }
}

void FeilianEnemy::doAction() {
    if (m_pdata->initData.isInited == false) {
        return;
    }

    // Implement enemy action logic
    if (m_pdata->deathData.isDead) {
        return;
    }
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
        // Move logic handled in think()
        break;
    case ActionState::ATTACKING:
        uint8_t m_x = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t m_y = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
        switch (m_pdata->actionData.attackMode) {
        case AttackMode::MODE_1:
            shoot(m_x, m_y, BulletType::BASIC);
            break;
        default:
            break;
        }
        m_pdata->actionData.currentState = ActionState::IDLE;
        m_pdata->actionData.attackMode   = AttackMode::NONE;
        break;
    }
}

void FeilianEnemy::drawRole() {
    if (m_pdata->img != nullptr && m_pdata->isActive && !m_pdata->deathData.isDead) {
        OLED_DrawImage(
            m_pdata->spatialData.currentPosX, m_pdata->spatialData.currentPosY, m_pdata->img, OLED_COLOR_NORMAL
        );
    }

    if (m_pdata->deathData.isDead) {
        // Draw death animation or effect
        // 绘制死亡扩散圆环特效
        uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
        uint8_t radius  = (feilianEnemyDeadTime - m_pdata->deathData.deathTimer) / 100; // 计算半径（随死亡计时变化）
        radius          = etl::max(radius, uint8_t(1));                                 // 保证最小半径为1

        OLED_DrawCircle(centerX, centerY, radius, OLED_COLOR_NORMAL);
    }
}

void FeilianEnemy::die() {
    // Implement enemy death logic
    if (m_pdata->deathData.deathTimer > 0) {
        m_pdata->deathData.deathTimer -= controlDelayTime;
        m_pdata->deathData.deathTimer = etl::max(m_pdata->deathData.deathTimer, uint16_t(0));
        return;
    }
    m_pdata->isActive = false;
}
/*******************************************************************/

/*******************************************************************/
/**
 * @brief GudiaoEnemy 类
 * @note  本类表示“蛊雕”敌人，属于中等规模敌群，具有中距移动与间歇性远程攻击能力。
 * @note  参考尺寸约 15x15 像素（OLED 显示比例），生命与攻击随 `level` 增强。
 * @note  行为特征：以常规移动为主，间隔性发射火球或特殊子弹并可能伴随爆炸效果。
 */
GudiaoEnemy::GudiaoEnemy(
    uint8_t startX, uint8_t startY, uint8_t initPosX, uint8_t initPosY, uint8_t level, uint16_t dropExp
)
: IRole() {
    // 蛊雕敌人数值设定
    // 血量: 20 + level * 150
    // 攻击力: 8 + level * 2
    // 移动速度：1 较慢
    // 射击冷却时间：16000/5 = 3200ms
    // 热量信息：每次射击消耗 20、最大储存 150、冷却速率 10/2000ms
    // 图片信息
    m_pdata->img = &GudiaoImg;

    // 身份信息
    m_pdata->identity          = RoleIdentity::ENEMY;
    m_pdata->isActive          = true;
    m_pdata->initData.isInited = false;

    // 等级信息
    m_pdata->level = level;

    // 血量信息
    m_pdata->healthData.currentHealth = 20 + level * 150;
    m_pdata->healthData.maxHealth     = 20 + level * 150;

    // 回血信息
    m_pdata->healthData.healValue       = 5;
    m_pdata->healthData.healTimeCounter = 0;
    m_pdata->healthData.healResetTime   = 15000;
    m_pdata->healthData.healSpeed       = 5;

    // 空间/移动信息
    m_pdata->spatialData.canCrossBorder            = false;
    m_pdata->spatialData.currentPosX               = startX; // 起始 X
    m_pdata->spatialData.currentPosY               = startY; // 起始 Y
    m_pdata->spatialData.refPosX                   = startX;
    m_pdata->spatialData.refPosY                   = startY;
    m_pdata->spatialData.sizeX                     = m_pdata->img->w;
    m_pdata->spatialData.sizeY                     = m_pdata->img->h;
    m_pdata->spatialData.moveSpeed                 = 1; // 移动速度
    m_pdata->spatialData.consecutiveCollisionCount = 0;

    // 初始化位置
    m_pdata->initData.posX = initPosX;
    m_pdata->initData.posY = initPosY;

    // 攻击信息
    m_pdata->attackData.attackPower            = 8 + level * 2;
    m_pdata->attackData.shootCooldownSpeed     = 5;
    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 16000; // 16000 ms
    m_pdata->attackData.bulletSpeed            = 1;

    m_pdata->attackData.bulletRange            = 10; // 子弹有效射程
    m_pdata->attackData.bulletDamageMultiplier = 1.5f;

    m_pdata->attackData.collisionPower = 4 + level * 1;

    // 热量（能量）信息
    m_pdata->heatData.maxHeat          = 150;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 20;
    m_pdata->heatData.heatCoolDownRate = 10; // 每次冷却 10 单位（计时单位约 200ms）

    // 状态信息
    m_pdata->deathData.deathTimer           = gudiaoEnemyDeadTime;
    m_pdata->deathData.isDead               = false;
    m_pdata->deathData.dropExperiencePoints = dropExp;

    // Initialize other enemy-specific data here
}

void GudiaoEnemy::init() {
    m_pdata->initData.init_count += controlDelayTime;
    // Initialize enemy role specifics
    if (m_pdata->initData.init_count < 30) { // 每30ms移动一次
        return;
    }
    m_pdata->initData.init_count = 0;

    if (m_pdata->spatialData.currentPosX > m_pdata->initData.posX) {
        m_pdata->spatialData.currentPosX -= 1;
    } else if (m_pdata->spatialData.currentPosX < m_pdata->initData.posX) {
        m_pdata->spatialData.currentPosX += 1;
    } else {
        m_pdata->initData.isInited   = true;
        m_pdata->spatialData.refPosX = m_pdata->spatialData.currentPosX;
        m_pdata->spatialData.refPosY = m_pdata->spatialData.currentPosY;
        m_pdata->initData.init_count = 0;
    }
}

void GudiaoEnemy::think() {
    // Implement enemy AI logic
    think_count += controlDelayTime;
    if (think_count < 200) // 每200ms进行一次判断
        return;

    think_count = 0;

    uint8_t randomAction = rand() % 6;
    // Random action: 0 - move left, 1 - move right, 2 - move down, 3 - move up, 4 - stay still, 5 - shoot
    if (m_pdata->actionData.currentState == ActionState::IDLE) {
        //�ƶ�
        if (randomAction == 0) {
            m_pdata->actionData.moveMode     = MoveMode::LEFT;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 1) {
            m_pdata->actionData.moveMode     = MoveMode::RIGHT;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 2) {
            m_pdata->actionData.moveMode     = MoveMode::DOWN;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 3) {
            m_pdata->actionData.moveMode     = MoveMode::UP;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 4) {
            // Stay still
            m_pdata->actionData.moveMode     = MoveMode::NONE;
            m_pdata->actionData.currentState = ActionState::MOVING;
        }

        // 攻击
        else if (randomAction == 5) {
            if (m_pdata->attackData.shootCooldownTimer <= 0) {
                m_pdata->actionData.attackMode   = AttackMode::MODE_1;
                m_pdata->actionData.currentState = ActionState::ATTACKING;
            } else {
                // 攻击冷却等待中，无法进行攻击，切换回空闲状态
                m_pdata->actionData.moveMode     = MoveMode::NONE;
                m_pdata->actionData.currentState = ActionState::IDLE;
            }
        }
    }
}

void GudiaoEnemy::doAction() {
    if (m_pdata->initData.isInited == false) {
        return;
    }

    // Implement enemy action logic
    if (m_pdata->deathData.isDead) {
        return;
    }
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
        // Move logic handled in think()
        break;
    case ActionState::ATTACKING:

        // 攻击冷却等待中
        if (m_pdata->attackData.shootCooldownTimer > 0) break;

        uint8_t m_x = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t m_y = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

        uint8_t m_x_1 = m_x;
        uint8_t m_y_1 = m_y - 3;
        uint8_t m_x_2 = m_x;
        uint8_t m_y_2 = m_y + 3;
        switch (m_pdata->actionData.attackMode) {
        case AttackMode::MODE_1:
            shoot(m_x_1, m_y_1, BulletType::BASIC);
            m_pdata->attackData.shootCooldownTimer = 0; // 攻击冷却等待中，无法进行攻击，切换回空闲状态
            shoot(m_x_2, m_y_2, BulletType::BASIC);
            break;
        default:
            break;
        }
        m_pdata->actionData.currentState = ActionState::IDLE;
        m_pdata->actionData.attackMode   = AttackMode::NONE;
        break;
    }
}

void GudiaoEnemy::drawRole() {
    if (m_pdata->img != nullptr && m_pdata->isActive && !m_pdata->deathData.isDead) {
        OLED_DrawImage(
            m_pdata->spatialData.currentPosX, m_pdata->spatialData.currentPosY, m_pdata->img, OLED_COLOR_NORMAL
        );
    }

    if (m_pdata->deathData.isDead) {
        // Draw death animation or effect
        // 绘制死亡特效（扩散圆环）
        uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
        uint8_t radius  = (gudiaoEnemyDeadTime - m_pdata->deathData.deathTimer) / 100; // 计算半径（随死亡计时变化）
        radius          = etl::max(radius, uint8_t(1));                                // 保证最小半径为1

        OLED_DrawCircle(centerX, centerY, radius, OLED_COLOR_NORMAL);
    }
}

void GudiaoEnemy::die() {
    // Implement enemy death logic
    if (m_pdata->deathData.deathTimer > 0) {
        m_pdata->deathData.deathTimer -= controlDelayTime;
        m_pdata->deathData.deathTimer = etl::max(m_pdata->deathData.deathTimer, uint16_t(0));
        return;
    }

    // 生成火球：创建并发射一个火球子弹
    IBullet *newBullet = createBullet(
        m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2,
        m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2, BulletType::FIRE_BALL
    );
    if (newBullet != nullptr) {
        if (!g_entityManager.addBullet(newBullet)) {
            delete newBullet; // Clean up if not added
        }
    }
    m_pdata->isActive = false;
}

/*******************************************************************/

/*******************************************************************/
/**
 * @brief ChiMeiEnemy 类
 * @note  本类表示“魑魅”小型敌人，体型小巧、移动灵活，擅长近中距离快速移动并进行点射攻击。
 * @note  参考尺寸约 8x8 像素（OLED 显示比例），适合作为群体出现的弱敌。
 */

ChiMeiEnemy::ChiMeiEnemy(
    uint8_t startX, uint8_t startY, uint8_t initPosX, uint8_t initPosY, uint8_t level, uint16_t dropExp
)
: IRole() {
    // 图片信息
    m_pdata->img = &ChiMeiImg;

    // 身份信息
    m_pdata->identity          = RoleIdentity::ENEMY;
    m_pdata->isActive          = true;
    m_pdata->initData.isInited = false;

    // 等级信息
    m_pdata->level = level;

    // 血量信息
    m_pdata->healthData.currentHealth = 1 + level * 1;
    m_pdata->healthData.maxHealth     = 1 + level * 1;

    // 初始化信息
    m_pdata->healthData.healValue       = 0;
    m_pdata->healthData.healTimeCounter = 0;
    m_pdata->healthData.healResetTime   = 15000;
    m_pdata->healthData.healSpeed       = 0;

    // 空间/移动信息
    m_pdata->spatialData.canCrossBorder            = false;
    m_pdata->spatialData.currentPosX               = startX; // Starting X position
    m_pdata->spatialData.currentPosY               = startY; // Starting Y position
    m_pdata->spatialData.refPosX                   = startX;
    m_pdata->spatialData.refPosY                   = startY;
    m_pdata->spatialData.sizeX                     = m_pdata->img->w;
    m_pdata->spatialData.sizeY                     = m_pdata->img->h;
    m_pdata->spatialData.moveSpeed                 = 3; // Set movement speed
    m_pdata->spatialData.consecutiveCollisionCount = 0;

    // 初始化位置
    m_pdata->initData.posX = initPosX;
    m_pdata->initData.posY = initPosY;

    // 攻击信息
    m_pdata->attackData.attackPower            = 1 + level * 1;
    m_pdata->attackData.shootCooldownSpeed     = 5;
    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 4000;
    m_pdata->attackData.bulletSpeed            = 2;

    m_pdata->attackData.bulletRange            = 0;    // 子弹无额外射程效果
    m_pdata->attackData.bulletDamageMultiplier = 1.5f; // 子弹伤害倍率

    m_pdata->attackData.collisionPower = 20;

    // 热量（能量）信息
    m_pdata->heatData.maxHeat          = 100;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 20;
    m_pdata->heatData.heatCoolDownRate = 10; // 每次冷却 10 单位（计时单位约 200ms）

    // 状态信息
    m_pdata->deathData.deathTimer           = chimeiEnemyDeadTime;
    m_pdata->deathData.isDead               = false;
    m_pdata->deathData.dropExperiencePoints = dropExp;

    // Initialize other enemy-specific data here
}

void ChiMeiEnemy::init() {
    m_pdata->initData.init_count += controlDelayTime;
    // Initialize enemy role specifics

    if (m_pdata->spatialData.currentPosX > m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 30) { // 每30ms移动一次
            m_pdata->spatialData.currentPosX -= 1;
            m_pdata->initData.init_count = 0;
        }
    } else if (m_pdata->spatialData.currentPosX < m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 30) { // 每30ms移动一次
            m_pdata->spatialData.currentPosX += 1;
            m_pdata->initData.init_count = 0;
        }
    } else {
        m_pdata->initData.isInited   = true;
        m_pdata->spatialData.refPosX = m_pdata->spatialData.currentPosX;
        m_pdata->spatialData.refPosY = m_pdata->spatialData.currentPosY;
        m_pdata->initData.init_count = 0;
    }
}

void ChiMeiEnemy::think() {
    // Implement enemy AI logic
    think_count += controlDelayTime;
    if (think_count < 150) // 每150ms执行一次思考
        return;

    think_count = 0;

    // 仅移动逻辑
    if (m_pdata->actionData.currentState == ActionState::IDLE) {
        // 移动
        m_pdata->actionData.moveMode     = MoveMode::LEFT;
        m_pdata->actionData.currentState = ActionState::MOVING;
    }
}

void ChiMeiEnemy::doAction() {
    if (m_pdata->initData.isInited == false) {
        return;
    }

    // 到达边界则判定死亡
    if (m_pdata->spatialData.currentPosX <= boundary_deadzone) {
        m_pdata->deathData.isDead = true;
    }

    // Implement enemy action logic
    if (m_pdata->deathData.isDead) {
        return;
    }
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
        // Move logic handled in think()
        break;
    case ActionState::ATTACKING:
        uint8_t m_x = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t m_y = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
        switch (m_pdata->actionData.attackMode) {
        case AttackMode::MODE_1:
            shoot(m_x, m_y, BulletType::BASIC);
            break;
        default:
            break;
        }
        m_pdata->actionData.currentState = ActionState::IDLE;
        m_pdata->actionData.attackMode   = AttackMode::NONE;
        break;
    }
}

void ChiMeiEnemy::drawRole() {
    if (m_pdata->img != nullptr && m_pdata->isActive && !m_pdata->deathData.isDead) {
        OLED_DrawImage(
            m_pdata->spatialData.currentPosX, m_pdata->spatialData.currentPosY, m_pdata->img, OLED_COLOR_NORMAL
        );
    }

    if (m_pdata->deathData.isDead) {
        // Draw death animation or effect
        // 绘制死亡特效（扩散圆环）
        uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
        uint8_t radius  = (chimeiEnemyDeadTime - m_pdata->deathData.deathTimer) / 100; // 从0逐渐增大到最大值5
        radius          = etl::max(radius, uint8_t(1));                                // 最小半径为1

        OLED_DrawCircle(centerX, centerY, radius, OLED_COLOR_NORMAL);
    }
}

void ChiMeiEnemy::die() {
    // Implement enemy death logic
    if (m_pdata->deathData.deathTimer > 0) {
        m_pdata->deathData.deathTimer -= controlDelayTime;
        m_pdata->deathData.deathTimer = etl::max(m_pdata->deathData.deathTimer, uint16_t(0));
        return;
    }
    m_pdata->isActive = false;
}

/*******************************************************************/

/*******************************************************************/
/**
 * @brief BoEnemy 类（精英级中型敌人）
 * @note  中文：驳 ｜ 英文：Bo
 * @note  神话典故：驳为马形神兽，白身黑尾、头生独角，声如鼓音，能捕食猛兽，具有威慑力。
 * @note  本类为精英级中型敌人，体型约 24x24 像素，血量与攻击力较高，移动为中速。
 * @note  行为特征：直接且凶猛，包含冲锋践踏（冲锋碰撞伤害）、扇形爪击与横向鼓音冲击波等攻击模式。
 *
 * @note  === 攻击方式 ===
 * @note  MODE_1: 冲锋践踏 - 向玩家方向快速直线冲锋，造成碰撞伤害（ChargeDistance=40，ChargeTime=800ms）
 * @note  MODE_2: 虎牙利爪 - 发射 3 发扇形普通子弹（ClawAttackTime=200ms）
 * @note  MODE_3: 鼓音震荡 - 发射横向冲击波子弹（DrumSoundTime=300ms）
 */

// 数值设定参考（精英级，比普通敌人强，比 BOSS 弱）
// 血量：80 + level * 80（中等血量）
// 攻击力：6 + level * 2（较高攻击力）
// 移动速度：2（中速移动）
// 碰撞伤害：10 + level * 3（较高碰撞伤害）

BoEnemy::BoEnemy(uint8_t startX, uint8_t startY, uint8_t initPosX, uint8_t initPosY, uint8_t level, uint16_t dropExp)
: IRole() {
    // 图片信息（参考 font.c 中的 BoImg）
    m_pdata->img = &BoImg;

    // 身份信息
    m_pdata->identity          = RoleIdentity::ENEMY;
    m_pdata->isActive          = true;
    m_pdata->initData.isInited = false;

    // 攻击相关信息
    m_pdata->level = level;

    // 血量信息
    m_pdata->healthData.currentHealth = 40 + level * 200;
    m_pdata->healthData.maxHealth     = 40 + level * 200;

    // 回血信息
    m_pdata->healthData.healValue       = 2;
    m_pdata->healthData.healTimeCounter = 0;
    m_pdata->healthData.healResetTime   = 10000;
    m_pdata->healthData.healSpeed       = 2;

    // 空间/移动信息
    m_pdata->spatialData.canCrossBorder            = false;
    m_pdata->spatialData.currentPosX               = startX;
    m_pdata->spatialData.currentPosY               = startY;
    m_pdata->spatialData.refPosX                   = startX;
    m_pdata->spatialData.refPosY                   = startY;
    m_pdata->spatialData.sizeX                     = m_pdata->img->w;
    m_pdata->spatialData.sizeY                     = m_pdata->img->h;
    m_pdata->spatialData.moveSpeed                 = 2; // 移动速度
    m_pdata->spatialData.consecutiveCollisionCount = 0;

    // 初始化位置
    m_pdata->initData.posX = initPosX;
    m_pdata->initData.posY = initPosY;

    // 攻击信息
    m_pdata->attackData.attackPower            = 6 + level * 2;
    m_pdata->attackData.shootCooldownSpeed     = 5;
    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 8000; // 8000/5=1600ms冷却
    m_pdata->attackData.bulletSpeed            = 1;

    m_pdata->attackData.bulletRange            = 8; // 火球子弹范围
    m_pdata->attackData.bulletDamageMultiplier = 1.5f;

    m_pdata->attackData.collisionPower = 10 + level * 3; // 较高碰撞伤害

    // 热量信息
    m_pdata->heatData.maxHeat          = 120;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 15;
    m_pdata->heatData.heatCoolDownRate = 10;

    // 状态信息
    m_pdata->deathData.deathTimer           = boEnemyDeadTime;
    m_pdata->deathData.isDead               = false;
    m_pdata->deathData.dropExperiencePoints = dropExp;

    // 初始化动作模式状态
    chargeStarted    = false;
    chargeDirectionX = 0;
    chargeDirectionY = 0;
}

void BoEnemy::init() {
    m_pdata->initData.init_count += controlDelayTime;

    // 缓慢移动到初始位置
    if (m_pdata->spatialData.currentPosX > m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 20) { // 每20ms移动一次
            m_pdata->spatialData.currentPosX -= 1;
            m_pdata->initData.init_count = 0;
        }
    } else if (m_pdata->spatialData.currentPosX < m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 20) { // 每20ms移动一次
            m_pdata->spatialData.currentPosX += 1;
            m_pdata->initData.init_count = 0;
        }
    } else {
        m_pdata->initData.isInited       = true;
        m_pdata->spatialData.refPosX     = m_pdata->spatialData.currentPosX;
        m_pdata->spatialData.refPosY     = m_pdata->spatialData.currentPosY;
        m_pdata->actionData.currentState = ActionState::IDLE;
        m_pdata->initData.init_count     = 0;
    }
}

void BoEnemy::think() {
    think_count += controlDelayTime;
    if (think_count < 150) // 每150ms执行一次思考
        return;

    think_count = 0;

    if (m_pdata->actionData.currentState == ActionState::IDLE) {
        // 获取玩家（当前未使用）
        IRole *player = g_entityManager.getPlayerRole();
        (void)player; // 玩家变量暂未使用

        uint8_t randomAction = rand() % 10;

        if (randomAction < 3) {
            // 30% 概率移动
            uint8_t moveDir                  = rand() % 4;
            m_pdata->actionData.currentState = ActionState::MOVING;
            switch (moveDir) {
            case 0:
                m_pdata->actionData.moveMode = MoveMode::LEFT;
                break;
            case 1:
                m_pdata->actionData.moveMode = MoveMode::RIGHT;
                break;
            case 2:
                m_pdata->actionData.moveMode = MoveMode::UP;
                break;
            case 3:
                m_pdata->actionData.moveMode = MoveMode::DOWN;
                break;
            }
        } else if (randomAction < 5) {
            // 20% 保持空闲
            m_pdata->actionData.currentState = ActionState::IDLE;
        } else {
            // 50% 选择攻击
            m_pdata->actionData.currentState = ActionState::ATTACKING;

            uint8_t attackChoice = rand() % 6;
            switch (attackChoice) {
            case 0:
            case 1:
                // MODE_1: 冲锋践踏 (约33%)
                action_timer                   = ChargeTime;
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                chargeStarted                  = false;
                m_pdata->actionData.attackMode = AttackMode::MODE_1;
                break;
            case 2:
            case 3:
                // MODE_2: 虎牙利爪 (约33%)
                action_timer                   = ClawAttackTime;
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                m_pdata->actionData.attackMode = AttackMode::MODE_2;
                break;
            case 4:
            case 5:
                // MODE_3: 鼓音震荡 (约33%)
                action_timer                   = DrumSoundTime;
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                m_pdata->actionData.attackMode = AttackMode::MODE_3;
                break;
            }
        }
    }
}

void BoEnemy::doAction() {
    if (m_pdata->initData.isInited == false) {
        return;
    }

    if (m_pdata->deathData.isDead) {
        return;
    }

    switch (m_pdata->actionData.currentState) {
    case ActionState::IDLE:
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

    case ActionState::ATTACKING:
        // 记录动作时间
        action_count += controlDelayTime;

        // 更新动作计时
        if (action_timer >= controlDelayTime)
            action_timer -= controlDelayTime;
        else
            action_timer = 0;

        // 执行对应的攻击动作
        switch (m_pdata->actionData.attackMode) {
        case AttackMode::MODE_1:
            chargeTowardsPlayer();
            break;
        case AttackMode::MODE_2:
            tigerClawAttack();
            break;
        case AttackMode::MODE_3:
            drumSoundWave();
            break;
        default:
            break;
        }

        // 动作结束处理
        if (action_timer == 0) {
            m_pdata->actionData.currentState       = ActionState::IDLE;
            m_pdata->actionData.attackMode         = AttackMode::NONE;
            action_count                           = 0;
            m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;

            // 重置冲锋状态
            chargeStarted = false;
        }
        break;
    }
}

void BoEnemy::drawRole() {
    if (m_pdata->img != nullptr && m_pdata->isActive && !m_pdata->deathData.isDead) {
        OLED_DrawImage(
            m_pdata->spatialData.currentPosX, m_pdata->spatialData.currentPosY, m_pdata->img, OLED_COLOR_NORMAL
        );

        // MODE_1 期间绘制尾部轨迹效果
        if (m_pdata->actionData.attackMode == AttackMode::MODE_1 && chargeStarted) {
            uint8_t tailX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX;
            uint8_t tailY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

            OLED_DrawLine(
                tailX - chargeDirectionX * 0, tailY - chargeDirectionY * 0, tailX - chargeDirectionX * 8,
                tailY - chargeDirectionY * 4, OLED_COLOR_NORMAL
            );
        }
    }

    // 死亡特效
    if (m_pdata->deathData.isDead) {
        uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
        uint8_t radius  = (boEnemyDeadTime - m_pdata->deathData.deathTimer) * 15 / boEnemyDeadTime;
        radius          = etl::max(radius, uint8_t(1));

        // 绘制扩散圆环特效
        OLED_DrawCircle(centerX, centerY, radius, OLED_COLOR_NORMAL);
    }
}

void BoEnemy::die() {
    if (m_pdata->deathData.deathTimer > 0) {
        m_pdata->deathData.deathTimer -= controlDelayTime;
        m_pdata->deathData.deathTimer = etl::max(m_pdata->deathData.deathTimer, uint16_t(0));
        return;
    }
    m_pdata->isActive = false;
}

//=========================== 攻击行为实现 ===========================

/**
 * @brief MODE_1: 冲锋践踏
 * @note  向玩家方向迅速直线冲锋，造成碰撞伤害并可能击退玩家。
 *        用于近战高威胁输出，冲锋期间会绘制尾部轨迹提示。
 */
void BoEnemy::chargeTowardsPlayer() {
    // 冲锋开始时的初始化
    static int16_t safe_dis = 40; // 安全距离，避免与玩家直接重叠
    if (!chargeStarted) {
        chargeStarted = true;

        IRole *player = g_entityManager.getPlayerRole();
        if (player != nullptr) {
            safe_dis = player->getData()->spatialData.sizeX / 2 + m_pdata->spatialData.sizeX / 2 + 50;
            int16_t playerX =
                player->getData()->spatialData.currentPosX + player->getData()->spatialData.sizeX / 2 + safe_dis;
            int16_t playerY = player->getData()->spatialData.currentPosY + player->getData()->spatialData.sizeY / 2;
            int16_t myX     = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
            int16_t myY     = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

            int16_t deltaX = playerX - myX;
            int16_t deltaY = playerY - myY;

            // 计算冲锋方向
            chargeDirectionX = (deltaX > 0) ? 1 : -1; // deltaX>0 表示玩家在右侧
            chargeDirectionY = 0;
            if (deltaY < -5)
                chargeDirectionY = -1;
            else if (deltaY > 5)
                chargeDirectionY = 1;

            if (deltaX == 0) {
                chargeDirectionX = 0;
            }
            if (deltaY == 0) {
                chargeDirectionY = 0;
            }

        } else {
            // 无玩家时使用默认方向（向左）
            chargeDirectionX = -1;
            chargeDirectionY = 0;
        }
    }

    // 每计时分频移动（约每20ms移动一次）
    if (action_count >= 30) {
        action_count = 0;
        // 按加速倍数移动（冲锋为2倍速度）
        move(chargeDirectionX * 2, chargeDirectionY, true);
    }
}

/**
 * @brief MODE_2: 虎牙利爪
 * @note  发射三枚扇形普通子弹（中、上偏移、下偏移），用于中距离群体打击。
 */
void BoEnemy::tigerClawAttack() {
    // 仅在进入动作后且满足最小延时才执行
    if (action_count < 50) return;
    if (action_timer < action_MaxTime - 100) return; // 动作开始后100ms后触发

    uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

    // 发射3枚扇形子弹：中、中上、中下
    m_pdata->attackData.shootCooldownTimer = 0;
    shoot(centerX, centerY, BulletType::BASIC); // 中
    m_pdata->attackData.shootCooldownTimer = 0;
    shoot(centerX, centerY - 3, BulletType::BASIC); // 上偏移
    m_pdata->attackData.shootCooldownTimer = 0;
    shoot(centerX, centerY + 3, BulletType::BASIC); // 下偏移
}

/**
 * @brief MODE_3: 鼓音震荡
 * @note  发射一列横向冲击波子弹，覆盖较宽的 Y 轴范围以限制玩家移动。
 */
void BoEnemy::drumSoundWave() {
    // 仅在进入动作且满足延时后执行
    if (action_count < 50) return;
    if (action_timer < action_MaxTime - 150) return; // 动作开始150ms后触发

    uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

    // 发射一列子弹，覆盖从 -10 到 +10 的 Y 偏移
    for (int8_t offset = -10; offset <= 10; offset += 5) {
        m_pdata->attackData.shootCooldownTimer = 0;
        shoot(centerX, centerY + offset, BulletType::BASIC);
    }
}

/*******************************************************************/
/**
 * @brief ShengyuEnemy 类 - 胜遇 精英敌人（水弹干扰型）
 * @note  中文：胜遇 ｜ 英文：Shengyu
 * @note  神话典故：野鸡形神兽，浑身赤红，出现预示当地将有洪水泛滥。
 * @note  精英级中型敌人，中等血量，低攻击力，中速移动。
 * @note  攻击方式以干扰和遮挡视野为主，核心能力与"水"绑定。
 * @note  === 攻击方式 ===
 * @note  MODE_1: 水雾弥漫 - 生成多个迷雾区域遮挡玩家视线（MistCloudTime=3000ms）
 * @note  MODE_2: 洪波推涌 - 推出长条迷雾屏障（FloodWaveTime=2800ms）
 * @note  MODE_3: 赤羽雷鸣 - 发射一串雷电子弹（RedThunderTime=800ms）
 */
/*******************************************************************/

ShengyuEnemy::ShengyuEnemy(
    uint8_t startX, uint8_t startY, uint8_t initPosX, uint8_t initPosY, uint8_t level, uint16_t dropExp
)
: IRole() {
    // 图片信息（参考 font.c 中的 ShengyuImg）
    m_pdata->img = &ShengyuImg;

    // 身份信息
    m_pdata->identity          = RoleIdentity::ENEMY;
    m_pdata->isActive          = true;
    m_pdata->initData.isInited = false;

    // 等级信息
    m_pdata->level = level;

    // 血量信息: 50 + level * 50（实际生命上限按设计调整）
    m_pdata->healthData.currentHealth = 50 + level * 220;
    m_pdata->healthData.maxHealth     = 50 + level * 220;

    // 回血信息
    m_pdata->healthData.healValue       = 1;
    m_pdata->healthData.healTimeCounter = 0;
    m_pdata->healthData.healResetTime   = 12000;
    m_pdata->healthData.healSpeed       = 2;

    // 空间/移动信息
    m_pdata->spatialData.canCrossBorder            = false;
    m_pdata->spatialData.currentPosX               = startX;
    m_pdata->spatialData.currentPosY               = startY;
    m_pdata->spatialData.refPosX                   = startX;
    m_pdata->spatialData.refPosY                   = startY;
    m_pdata->spatialData.sizeX                     = m_pdata->img->w;
    m_pdata->spatialData.sizeY                     = m_pdata->img->h;
    m_pdata->spatialData.moveSpeed                 = 2; // 中速移动
    m_pdata->spatialData.consecutiveCollisionCount = 0;

    // 初始化位置
    m_pdata->initData.posX = initPosX;
    m_pdata->initData.posY = initPosY;

    // 攻击信息: 4 + level * 1（注：具体数值可按设计微调）
    m_pdata->attackData.attackPower            = 10 + level * 4;
    m_pdata->attackData.shootCooldownSpeed     = 4;
    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 6000; // 6000/4=1500ms冷却
    m_pdata->attackData.bulletSpeed            = 1;

    m_pdata->attackData.bulletRange            = 6;    // 火球弹范围
    m_pdata->attackData.bulletDamageMultiplier = 1.5f; // 雷电弹弹伤害倍率

    // 碰撞伤害: 6 + level * 2（较低碰撞伤害）
    m_pdata->attackData.collisionPower = 6 + level * 2;

    // 热量信息
    m_pdata->heatData.maxHeat          = 80;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 10;
    m_pdata->heatData.heatCoolDownRate = 12;

    // 死亡信息
    m_pdata->deathData.deathTimer           = shengyuEnemyDeadTime;
    m_pdata->deathData.isDead               = false;
    m_pdata->deathData.dropExperiencePoints = dropExp;

    // 初始化攻击模式状态标志
    mistGenerated       = false;
    floodWaveLaunched   = false;
    floodWaveCurrentLen = 0;
    thunderFiredCount   = 0;
    for (uint8_t i = 0; i < MistCloudCount; i++) {
        mistPosX[i] = 0;
        mistPosY[i] = 0;
    }
}

void ShengyuEnemy::init() {
    m_pdata->initData.init_count += controlDelayTime;

    // 缓慢移动到初始位置
    if (m_pdata->spatialData.currentPosX > m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 25) {
            m_pdata->spatialData.currentPosX -= 1;
            m_pdata->initData.init_count = 0;
        }
    } else if (m_pdata->spatialData.currentPosX < m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 25) {
            m_pdata->spatialData.currentPosX += 1;
            m_pdata->initData.init_count = 0;
        }
    } else {
        m_pdata->initData.isInited       = true;
        m_pdata->spatialData.refPosX     = m_pdata->spatialData.currentPosX;
        m_pdata->spatialData.refPosY     = m_pdata->spatialData.currentPosY;
        m_pdata->actionData.currentState = ActionState::IDLE;
        m_pdata->initData.init_count     = 0;
    }
}

void ShengyuEnemy::think() {
    think_count += controlDelayTime;
    if (think_count < 350) return; // 350ms思考周期（包含思考与延迟）

    think_count = 0;

    if (m_pdata->actionData.currentState == ActionState::IDLE) {
        uint8_t randomAction = rand() % 10;

        if (randomAction < 4) { // 40%概率移动
            uint8_t moveDir                  = rand() % 4;
            m_pdata->actionData.currentState = ActionState::MOVING;
            switch (moveDir) {
            case 0:
                m_pdata->actionData.moveMode = MoveMode::LEFT;
                break;
            case 1:
                m_pdata->actionData.moveMode = MoveMode::RIGHT;
                break;
            case 2:
                m_pdata->actionData.moveMode = MoveMode::UP;
                break;
            case 3:
                m_pdata->actionData.moveMode = MoveMode::DOWN;
                break;
            }
        } else if (randomAction < 6) { // 20%概率待机
            m_pdata->actionData.currentState = ActionState::IDLE;
        } else { // 40%概率攻击
            m_pdata->actionData.currentState = ActionState::ATTACKING;

            uint8_t attackChoice = rand() % 10;
            if (attackChoice < 4) { // 40%选择 MODE_1
                action_timer                   = MistCloudTime;
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                mistGenerated                  = false;
                m_pdata->actionData.attackMode = AttackMode::MODE_1;
            } else if (attackChoice < 7) { // 30%选择 MODE_2
                action_timer                   = FloodWaveTime;
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                floodWaveLaunched              = false;
                floodWaveCurrentLen            = 0;
                m_pdata->actionData.attackMode = AttackMode::MODE_2;
            } else { // 30%选择 MODE_3
                action_timer                   = RedThunderTime;
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                thunderFiredCount              = 0;
                m_pdata->actionData.attackMode = AttackMode::MODE_3;
            }
        }
    }
}

void ShengyuEnemy::doAction() {
    if (!m_pdata->initData.isInited || m_pdata->deathData.isDead) return;

    switch (m_pdata->actionData.currentState) {
    case ActionState::IDLE:
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

    case ActionState::ATTACKING:
        action_count += controlDelayTime;

        if (action_timer >= controlDelayTime)
            action_timer -= controlDelayTime;
        else
            action_timer = 0;

        switch (m_pdata->actionData.attackMode) {
        case AttackMode::MODE_1:
            mistCloud();
            break;
        case AttackMode::MODE_2:
            floodWave();
            break;
        case AttackMode::MODE_3:
            redThunder();
            break;
        default:
            break;
        }

        // 执行完成
        if (action_timer == 0) {
            m_pdata->actionData.currentState       = ActionState::IDLE;
            m_pdata->actionData.attackMode         = AttackMode::NONE;
            action_count                           = 0;
            m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;

            // 重置状态标志
            mistGenerated       = false;
            floodWaveLaunched   = false;
            floodWaveCurrentLen = 0;
            thunderFiredCount   = 0;
        }
        break;
    default:
        break;
    }
}

void ShengyuEnemy::drawRole() {
    if (m_pdata->img != nullptr && m_pdata->isActive && !m_pdata->deathData.isDead) {
        OLED_DrawImage(
            m_pdata->spatialData.currentPosX, m_pdata->spatialData.currentPosY, m_pdata->img, OLED_COLOR_NORMAL
        );

        // MODE_1 云雾模式（绘制20x20方块 + 动态装饰）
        if (m_pdata->actionData.attackMode == AttackMode::MODE_1 && mistGenerated) {
            uint8_t flickerPhase = (action_count / 120) % 3; // 闪烁周期3种切换

            for (uint8_t i = 0; i < MistCloudCount; i++) {
                uint8_t px = mistPosX[i];
                uint8_t py = mistPosY[i];

                // 绘制云雾的主体（紧密码，分1条条）
                for (uint8_t dy = 0; dy < MistSize; dy++) {
                    // 根据闪烁周期跳过一些行，产生闪烁效果
                    if (dy % 3 != flickerPhase) {
                        OLED_DrawLine(px, py + dy, px + MistSize - 1, py + dy, OLED_COLOR_NORMAL);
                    }
                }

                // 的角装饰（动态闪烁的角形，最大20x20）
                if (flickerPhase != 0) {
                    // 左上角
                    OLED_DrawLine(px, py, px + 3, py, OLED_COLOR_NORMAL);
                    OLED_DrawLine(px, py, px, py + 3, OLED_COLOR_NORMAL);
                    // 右上角
                    OLED_DrawLine(px + MistSize - 4, py, px + MistSize - 1, py, OLED_COLOR_NORMAL);
                    OLED_DrawLine(px + MistSize - 1, py, px + MistSize - 1, py + 3, OLED_COLOR_NORMAL);
                    // 左下角
                    OLED_DrawLine(px, py + MistSize - 1, px + 3, py + MistSize - 1, OLED_COLOR_NORMAL);
                    OLED_DrawLine(px, py + MistSize - 4, px, py + MistSize - 1, OLED_COLOR_NORMAL);
                    // 右下角
                    OLED_DrawLine(
                        px + MistSize - 4, py + MistSize - 1, px + MistSize - 1, py + MistSize - 1, OLED_COLOR_NORMAL
                    );
                    OLED_DrawLine(
                        px + MistSize - 1, py + MistSize - 4, px + MistSize - 1, py + MistSize - 1, OLED_COLOR_NORMAL
                    );
                }

                // 十字图纹（在每个周期的中间才绘制）
                if (flickerPhase == 1) {
                    uint8_t cx = px + MistSize / 2;
                    uint8_t cy = py + MistSize / 2;
                    OLED_DrawLine(cx - 3, cy, cx + 3, cy, OLED_COLOR_NORMAL);
                    OLED_DrawLine(cx, cy - 3, cx, cy + 3, OLED_COLOR_NORMAL);
                }
            }
        }

        // MODE_2 洪流模式（绘制水流在屏幕前方展开）
        if (m_pdata->actionData.attackMode == AttackMode::MODE_2 && floodWaveLaunched) {
            if (floodWaveCurrentLen > 0) {
                // 获胜者位置计算，前方朝右方展开
                uint8_t drawEndX   = floodWaveEndX;
                uint8_t drawStartX = (drawEndX > floodWaveCurrentLen) ? (drawEndX - floodWaveCurrentLen) : 0;
                uint8_t wavePhase  = (action_count / 80) % 4; // 流动效果

                // 上边界线
                OLED_DrawLine(drawStartX, floodWaveY, drawEndX, floodWaveY, OLED_COLOR_NORMAL);
                // 下边界线
                OLED_DrawLine(
                    drawStartX, floodWaveY + FloodWaveHeight - 1, drawEndX, floodWaveY + FloodWaveHeight - 1,
                    OLED_COLOR_NORMAL
                );

                // 内部动态水纹图（用纹理表达）
                for (uint8_t dy = 2; dy < FloodWaveHeight - 2; dy++) {
                    // 根据时间的Y位置产生动态变化效果
                    uint8_t lineOffset = ((dy + wavePhase) % 4);
                    if (lineOffset < 2) {
                        // 向外扩张线（实线状态）
                        OLED_DrawLine(drawStartX, floodWaveY + dy, drawEndX, floodWaveY + dy, OLED_COLOR_NORMAL);
                    } else if (lineOffset == 2) {
                        // 绘制波浪线纹（间隔交替）
                        for (uint8_t sx = drawStartX; sx < drawEndX; sx += 6) {
                            uint8_t segEnd = (sx + 3 < drawEndX) ? (sx + 3) : drawEndX;
                            OLED_DrawLine(sx, floodWaveY + dy, segEnd, floodWaveY + dy, OLED_COLOR_NORMAL);
                        }
                    }
                    // lineOffset == 3 时空线（形成空隙）
                }

                // 绘制前方箭头线（强调前头）
                if (drawStartX > 2) {
                    OLED_DrawLine(
                        drawStartX, floodWaveY + 2, drawStartX, floodWaveY + FloodWaveHeight - 3, OLED_COLOR_NORMAL
                    );
                    OLED_DrawLine(
                        drawStartX + 1, floodWaveY + 1, drawStartX + 1, floodWaveY + FloodWaveHeight - 2,
                        OLED_COLOR_NORMAL
                    );
                }
            }
        }
    }

    // 死亡特效
    if (m_pdata->deathData.isDead) {
        uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
        uint8_t radius  = (shengyuEnemyDeadTime - m_pdata->deathData.deathTimer) * 12 / shengyuEnemyDeadTime;
        radius          = etl::max(radius, uint8_t(1));
        OLED_DrawCircle(centerX, centerY, radius, OLED_COLOR_NORMAL);
    }
}

void ShengyuEnemy::die() {
    if (m_pdata->deathData.deathTimer > 0) {
        m_pdata->deathData.deathTimer -= controlDelayTime;
        m_pdata->deathData.deathTimer = etl::max(m_pdata->deathData.deathTimer, uint16_t(0));
        return;
    }
    m_pdata->isActive = false;
}

/**
 * @brief MODE_1: 水雾弥漫
 * @note  呼应"大水"典故，在场景中生成多个迷雾区域遮挡玩家视线。
 *        持续时间 MistCloudTime=2000ms，生成 MistCloudCount=4 个迷雾区域。
 *        生成位置随机分布，产生闪烁动画效果。
 */
void ShengyuEnemy::mistCloud() {
    // 初期延迟50ms后开始生成迷雾区域
    if (!mistGenerated && action_count >= 50) {
        mistGenerated = true;

        // 在屏幕可见区域随机生成迷雾块位置（保持与敌人有距离）
        for (uint8_t i = 0; i < MistCloudCount; i++) {
            // 分配X位置：10-70之间，Y位置：5-50之间
            mistPosX[i] = 10 + (rand() % 60);
            mistPosY[i] = 5 + (rand() % 45);
        }
    }
    // 后续由drawRole()中绘制迷雾视效，整个持续时间内保持显示
}

/**
 * @brief MODE_2: 洪波推涌
 * @note  往前推出一长条迷雾屏障，从发射位置开始逐渐消散。
 *        持续时间 FloodWaveTime=1500ms，迷雾条长度 FloodWaveLength=80 像素。
 *        分为三个阶段：增长期、保持期、消散期，形成动态的洪流视效。
 */
void ShengyuEnemy::floodWave() {
    // 初期延迟50ms后开始发射洪波
    if (!floodWaveLaunched && action_count >= 50) {
        floodWaveLaunched = true;

        // 洪波末端位置（从敌人当前X位置开始向前延伸）
        floodWaveEndX = m_pdata->spatialData.currentPosX;
        // 洪波中心Y位置（对齐敌人中心）
        floodWaveY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2 - FloodWaveHeight / 2;

        // 边界检查，确保洪波完全在屏幕内
        if (floodWaveY < 2) floodWaveY = 2;
        if (floodWaveY + FloodWaveHeight > 62) floodWaveY = 62 - FloodWaveHeight;
        floodWaveCurrentLen = 0;
    }

    if (floodWaveLaunched) {
        // ========== 第一阶段：增长期（0-1000ms） ==========
        // 洪流快速向前推进，宽度不断增长
        if (action_count < 1000) {
            // 每40ms增长12像素宽度，实现平稳加速的视觉效果
            if (action_count > 50) {
                floodWaveCurrentLen = ((action_count - 50) / 40) * 12;
            }
            if (floodWaveCurrentLen > FloodWaveLength) floodWaveCurrentLen = FloodWaveLength;
        }
        // ========== 第二阶段：保持期（1000-1800ms） ==========
        // 洪流保持最大宽度，为玩家制造持续压力
        else if (action_count < 1800) {
            floodWaveCurrentLen = FloodWaveLength;
        }
        // ========== 第三阶段：消散期（1800-2800ms） ==========
        // 洪流逐渐向后收缩并消散
        else {
            uint16_t fadeTime   = action_count - 1800;
            uint8_t  fadeAmount = (fadeTime / 80) * 11; // 每80ms消散11个像素
            if (fadeAmount >= FloodWaveLength) {
                floodWaveCurrentLen = 0;
            } else {
                floodWaveCurrentLen = FloodWaveLength - fadeAmount;
            }
        }
    }
}

/**
 * @brief MODE_3: 赤羽雷鸣
 * @note  呼应"赤"色特征，在自身中央发射一串雷电子弹形成瀑布般的打击。
 *        持续时间 RedThunderTime=300ms，每 ThunderInterval=150ms 发射一发子弹，
 *        总共发射 ThunderBulletCount=5 发，形成从上到下的连贯打击。
 */
void ShengyuEnemy::redThunder() {
    // 每隔 ThunderInterval(150ms) 发射一发子弹，直到达到总发射数量
    if (action_count >= ThunderInterval * (thunderFiredCount + 1) && thunderFiredCount < ThunderBulletCount) {
        thunderFiredCount++;

        uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

        // 计算Y轴偏移，使子弹形成扇形散开（模拟雷鸣从上到下的特效）
        int8_t yOffset = (thunderFiredCount % 2 == 0) ? (thunderFiredCount - 3) : (3 - thunderFiredCount);

        m_pdata->attackData.shootCooldownTimer = 0; // 重置冷却
        shoot(centerX, centerY + yOffset, BulletType::BASIC);
    }
}

/*******************************************************************/
/**
 * @brief LiliEnemy class - 狸力 精英敌人
 * @note  中文：狸力 ｜ 英文：Lili
 * @note  神话典故：《山海经·南山经》记载："柜山，有兽焉，其状如豚，有距，其音如狗吠，其名曰狸力，见则其县多土功。"
 * @note  狸力是一种猪形神兽，长有利爪（距），叫声如狗吠，出现则预示当地将有大量土木工程。
 * @note  精英级中型敌人，体型中等，中等血量，中等攻击力，较快移动。
 * @note  会与普通敌人一同出现，攻击方式以土系和声波为主。
 * @note  === 攻击方式 ===
 * @note  MODE_1: 土涌突刺 - 呼应"土功"，在前方间隔发射火球弹（土块爆炸）
 *               持续时间 EarthSurgeTime=1200ms，每 EarthSurgeInterval=300ms 发射一发火球
 * @note  MODE_2: 獠吠震波 - 呼应"其音如狗吠"，发射5发扇形普通子弹阵
 *               持续时间 BarkWaveTime=400ms，一次性发射
 * @note  MODE_3: 穴地陷阱 - 呼应"见则其县多土功"，在随机位置挖掘陷阱后爆炸
 *               持续时间 BurrowTrapTime=800ms，先标记2个陷阱位置，延迟后发射火球
 */
/*******************************************************************/

LiliEnemy::LiliEnemy(
    uint8_t startX, uint8_t startY, uint8_t initPosX, uint8_t initPosY, uint8_t level, uint16_t dropExp
)
: IRole() {
    // 图片信息
    m_pdata->img = &LiliImg;

    // 身份信息
    m_pdata->identity          = RoleIdentity::ENEMY;
    m_pdata->isActive          = true;
    m_pdata->initData.isInited = false;

    // 等级信息
    m_pdata->level = level;

    // 血量信息: 60 + level * 160
    m_pdata->healthData.currentHealth = 60 + level * 160;
    m_pdata->healthData.maxHealth     = 60 + level * 160;

    // 回血信息
    m_pdata->healthData.healValue       = 1;
    m_pdata->healthData.healTimeCounter = 0;
    m_pdata->healthData.healResetTime   = 12000;
    m_pdata->healthData.healSpeed       = 2;

    // 空间/移动信息
    m_pdata->spatialData.canCrossBorder            = false;
    m_pdata->spatialData.currentPosX               = startX;
    m_pdata->spatialData.currentPosY               = startY;
    m_pdata->spatialData.refPosX                   = startX;
    m_pdata->spatialData.refPosY                   = startY;
    m_pdata->spatialData.sizeX                     = m_pdata->img->w;
    m_pdata->spatialData.sizeY                     = m_pdata->img->h;
    m_pdata->spatialData.moveSpeed                 = 2; // 移动速度
    m_pdata->spatialData.consecutiveCollisionCount = 0;

    // 初始化位置
    m_pdata->initData.posX = initPosX;
    m_pdata->initData.posY = initPosY;

    // 攻击信息: 6 + level * 3
    m_pdata->attackData.attackPower            = 6 + level * 3;
    m_pdata->attackData.shootCooldownSpeed     = 4;
    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 6000; // 6000/4=1500ms冷却
    m_pdata->attackData.bulletSpeed            = 1;

    m_pdata->attackData.bulletRange            = 6; // 火球弹范围
    m_pdata->attackData.bulletDamageMultiplier = 1.2f;

    // 碰撞伤害: 8 + level * 2（中等碰撞伤害）
    m_pdata->attackData.collisionPower = 8 + level * 2;

    // 热量信息
    m_pdata->heatData.maxHeat          = 100;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 12;
    m_pdata->heatData.heatCoolDownRate = 12;

    // 状态信息
    m_pdata->deathData.deathTimer           = liliEnemyDeadTime;
    m_pdata->deathData.isDead               = false;
    m_pdata->deathData.dropExperiencePoints = dropExp;

    // 初始化攻击模式状态标志
    barkFired       = false;
    earthSurgeCount = 0;
    trapPlaced      = false;
    trapExploded    = false;
    for (uint8_t i = 0; i < TrapCount; i++) {
        trapPosX[i] = 0;
        trapPosY[i] = 0;
    }
}

void LiliEnemy::init() {
    m_pdata->initData.init_count += controlDelayTime;

    if (m_pdata->spatialData.currentPosX > m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 25) {
            m_pdata->spatialData.currentPosX -= 1;
            m_pdata->initData.init_count = 0;
        }
    } else if (m_pdata->spatialData.currentPosX < m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 25) {
            m_pdata->spatialData.currentPosX += 1;
            m_pdata->initData.init_count = 0;
        }
    } else {
        m_pdata->initData.isInited       = true;
        m_pdata->spatialData.refPosX     = m_pdata->spatialData.currentPosX;
        m_pdata->spatialData.refPosY     = m_pdata->spatialData.currentPosY;
        m_pdata->actionData.currentState = ActionState::IDLE;
        m_pdata->initData.init_count     = 0;
    }
}

void LiliEnemy::think() {
    think_count += controlDelayTime;
    if (think_count < 300) return; // 300ms思考周期（包含思考与延迟）

    think_count = 0;

    if (m_pdata->actionData.currentState == ActionState::IDLE) {
        uint8_t randomAction = rand() % 10;

        if (randomAction < 4) { // 40%概率移动
            uint8_t moveDir                  = rand() % 4;
            m_pdata->actionData.currentState = ActionState::MOVING;
            switch (moveDir) {
            case 0:
                m_pdata->actionData.moveMode = MoveMode::LEFT;
                break;
            case 1:
                m_pdata->actionData.moveMode = MoveMode::RIGHT;
                break;
            case 2:
                m_pdata->actionData.moveMode = MoveMode::UP;
                break;
            case 3:
                m_pdata->actionData.moveMode = MoveMode::DOWN;
                break;
            }
        } else if (randomAction < 5) { // 10%保持静止（原10%）
            m_pdata->actionData.currentState = ActionState::IDLE;
        } else { // 50%攻击（原60%）
            m_pdata->actionData.currentState = ActionState::ATTACKING;

            uint8_t attackChoice = rand() % 6;
            switch (attackChoice) {
            case 0:
            case 1:
                action_timer                   = EarthSurgeTime;
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                earthSurgeCount                = 0;
                m_pdata->actionData.attackMode = AttackMode::MODE_1;
                break;
            case 2:
            case 3:
                action_timer                   = BarkWaveTime;
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                barkFired                      = false;
                m_pdata->actionData.attackMode = AttackMode::MODE_2;
                break;
            case 4:
            case 5:
                action_timer                   = BurrowTrapTime;
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                trapPlaced                     = false;
                trapExploded                   = false;
                m_pdata->actionData.attackMode = AttackMode::MODE_3;
                break;
            }
        }
    }
}

void LiliEnemy::doAction() {
    if (!m_pdata->initData.isInited || m_pdata->deathData.isDead) return;

    switch (m_pdata->actionData.currentState) {
    case ActionState::IDLE:
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

    case ActionState::ATTACKING:
        action_count += controlDelayTime;

        if (action_timer >= controlDelayTime)
            action_timer -= controlDelayTime;
        else
            action_timer = 0;

        switch (m_pdata->actionData.attackMode) {
        case AttackMode::MODE_1:
            earthSurge();
            break;
        case AttackMode::MODE_2:
            barkWave();
            break;
        case AttackMode::MODE_3:
            burrowTrap();
            break;
        default:
            break;
        }

        // 冷却计时
        if (action_timer == 0) {
            m_pdata->actionData.currentState       = ActionState::IDLE;
            m_pdata->actionData.attackMode         = AttackMode::NONE;
            action_count                           = 0;
            m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;

            // 重置状态变量
            barkFired       = false;
            earthSurgeCount = 0;
            trapPlaced      = false;
            trapExploded    = false;
        }
        break;
    default:
        break;
    }
}

void LiliEnemy::drawRole() {
    if (m_pdata->img != nullptr && m_pdata->isActive && !m_pdata->deathData.isDead) {
        OLED_DrawImage(
            m_pdata->spatialData.currentPosX, m_pdata->spatialData.currentPosY, m_pdata->img, OLED_COLOR_NORMAL
        );

        // MODE_3陷阱模式 - 在陷阱未爆炸前显示陷阱位置标记
        if (m_pdata->actionData.attackMode == AttackMode::MODE_3 && trapPlaced && !trapExploded) {
            for (uint8_t i = 0; i < TrapCount; i++) {
                // 陷阱位置标记，绘制X形图案
                OLED_DrawLine(trapPosX[i] - 3, trapPosY[i] - 3, trapPosX[i] + 3, trapPosY[i] + 3, OLED_COLOR_NORMAL);
                OLED_DrawLine(trapPosX[i] - 3, trapPosY[i] + 3, trapPosX[i] + 3, trapPosY[i] - 3, OLED_COLOR_NORMAL);
            }
        }
    }

    if (m_pdata->deathData.isDead) {
        uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
        uint8_t radius  = (liliEnemyDeadTime - m_pdata->deathData.deathTimer) * 12 / liliEnemyDeadTime;
        radius          = etl::max(radius, uint8_t(1));
        OLED_DrawCircle(centerX, centerY, radius, OLED_COLOR_NORMAL);
    }
}

void LiliEnemy::die() {
    if (m_pdata->deathData.deathTimer > 0) {
        m_pdata->deathData.deathTimer -= controlDelayTime;
        m_pdata->deathData.deathTimer = etl::max(m_pdata->deathData.deathTimer, uint16_t(0));
        return;
    }
    m_pdata->isActive = false;
}

/**
 * @brief MODE_1: 
 * @note  
 *        
 */
void LiliEnemy::earthSurge() {
    // 每隔 EarthSurgeInterval(300ms) 发射一发火球弹
    if (action_count >= EarthSurgeInterval * (earthSurgeCount + 1)) {
        earthSurgeCount++;

        uint8_t shootX  = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

        // 获取玩家位置，调整火球弹射击偏移
        IRole *player        = g_entityManager.getPlayerRole();
        int8_t targetYOffset = 0;
        if (player != nullptr) {
            int16_t playerY = player->getData()->spatialData.currentPosY + player->getData()->spatialData.sizeY / 2;
            int16_t deltaY  = playerY - centerY;
            // 调整射击偏移，避免火球弹完全重叠
            targetYOffset = (deltaY > 5) ? 4 : ((deltaY < -5) ? -4 : 0);
            targetYOffset += (rand() % 5) - 2; // -2 到 +2 的随机偏移
        }
        m_pdata->attackData.shootCooldownTimer = 0;
        // 发射火球弹，重置冷却计时器
        shoot(shootX, centerY + targetYOffset, BulletType::FIRE_BALL);
    }
}

/**
 * @brief MODE_2: 
 * @note  
 *        
 */
void LiliEnemy::barkWave() {
    // 仅能在开始50ms后发射一发扇形子弹阵
    if (barkFired || action_count < 50) return;

    barkFired       = true;
    uint8_t shootX  = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

    // 发射5发扇形子弹，产生扩散效果
    for (int8_t i = -2; i <= 2; i++) {
        m_pdata->attackData.shootCooldownTimer = 0; // 重置冷却计时器
        shoot(shootX, centerY + i * 2, BulletType::BASIC);
    }
}

/**
 * @brief MODE_3:
 * @note  
 */
void LiliEnemy::burrowTrap() {
    // 阶段1：陷阱放置开始时间
    if (!trapPlaced && action_count >= 50) {
        trapPlaced = true;

        // 获取玩家当前位置，调整陷阱位置
        IRole *player = g_entityManager.getPlayerRole();
        if (player != nullptr) {
            uint8_t playerX = player->getData()->spatialData.currentPosX + player->getData()->spatialData.sizeX / 2;
            uint8_t playerY = player->getData()->spatialData.currentPosY + player->getData()->spatialData.sizeY / 2;

            for (uint8_t i = 0; i < TrapCount; i++) {
                // 计算陷阱位置，基于玩家当前位置，X偏移30~50，Y偏移-15~+15
                int16_t randomOffsetX = 30 + (rand() % 20);
                int16_t randomOffsetY = (rand() % 31) - 15; // -15 到 +15

                trapPosX[i] = etl::clamp<int16_t>(playerX + randomOffsetX, 10, 120);
                trapPosY[i] = etl::clamp<int16_t>(playerY + randomOffsetY, 5, 58);
            }
        } else {
            // 玩家不存在时在屏幕中间附近随机生成陷阱位置
            for (uint8_t i = 0; i < TrapCount; i++) {
                trapPosX[i] = 30 + (rand() % 60);
                trapPosY[i] = 10 + (rand() % 44);
            }
        }
    }

    // 阶段2：陷阱爆炸时间
    if (trapPlaced && !trapExploded && action_count >= TrapExplodeDelay) {
        trapExploded = true;

        // 遍历所有陷阱位置，发射火球弹
        for (uint8_t i = 0; i < TrapCount; i++) {
            m_pdata->attackData.shootCooldownTimer = 0; // 重置冷却计时器
            shoot(trapPosX[i], trapPosY[i], BulletType::FIRE_BALL);
        }
    }
}

/*******************************************************************/

/***************BOSS级大型敌人***************/
/*******************************************************************/
/**
 * @brief TaotieEnemy class
 * @note  中文：饕餮 ｜ 英文：Taotie,神话典故：四凶之一，羊身人面、眼在腋下、虎齿人爪，声音似婴儿；
 * @note  上古 "四凶" 之一，贪婪无度，能吞食天地万物，专食人类与牲畜，象征极致贪欲。
 * @note  BOSS级大型敌人，体型巨大（64x64 像素），高血量，高攻击力，低速移动，攻击方式多样且具有威胁性，擅长近战。
 * @note  攻击方式1，将玩家向自己拉近，进行吞噬攻击 
 * @note  攻击方式2，发射三排普通子弹
 * @note  攻击方式3，向前冲撞，进行撞击攻击
 * @note  攻击方式4, 向后碾压，从玩家左侧出现，进行碾压攻击
 * @note  攻击方式5, 将玩家向自己拉近，同时向前冲撞
 */

TaotieEnemy::TaotieEnemy(
    uint8_t startX, uint8_t startY, uint8_t initPosX, uint8_t initPosY, uint8_t level, uint16_t dropExp
)
: IRole() {
    // 图片信息
    m_pdata->img = &TaotieImg;

    // 身份信息
    m_pdata->identity          = RoleIdentity::ENEMY;
    m_pdata->isActive          = true;
    m_pdata->initData.isInited = false;

    // 等级信息
    m_pdata->level = level;

    // 血量信息
    m_pdata->healthData.currentHealth = 50 + level * 1700;
    m_pdata->healthData.maxHealth     = 50 + level * 1700;

    // 回血信息
    m_pdata->healthData.healValue       = 20;
    m_pdata->healthData.healTimeCounter = 0;
    m_pdata->healthData.healResetTime   = 15000;
    m_pdata->healthData.healSpeed       = 5;

    // 空间移动信息
    m_pdata->spatialData.canCrossBorder            = true;
    m_pdata->spatialData.currentPosX               = startX; // Starting X position
    m_pdata->spatialData.currentPosY               = startY; // Starting Y position
    m_pdata->spatialData.refPosX                   = startX;
    m_pdata->spatialData.refPosY                   = startY;
    m_pdata->spatialData.sizeX                     = m_pdata->img->w;
    m_pdata->spatialData.sizeY                     = m_pdata->img->h;
    m_pdata->spatialData.moveSpeed                 = 1; // 移动速度
    m_pdata->spatialData.consecutiveCollisionCount = 0;

    // 初始化位置
    m_pdata->initData.posX = initPosX;
    m_pdata->initData.posY = initPosY;

    // 攻击信息
    m_pdata->attackData.attackPower            = 2 + level * 4;
    m_pdata->attackData.shootCooldownSpeed     = 5;
    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 5000; //5000 ms
    m_pdata->attackData.bulletSpeed            = 1;

    m_pdata->attackData.bulletRange            = 10;   // 火球弹范围
    m_pdata->attackData.bulletDamageMultiplier = 1.5f; // 闪电链弹伤害倍率

    m_pdata->attackData.collisionPower = 10 + level * 10;

    // 热量信息
    m_pdata->heatData.maxHeat          = 200;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 20;
    m_pdata->heatData.heatCoolDownRate = 10; // 每次冷却 10 单位（计时单位约 200ms）

    // 状态信息
    m_pdata->deathData.deathTimer           = TaotieEnemyDeadTime;
    m_pdata->deathData.isDead               = false;
    m_pdata->deathData.dropExperiencePoints = dropExp;

    // Initialize other enemy-specific data here
}

void TaotieEnemy::init() {
    m_pdata->initData.init_count += controlDelayTime;
    // 敌人初始化逻辑

    if (m_pdata->spatialData.currentPosX > m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 60) { // 每 60 帧移动一次
            m_pdata->spatialData.currentPosX -= 1;
            m_pdata->initData.init_count = 0;
        }
    } else if (m_pdata->spatialData.currentPosX < m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 60) { // 每 60 帧移动一次
            m_pdata->spatialData.currentPosX += 1;
            m_pdata->initData.init_count = 0;
        }
    } else {
        m_pdata->initData.isInited   = true;
        m_pdata->spatialData.refPosX = m_pdata->spatialData.currentPosX;
        m_pdata->spatialData.refPosY = m_pdata->spatialData.currentPosY;
        m_pdata->initData.init_count = 0;
    }
}

void TaotieEnemy::think() {
    // 敌人 AI 逻辑实现
    think_count += controlDelayTime;
    if (think_count < 100) // 100ms思考周期
        return;

    think_count = 0;

    uint8_t randomAction = rand() % 6;
    // Random action: 0 - move left, 1 - move right, 2 - move down, 3 - move up, 4 - stay still, 5 - shoot
    if (m_pdata->actionData.currentState == ActionState::IDLE) {
        // 移动
        if (randomAction == 0) {
            m_pdata->actionData.moveMode     = MoveMode::LEFT;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 1) {
            m_pdata->actionData.moveMode     = MoveMode::RIGHT;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 2) {
            m_pdata->actionData.moveMode     = MoveMode::DOWN;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 3) {
            m_pdata->actionData.moveMode     = MoveMode::UP;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 4) {
            // Stay still
            m_pdata->actionData.moveMode     = MoveMode::NONE;
            m_pdata->actionData.currentState = ActionState::MOVING;
        }

        // 攻击
        else if (randomAction == 5) {
            if (m_pdata->attackData.shootCooldownTimer > 0) {
                // 攻击冷却中，无法进行攻击，切换回空闲状态
                m_pdata->actionData.moveMode     = MoveMode::NONE;
                m_pdata->actionData.currentState = ActionState::IDLE;
                return;
            }

            uint8_t randomAttackMode         = rand() % 6 + 1; // 1-6 攻击模式
            m_pdata->actionData.currentState = ActionState::ATTACKING;

            switch (randomAttackMode) {
            case 1:
                // 攻击方式1，将玩家向自己拉近，进行吞噬攻击
                action_timer                   = 1500; // 攻击持续时间1500ms
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                m_pdata->actionData.attackMode = AttackMode::MODE_1;
                break;
            case 2:
                // 攻击方式2，发射三排普通子弹
                action_timer                   = 1000; // 攻击持续时间1000ms
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                m_pdata->actionData.attackMode = AttackMode::MODE_2;
                break;

            case 3:
                // 攻击方式3，向前冲撞，进行撞击攻击
                // 冲进距离可达 30
                action_timer                   = 2000; // 攻击持续时间2000ms
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                m_pdata->actionData.attackMode = AttackMode::MODE_3;
                break;
            case 4:
                // 攻击方式4，向后磾压，从玩家左侧出现，进行磾压攻击
                // 出现位置距离玩家左侧 100 像素
                // action_timer                   = 4000; // 攻击持续时间4000ms
                // action_MaxTime                 = action_timer;
                // action_count                   = 0;
                // appearedForCrush               = false;
                // comeBackForCrush               = false;
                // m_pdata->actionData.attackMode = AttackMode::MODE_4;
                // 攻击方式6，将玩家拉近并发射随机中量弹幕
                action_timer                   = 2200; // 攻击持续时间2200ms
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                m_pdata->actionData.attackMode = AttackMode::MODE_6;
                break;

                break;
            case 5:
                // 攻击方式5，将玩家向自己拉近，同时向前冲撞
                action_timer                   = 3000; // 攻击持续时间3000ms
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                m_pdata->actionData.attackMode = AttackMode::MODE_5;
                break;
            case 6:
                // 攻击方式6，将玩家拉近并发射随机中量弹幕
                action_timer                   = 2200; // 攻击持续时间2200ms
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                m_pdata->actionData.attackMode = AttackMode::MODE_6;
                break;
            default:
                // 默认攻击方式1
                action_timer                   = 1500; // 攻击持续时间1500ms
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                m_pdata->actionData.attackMode = AttackMode::MODE_1;
            }
        }
    }
}

void TaotieEnemy::doAction() {
    if (m_pdata->initData.isInited == false) {
        return;
    }

    // 敌人动作逻辑实现
    if (m_pdata->deathData.isDead) {
        return;
    }

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
        // Move logic handled in think()
        break;
    case ActionState::ATTACKING:
        // 攻击动作逻辑
        action_count += controlDelayTime;

        // 更新攻击计时器
        if (action_timer >= controlDelayTime)
            action_timer -= controlDelayTime;
        else
            action_timer = 0;

        switch (m_pdata->actionData.attackMode) {
        // 根据攻击模式执行相应攻击
        case AttackMode::MODE_1:
            pullPlayerAndDevourAttack();
            break;
        case AttackMode::MODE_2:
            fireThreeRowsBasicBullets();
            break;

        case AttackMode::MODE_3:
            chargeForwardAndRamAttack();
            break;
        case AttackMode::MODE_4:
            appearLeftAndRollBackCrushAttack();
            break;
        case AttackMode::MODE_5:
            pullPlayerAndChargeForwardAttack();
            break;
        case AttackMode::MODE_6:
            pullPlayerAndScatterBullets();
            break;
        default:
            break;
        }

        if (action_timer == 0) {
            m_pdata->actionData.currentState       = ActionState::IDLE;
            m_pdata->actionData.attackMode         = AttackMode::NONE;
            action_count                           = 0;
            m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime; // 重置射击冷却计时器
        }
        break;
    }
}

void TaotieEnemy::drawRole() {
    if (m_pdata->img != nullptr && m_pdata->isActive && !m_pdata->deathData.isDead) {
        OLED_DrawImage(
            m_pdata->spatialData.currentPosX, m_pdata->spatialData.currentPosY, m_pdata->img, OLED_COLOR_NORMAL
        );
    }

    if (m_pdata->deathData.isDead) {
        // Draw death animation or effect
        // 绘制一个逐渐增大的圆环来表示死亡效果
        uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
        uint8_t radius =
            (TaotieEnemyDeadTime - m_pdata->deathData.deathTimer) * 30 / TaotieEnemyDeadTime; // 从0逐渐增大到最大值5
        radius = etl::max(radius, uint8_t(1));                                                // 最小半径限制

        OLED_DrawCircle(centerX, centerY, radius, OLED_COLOR_NORMAL);
    }
}

void TaotieEnemy::die() {
    // 敌人死亡逻辑实现
    if (m_pdata->deathData.deathTimer > 0) {
        m_pdata->deathData.deathTimer -= controlDelayTime;
        m_pdata->deathData.deathTimer = etl::max(m_pdata->deathData.deathTimer, uint16_t(0));
        return;
    }

    m_pdata->isActive = false;
}

// 攻击方式1：将玩家向自己拉近，进行吞噬攻击
void TaotieEnemy::pullPlayerAndDevourAttack() {
    // 拉近玩家并进行吞噬攻击
    if (action_count < action_MaxTime / pullDistance) // 每50ms移动一次
        return;
    action_count = 0;

    IRole *player = g_entityManager.getPlayerRole();
    if (player == nullptr) return;

    uint16_t dirX = 0;
    uint16_t dirY = 0;
    // 计算玩家相对于Taotie的方向

    int16_t deltaX = (m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2)
                     - (player->getData()->spatialData.currentPosX + player->getData()->spatialData.sizeX / 2);
    int16_t deltaY = (m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2)
                     - (player->getData()->spatialData.currentPosY + player->getData()->spatialData.sizeY / 2);

    if (deltaX < 0) dirX = -1;
    if (deltaX > 0) dirX = 1;
    if (deltaX == 0) dirX = 1;
    if (deltaY < 0) dirY = -1;
    if (deltaY > 0) dirY = 1;
    if (deltaY == 0) dirY = 0;

    // 将玩家向Taotie方向移动，实现拉近效果
    player->move(dirX, dirY, true);
}

void TaotieEnemy::fireThreeRowsBasicBullets() {
    if (action_count < 500) // 每500ms发射一次
        return;
    action_count = 0;

    // 发射三排普通子弹
    uint8_t m_x = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t m_y = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

    uint8_t m_x_1 = m_x;
    uint8_t m_y_1 = m_y - 6;
    uint8_t m_x_2 = m_x;
    uint8_t m_y_2 = m_y;
    uint8_t m_x_3 = m_x;
    uint8_t m_y_3 = m_y + 6;

    m_pdata->attackData.shootCooldownTimer = 0; // 重置射击冷却计时器，准备发射下一枚子弹
    m_pdata->heatData.currentHeat          = 0; // 重置热量信息（发射时消耗/重置）
    shoot(m_x, m_y - 6, BulletType::BASIC);
    m_pdata->attackData.shootCooldownTimer = 0; // 重置射击冷却计时器，准备发射下一枚子弹
    m_pdata->heatData.currentHeat          = 0; // 重置热量信息（发射时消耗/重置）
    shoot(m_x_2, m_y_2, BulletType::BASIC);
    m_pdata->attackData.shootCooldownTimer = 0; // 重置射击冷却计时器，准备发射下一枚子弹
    m_pdata->heatData.currentHeat          = 0; // 重置热量信息（发射时消耗/重置）
    shoot(m_x_3, m_y_3, BulletType::BASIC);

    m_x_1 = m_x + 10;
    m_x_2 = m_x + 10;
    m_x_3 = m_x + 10;

    m_pdata->attackData.shootCooldownTimer = 0; // 重置射击冷却计时器，准备发射下一枚子弹
    m_pdata->heatData.currentHeat          = 0; // 重置热量信息（发射时消耗/重置）
    shoot(m_x_1, m_y_1, BulletType::BASIC);
    m_pdata->attackData.shootCooldownTimer = 0; // 重置射击冷却计时器，准备发射下一枚子弹
    m_pdata->heatData.currentHeat          = 0; // 重置热量信息（发射时消耗/重置）
    shoot(m_x_2, m_y_2, BulletType::BASIC);
    m_pdata->attackData.shootCooldownTimer = 0; // 重置射击冷却计时器，准备发射下一枚子弹
    m_pdata->heatData.currentHeat          = 0; // 重置热量信息（发射时消耗/重置）
    shoot(m_x_3, m_y_3, BulletType::BASIC);
}

void TaotieEnemy::chargeForwardAndRamAttack() {
    // 攻击方式3：向前冲撞，进行撞击攻击
    // 先向左移动蜂备，然后向右快速冲锋
    if (action_count < action_MaxTime / chargeDistance / 4) // 每 16.67ms 移动一次
        return;
    action_count = 0;
    int8_t dir   = -1;                                                                        // 初始向左移动
    if (action_timer < action_MaxTime * 3 / 4 && action_timer >= action_MaxTime / 4) dir = 0; // 中期保持静止
    if (action_timer < action_MaxTime / 4) dir = 1;                                           // 最后向右冲锋
    move(dir, 0);
}

void TaotieEnemy::appearLeftAndRollBackCrushAttack() {
    // 攻击方式4：从左侧出现并进行磾压攻击

    // 控制移动速度
    if (action_count < action_MaxTime / crushChargeDistance / 2) // 每 40ms 移动一次
        return;
    action_count = 0;

    RoleData *taoTie = this->getData();
    if (taoTie == nullptr) return;

    if (action_timer >= action_MaxTime / 2) {
        // 第一阶段：移动到右侧屏幕外
        if (taoTie->spatialData.currentPosX < 120 && appearedForCrush == false) move(1, 0);

        // 达到右侧后传送到左侧
        if (taoTie->spatialData.currentPosX >= 120 && appearedForCrush == false) {
            appearedForCrush                = true;
            taoTie->spatialData.refPosX     = -70; // 设置到左侧屏幕外
            taoTie->spatialData.refPosY     = 1;
            taoTie->spatialData.currentPosX = -70;
            taoTie->spatialData.currentPosY = 1;
        }
        // 从左侧出现，向右移动
        if (appearedForCrush == true) {
            move(1, 0);
        }
    } else {
        // 第二阶段：回滚攻击，向左移动
        if (!comeBackForCrush) move(-1, 0);
        if (taoTie->spatialData.currentPosX <= -64) {
            // 移动到左侧屏幕外后，传送回原位置
            comeBackForCrush                = true;
            taoTie->spatialData.refPosX     = 120; // 传送到右侧屏幕外
            taoTie->spatialData.refPosY     = 1;
            taoTie->spatialData.currentPosX = 120;
            taoTie->spatialData.currentPosY = 1;
        }
        if (taoTie->spatialData.currentPosX > 64) {
            move(-1, 0);
        }
    }

    if (action_timer <= 50) {
        taoTie->spatialData.refPosX     = 64; // 最后回到初始位置
        taoTie->spatialData.refPosY     = 1;
        taoTie->spatialData.currentPosX = 64;
        taoTie->spatialData.currentPosY = 1;
    }
}

void TaotieEnemy::pullPlayerAndChargeForwardAttack() {
    // 攻击方式5：将玩家向自己拉近，同时向前冲撞
    // 同时执行拉近和冲锋攻击
    if (action_count < action_MaxTime / pullAndChargeDistance) // 每 60ms 移动一次
        return;
    action_count = 0;

    IRole *player = g_entityManager.getPlayerRole();
    if (player == nullptr) return;

    uint16_t dirX = 0;
    uint16_t dirY = 0;
    // 计算玩家相对于Taotie的方向

    int16_t deltaX = (m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2)
                     - (player->getData()->spatialData.currentPosX + player->getData()->spatialData.sizeX / 2);
    int16_t deltaY = (m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2)
                     - (player->getData()->spatialData.currentPosY + player->getData()->spatialData.sizeY / 2);

    if (deltaX < 0) dirX = -1;
    if (deltaX > 0) dirX = 1;
    if (deltaX == 0) dirX = 1;
    if (deltaY < 0) dirY = -1;
    if (deltaY > 0) dirY = 1;
    if (deltaY == 0) dirY = 0;

    // 将玩家向Taotie方向移动，实现拉近效果
    player->move(dirX, dirY, true);

    // Taotie同时进行冲锋移动
    // 先向左移动蜂备，然后向右快速冲锋
    // 每次移动距离为2像素，加快移动速度
    int8_t dir = -2;
    if (action_timer > action_MaxTime * 3 / 4) dir = -2;                                      // 前期向左移动
    if (action_timer <= action_MaxTime * 3 / 4 && action_timer > action_MaxTime / 4) dir = 0; // 中期保持静止
    if (action_timer <= action_MaxTime / 4) dir = 2;                                          // 后期向右冲锋
    move(dir, 0);
}

// 攻击方式6：拉近玩家并发射中量随机普通子弹弹幕
void TaotieEnemy::pullPlayerAndScatterBullets() {
    // 控制触发节奏：每 150ms 进行一次拉扯与弹幕发射
    if (action_count < 150) return;
    action_count = 0;

    IRole *player = g_entityManager.getPlayerRole();
    if (player == nullptr) return;

    // 计算玩家相对于Taotie的方向并进行一次拉扯
    int16_t deltaX = (m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2)
                     - (player->getData()->spatialData.currentPosX + player->getData()->spatialData.sizeX / 2);
    int16_t deltaY = (m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2)
                     - (player->getData()->spatialData.currentPosY + player->getData()->spatialData.sizeY / 2);

    int8_t dirX = 0;
    int8_t dirY = 0;
    if (deltaX < 0)
        dirX = -1;
    else if (deltaX > 0)
        dirX = 1;
    else
        dirX = 1;

    if (deltaY < 0)
        dirY = -1;
    else if (deltaY > 0)
        dirY = 1;
    else
        dirY = 0;

    player->move(dirX, dirY, true);

    // 发射中量随机散射普通子弹（偏移随机）
    uint8_t originX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t originY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

    const uint8_t bulletCount = 2;          // 中量弹幕
    const int8_t  spreadRange = 20;         // 随机偏移范围
    for (uint8_t i = 0; i < bulletCount; ++i) {
        int16_t offsetX = (rand() % (spreadRange + 1)) - spreadRange / 2; // 约 -10..+10
        int16_t offsetY = (rand() % (spreadRange + 1)) - spreadRange / 2;

        int16_t bx = originX + offsetX;
        int16_t by = originY + offsetY;

        bx = etl::clamp<int16_t>(bx, 0, 159);
        by = etl::clamp<int16_t>(by, 0, 63);

        m_pdata->attackData.shootCooldownTimer = 0;
        m_pdata->heatData.currentHeat          = 0;
        shoot(static_cast<uint8_t>(bx), static_cast<uint8_t>(by), BulletType::BASIC);
    }
}

/*******************************************************************/

/*******************************************************************/
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

TaowuEnemy::TaowuEnemy(
    uint8_t startX, uint8_t startY, uint8_t initPosX, uint8_t initPosY, uint8_t level, uint16_t dropExp
)
: IRole() {
    // 图片信息
    m_pdata->img = &TaowuImg;

    // 身份信息
    m_pdata->identity          = RoleIdentity::ENEMY;
    m_pdata->isActive          = true;
    m_pdata->initData.isInited = false;

    // 等级信息
    m_pdata->level = level;

    // 血量信息
    //（具体数值按敌人类型设计）
    m_pdata->healthData.currentHealth = 30 + level * 1200;
    m_pdata->healthData.maxHealth     = 30 + level * 1200;

    // 治疗信息
    m_pdata->healthData.healValue       = 250;
    m_pdata->healthData.healTimeCounter = 0;
    m_pdata->healthData.healResetTime   = 15000;
    m_pdata->healthData.healSpeed       = 5;

    // 空间信息
    m_pdata->spatialData.canCrossBorder            = true;
    m_pdata->spatialData.currentPosX               = startX; // Starting X position
    m_pdata->spatialData.currentPosY               = startY; // Starting Y position
    m_pdata->spatialData.refPosX                   = startX;
    m_pdata->spatialData.refPosY                   = startY;
    m_pdata->spatialData.sizeX                     = m_pdata->img->w;
    m_pdata->spatialData.sizeY                     = m_pdata->img->h;
    m_pdata->spatialData.moveSpeed                 = 3; // Set movement speed
    m_pdata->spatialData.consecutiveCollisionCount = 0;

    // 初始化位置
    m_pdata->initData.posX = initPosX;
    m_pdata->initData.posY = initPosY;

    // 攻击信息
    m_pdata->attackData.attackPower            = 10 + level * 6;
    m_pdata->attackData.shootCooldownSpeed     = 5;
    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 5000; //5000 ms
    m_pdata->attackData.bulletSpeed            = 1;

    m_pdata->attackData.bulletRange            = 10;   // 只对火球弹有效
    m_pdata->attackData.bulletDamageMultiplier = 1.5f; // 只对闪电链弹有效

    m_pdata->attackData.collisionPower = 7 + level * 5;

    // 热量信息
    m_pdata->heatData.maxHeat          = 250;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 0;
    m_pdata->heatData.heatCoolDownRate = 10; // 每次冷却 10 单位（计时单位约 200ms）

    // 死亡状态信息
    m_pdata->deathData.deathTimer           = TaowuEnemyDeadTime;
    m_pdata->deathData.isDead               = false;
    m_pdata->deathData.dropExperiencePoints = dropExp;

    // Initialize other enemy-specific data here
}

void TaowuEnemy::init() {
    m_pdata->initData.init_count += controlDelayTime;
    // Initialize enemy role specifics

    if (m_pdata->spatialData.currentPosX > m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 60) { // 每60ms移动一次
            m_pdata->spatialData.currentPosX -= 1;
            m_pdata->initData.init_count = 0;
        }
    } else if (m_pdata->spatialData.currentPosX < m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 60) { // 每60ms移动一次
            m_pdata->spatialData.currentPosX += 1;
            m_pdata->initData.init_count = 0;
        }
    } else {
        m_pdata->initData.isInited   = true;
        m_pdata->spatialData.refPosX = m_pdata->spatialData.currentPosX;
        m_pdata->spatialData.refPosY = m_pdata->spatialData.currentPosY;
        m_pdata->initData.init_count = 0;
    }
}

void TaowuEnemy::think() {
    // Implement enemy AI logic
    think_count += controlDelayTime;
    if (think_count < 100) // 每100ms进行一次判断
        return;

    think_count = 0;

    uint8_t randomAction = rand() % 6;
    // Random action: 0 - move left, 1 - move right, 2 - move down, 3 - move up, 4 - stay still, 5 - shoot
    if (m_pdata->actionData.currentState == ActionState::IDLE) {
        // 移动
        if (randomAction == 0) {
            m_pdata->actionData.moveMode     = MoveMode::LEFT;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 1) {
            m_pdata->actionData.moveMode     = MoveMode::RIGHT;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 2) {
            m_pdata->actionData.moveMode     = MoveMode::DOWN;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 3) {
            m_pdata->actionData.moveMode     = MoveMode::UP;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 4) {
            // Stay still
            m_pdata->actionData.moveMode     = MoveMode::NONE;
            m_pdata->actionData.currentState = ActionState::MOVING;
        }

        // 射击
        else if (randomAction == 5) {
            if (m_pdata->attackData.shootCooldownTimer > 0) {
                // 冷却中，无法射击，切换回空闲状态
                m_pdata->actionData.moveMode     = MoveMode::NONE;
                m_pdata->actionData.currentState = ActionState::IDLE;
                return;
            }

            uint8_t randomAttackMode         = rand() % 6 + 1; // 1-6  攻击模式
            m_pdata->actionData.currentState = ActionState::ATTACKING;

            switch (randomAttackMode) {
            case 1:
                // 攻击模式1 - 大量基本子弹攻击，攻击时间根据当前生命值动态调整，最短为3秒，最长为6秒
                action_timer   = (uint16_t)(MassiveBasicBulletFireTime
                                          * float(
                                              (float)(m_pdata->healthData.maxHealth - m_pdata->healthData.currentHealth)
                                                  / (float)m_pdata->healthData.maxHealth
                                              + 1
                                          )); // 攻击持续时间，血量越低时间越长
                action_MaxTime = action_timer;
                action_count   = 0;

                positionChange = false;

                m_pdata->actionData.attackMode = AttackMode::MODE_1;
                break;
            case 2:
                // 攻击模式2 - 五个火球子弹攻击，攻击时间3秒
                action_timer   = FiveFireballBulletFireTime; // 攻击持续时间3000ms
                action_MaxTime = action_timer;
                action_count   = 0;
                positionChange = false;

                m_pdata->actionData.attackMode = AttackMode::MODE_2;
                break;

            case 3:
                //攻击模式3 - 中间火球两侧基本子弹攻击，攻击时间
                //1000ms
                action_timer   = CenterFireballAttackTime; // 攻击持续时间100ms
                action_MaxTime = action_timer;
                action_count   = 0;

                m_pdata->actionData.attackMode = AttackMode::MODE_3;
                break;
            case 4:
                //攻击模式4 - 单排缺口基本子弹攻击，攻击时间
                action_timer   = NotchedBulletsAttackTime; // 攻击持续时间500ms
                action_MaxTime = action_timer;
                action_count   = 0;

                m_pdata->actionData.attackMode = AttackMode::MODE_4;
                break;
            case 5:
                //攻击模式5 - 随机闪烁移动，攻击时间
                action_timer   = BlinkRandomTime; // 攻击持续时间1000ms
                action_MaxTime = action_timer;
                action_count   = 0;
                positionChange = false;

                m_pdata->actionData.attackMode = AttackMode::MODE_5;
                break;
            case 6:
                //攻击模式6 - 闪烁对齐位置，攻击时间
                action_timer   = BlinkAlignedTime; // 攻击持续时间100ms
                action_MaxTime = action_timer;
                action_count   = 0;
                positionChange = false;

                m_pdata->actionData.attackMode = AttackMode::MODE_6;
                break;
            default:
                break;
            }
        }
    }
}

void TaowuEnemy::doAction() {
    if (m_pdata->initData.isInited == false) {
        return;
    }

    // Implement enemy action logic
    if (m_pdata->deathData.isDead) {
        return;
    }
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
        // Move logic handled in think()
        break;
    case ActionState::ATTACKING:
        // 攻击计时器，控制攻击频率
        action_count += controlDelayTime;

        // 攻击持续时间计时
        if (action_timer >= controlDelayTime)
            action_timer -= controlDelayTime;
        else
            action_timer = 0;

        switch (m_pdata->actionData.attackMode) {
        //执行攻击模式
        case AttackMode::MODE_1:
            fireContinuousMassiveBasicBullets();
            break;
        case AttackMode::MODE_2:
            fireFiveFireballBulletsAtRandom();
            break;
        case AttackMode::MODE_3:
            fireCenterFireballAndSideBasicBullets();
            break;
        case AttackMode::MODE_4:
            fireSingleRowNotchedBasicBullets();
            break;
        case AttackMode::MODE_5:
            blinkToRandomPosition();
            break;
        case AttackMode::MODE_6:
            blinkToPlayerAlignedPosition();
            break;
        default:
            break;
        }

        if (action_timer == 0) {
            // 攻击模式6结束后清除CD，其他模式恢复攻击前的CD
            bool clearCD = (m_pdata->actionData.attackMode == AttackMode::MODE_6);

            m_pdata->actionData.currentState = ActionState::IDLE;
            m_pdata->actionData.attackMode   = AttackMode::NONE;
            action_count                     = 0;

            if (clearCD) {
                m_pdata->attackData.shootCooldownTimer = 0; // MODE_6 攻击等待时间
            } else {
                m_pdata->attackData.shootCooldownTimer =
                    m_pdata->attackData.shootCooldownResetTime; // 其他模式恢复等待时间
            }
        }
        break;
    }
}

void TaowuEnemy::drawRole() {
    if (m_pdata->img != nullptr && m_pdata->isActive && !m_pdata->deathData.isDead) {
        OLED_DrawImage(
            m_pdata->spatialData.currentPosX, m_pdata->spatialData.currentPosY, m_pdata->img, OLED_COLOR_NORMAL
        );
    }

    if (m_pdata->deathData.isDead) {
        // Draw death animation or effect
        //  绘制一个逐渐增大的圆环来表示死亡效果
        uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
        uint8_t radius =
            (TaowuEnemyDeadTime - m_pdata->deathData.deathTimer) * 30 / TaowuEnemyDeadTime; // 从0逐渐增大到最大值30
        radius = etl::max(radius, uint8_t(1));                                              // 最小半径为1

        OLED_DrawCircle(centerX, centerY, radius, OLED_COLOR_NORMAL);
    }
}

void TaowuEnemy::die() {
    // Implement enemy death logic
    if (m_pdata->deathData.deathTimer > 0) {
        m_pdata->deathData.deathTimer -= controlDelayTime;
        m_pdata->deathData.deathTimer = etl::max(m_pdata->deathData.deathTimer, uint16_t(0));
        return;
    }

    m_pdata->isActive = false;
}

// 攻击方式1：连续大量发射基本子弹
void TaowuEnemy::fireContinuousMassiveBasicBullets() {
    if (action_count < 1000 / BulletsPerSecond) // 每125ms发一次 (1000/8)
        return;
    action_count = 0;

    if (!positionChange) {
        //位置改变
        m_pdata->spatialData.currentPosX = 63;
        m_pdata->spatialData.currentPosY = 1;
        m_pdata->spatialData.refPosX     = 63;
        m_pdata->spatialData.refPosY     = 1;
        positionChange                   = true;
    }

    // 位置改变后发射普通子弹
    uint8_t m_x = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t m_y = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

    uint8_t offsetY = (rand() % 61) - 30; // -30 到 +30 的随机偏移

    // BOSS攻击模式1发射子弹，重置射击冷却时间
    m_pdata->attackData.shootCooldownTimer = 0; // 重置冷却时间，允许连续射击
    shoot(m_x, m_y + offsetY, BulletType::BASIC);
}

// 每375ms发一次 (3000/8)
void TaowuEnemy::fireFiveFireballBulletsAtRandom() {
    if (action_count < FiveFireballBulletFireTime / FireballCount) // 每375ms发一次 (3000/8)
        return;
    action_count = 0;

    if (!positionChange) {
        //位置改变
        m_pdata->spatialData.currentPosX = 63;
        m_pdata->spatialData.currentPosY = 1;
        m_pdata->spatialData.refPosX     = 63;
        m_pdata->spatialData.refPosY     = 1;
        positionChange                   = true;
    }

    // 位置改变后发射火球子弹
    uint8_t m_x = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t m_y = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

    uint8_t offsetY = (rand() % 61) - 30; // -30 到 +30 的随机偏移

    m_pdata->attackData.shootCooldownTimer = 0; // 重置冷却时间，允许连续射击
    shoot(m_x, m_y + offsetY, BulletType::FIRE_BALL);
}

void TaowuEnemy::fireCenterFireballAndSideBasicBullets() {
    if (action_count < action_MaxTime - 10) // action_MaxTime=100ms，90ms执行一次
        return;
    action_count = 0;
    // 位置改变后发射火球和普通子弹
    uint8_t m_x = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t m_y = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
    // 中心火球
    m_pdata->attackData.shootCooldownTimer = 0; // 重置冷却时间，允许连续射击
    shoot(m_x, m_y, BulletType::FIRE_BALL);
    // 位置改变后发射普通子弹
    m_pdata->attackData.shootCooldownTimer = 0; // 重置冷却时间，允许连续射击
    shoot(m_x, m_y + 26, BulletType::BASIC);
    m_pdata->attackData.shootCooldownTimer = 0; // 重置冷却时间，允许连续射击
    shoot(m_x + 20, m_y + 30, BulletType::BASIC);
    // 位置改变后发射普通子弹
    m_pdata->attackData.shootCooldownTimer = 0; // 重置冷却时间，允许连续射击
    shoot(m_x, m_y - 26, BulletType::BASIC);
    m_pdata->attackData.shootCooldownTimer = 0; // 重置冷却时间，允许连续射击
    shoot(m_x + 20, m_y - 30, BulletType::BASIC);
}

void TaowuEnemy::fireSingleRowNotchedBasicBullets() {
    if (action_count < action_MaxTime - 10) // action_MaxTime=500ms，490ms执行一次
        return;
    action_count = 0;
    // 位置改变后发射普通子弹
    uint8_t m_x = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t m_y = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

    // 位置改变后发射普通子弹，间隔6像素，跳过中间位置
    for (int8_t offsetY = -30; offsetY <= 30; offsetY += 6) {
        if (offsetY >= -12 && offsetY <= 12) {
            // 中间位置跳过，避免重叠
            continue;
        }
        m_pdata->attackData.shootCooldownTimer = 0; // 重置冷却时间，允许连续射击
        shoot(m_x, m_y + offsetY, BulletType::BASIC);
    }
}

void TaowuEnemy::blinkToRandomPosition() {
    // 使用 action_MaxTime/3 作为阶段时间 (BlinkRandomTime=1000ms, 每阶段333.3ms)
    uint16_t phaseInterval = action_MaxTime / 3 - 20; // 当前20ms执行阶段切换
    if (action_count < phaseInterval) return;
    action_count = 0;

    // 使用 action_timer 判断当前阶段
    // action_timer 和 action_MaxTime 之间的关系
    // 阶段1: action_timer > action_MaxTime * 2/3  (开始)
    // 阶段2: action_timer 在 action_MaxTime * 1/3 到 2/3 之间
    // 阶段3: action_timer < action_MaxTime * 1/3  (结束)

    uint16_t phase2Threshold = action_MaxTime * 2 / 3; // 大约666ms
    uint16_t phase3Threshold = action_MaxTime / 3;     // 大约333ms

    if (!positionChange) {
        // 阶段1: 设定随机位置
        m_pdata->spatialData.currentPosX = 140;
        m_pdata->spatialData.currentPosY = 1;
        m_pdata->spatialData.refPosX     = m_pdata->spatialData.currentPosX;
        m_pdata->spatialData.refPosY     = m_pdata->spatialData.currentPosY;
        positionChange                   = true;
    } else if (action_timer >= phase3Threshold) {
        // 阶段2: 随机位置发射3发火球，随机Y位置
        // 注意：每次射击前都要重置冷却时间，因为shoot()内部会等待冷却时间

        for (int i = 0; i < 3; i++) {
            m_pdata->attackData.shootCooldownTimer = 0;                // 重置每次shoot前的冷却时间
            uint8_t m_y                            = rand() % 54 + 6;  // 6-59 随机Y位置
            uint8_t m_x                            = 80 + rand() % 42; // 80-121 随机X位置
            shoot(m_x, m_y, BulletType::FIRE_BALL);
        }
    } else {
        // 阶段3: 归位原位置
        m_pdata->spatialData.currentPosX = 63;
        m_pdata->spatialData.currentPosY = 1;
        m_pdata->spatialData.refPosX     = 63;
        m_pdata->spatialData.refPosY     = 1;
    }
}

void TaowuEnemy::blinkToPlayerAlignedPosition() {
    if (action_count < action_MaxTime - 10) // 每隔490ms执行一次
        return;
    action_count = 0;

    IRole *player = g_entityManager.getPlayerRole();
    if (player == nullptr) return;

    if (!positionChange) {
        // 位置改变，重新定位
        uint8_t playerY = player->getData()->spatialData.currentPosY + player->getData()->spatialData.sizeY / 2;
        int8_t  targetY = playerY - m_pdata->spatialData.sizeY / 2;

        //X位置随机
        m_pdata->spatialData.currentPosX = 30 + (rand() % 51); // 30-80 随机位置

        // 确保BOSS位置不会超出屏幕边界
        if (targetY < -31) targetY = -31;
        if (targetY > 95) targetY = 95;

        m_pdata->spatialData.currentPosY = targetY;
        m_pdata->spatialData.refPosX     = m_pdata->spatialData.currentPosX;
        m_pdata->spatialData.refPosY     = m_pdata->spatialData.currentPosY;
        positionChange                   = true;
    }
}

/*******************************************************************/

/*******************************************************************/
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

XiangliuEnemy::XiangliuEnemy(
    uint8_t startX, uint8_t startY, uint8_t initPosX, uint8_t initPosY, uint8_t level, uint16_t dropExp
)
: IRole() {
    // 图片信息
    m_pdata->img = &XiangliuImg;

    // 身份信息
    m_pdata->identity          = RoleIdentity::ENEMY;
    m_pdata->isActive          = true;
    m_pdata->initData.isInited = false;

    // 等级信息
    m_pdata->level = level;

    // 血量信息
    // 血量信息，随着等级增加，血量增加
    m_pdata->healthData.currentHealth = 30 + level * 1300;
    m_pdata->healthData.maxHealth     = 30 + level * 1300;

    //回血信息
    m_pdata->healthData.healValue       = 200;
    m_pdata->healthData.healTimeCounter = 0;
    m_pdata->healthData.healResetTime   = 15000;
    m_pdata->healthData.healSpeed       = 5;

    // 空间移动信息
    m_pdata->spatialData.canCrossBorder            = true;
    m_pdata->spatialData.currentPosX               = startX; // Starting X position
    m_pdata->spatialData.currentPosY               = startY; // Starting Y position
    m_pdata->spatialData.refPosX                   = startX;
    m_pdata->spatialData.refPosY                   = startY;
    m_pdata->spatialData.sizeX                     = m_pdata->img->w;
    m_pdata->spatialData.sizeY                     = m_pdata->img->h;
    m_pdata->spatialData.moveSpeed                 = 2; // Set movement speed
    m_pdata->spatialData.consecutiveCollisionCount = 0;

    // 初始化位置
    m_pdata->initData.posX = initPosX;
    m_pdata->initData.posY = initPosY;

    // 攻击信息
    m_pdata->attackData.attackPower            = 3 + level * 5;
    m_pdata->attackData.shootCooldownSpeed     = 5;
    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 8000; //8000 ms
    m_pdata->attackData.bulletSpeed            = 1;
    //子弹速度 15000 ms

    m_pdata->attackData.bulletRange            = 10;   //火球子弹有效
    m_pdata->attackData.bulletDamageMultiplier = 1.5f; //雷电子弹有效

    m_pdata->attackData.collisionPower = 12 + level * 4;

    // 热量信息
    m_pdata->heatData.maxHeat          = 250;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 0;
    m_pdata->heatData.heatCoolDownRate = 10; // 每次冷却 10 单位（计时单位约 200ms）

    // 死亡状态信息
    m_pdata->deathData.deathTimer           = XiangliuEnemyDeadTime;
    m_pdata->deathData.isDead               = false;
    m_pdata->deathData.dropExperiencePoints = dropExp;

    // Initialize other enemy-specific data here
}

void XiangliuEnemy::init() {
    m_pdata->initData.init_count += controlDelayTime;
    // Initialize enemy role specifics

    if (m_pdata->spatialData.currentPosX > m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 60) { // 每60ms移动一次
            m_pdata->spatialData.currentPosX -= 1;
            m_pdata->initData.init_count = 0;
        }
    } else if (m_pdata->spatialData.currentPosX < m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 60) { // 每60ms移动一次
            m_pdata->spatialData.currentPosX += 1;
            m_pdata->initData.init_count = 0;
        }
    } else {
        m_pdata->initData.isInited   = true;
        m_pdata->spatialData.refPosX = m_pdata->spatialData.currentPosX;
        m_pdata->spatialData.refPosY = m_pdata->spatialData.currentPosY;
        m_pdata->initData.init_count = 0;
    }
}

void XiangliuEnemy::think() {
    // Implement enemy AI logic
    think_count += controlDelayTime;
    if (think_count < 100) // 每100ms进行一次判断
        return;

    think_count = 0;

    uint8_t randomAction = rand() % 6;
    // Random action: 0 - move left, 1 - move right, 2 - move down, 3 - move up, 4 - stay still, 5 - shoot
    if (m_pdata->actionData.currentState == ActionState::IDLE) {
        //移动
        if (randomAction == 0) {
            m_pdata->actionData.moveMode     = MoveMode::LEFT;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 1) {
            m_pdata->actionData.moveMode     = MoveMode::RIGHT;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 2) {
            m_pdata->actionData.moveMode     = MoveMode::DOWN;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 3) {
            m_pdata->actionData.moveMode     = MoveMode::UP;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 4) {
            // Stay still
            m_pdata->actionData.moveMode     = MoveMode::NONE;
            m_pdata->actionData.currentState = ActionState::MOVING;
        }

        //攻击
        else if (randomAction == 5) {
            if (m_pdata->attackData.shootCooldownTimer > 0) {
                // 冷却中无法射击，恢复到空闲状态
                m_pdata->actionData.moveMode     = MoveMode::NONE;
                m_pdata->actionData.currentState = ActionState::IDLE;
                return;
            }

            uint8_t randomAttackMode = rand() % 6 + 1; // 1-6 攻击模式
            if (randomAttackMode > 3 && g_entityManager.m_roles.size() >= 4)
                randomAttackMode -= 3; // 如果敌人数量较多，减少高攻击模式的概率

            m_pdata->actionData.currentState = ActionState::ATTACKING;

            switch (randomAttackMode) {
            case 1:
                action_count   = 0;
                action_timer   = 300; // 攻击持续时间300ms
                action_MaxTime = action_timer;

                m_pdata->actionData.attackMode = AttackMode::MODE_1;
                break;
            case 2:
                action_count   = 0;
                action_timer   = 300; // 攻击持续时间300ms
                action_MaxTime = action_timer;

                m_pdata->actionData.attackMode = AttackMode::MODE_2;
                break;

            case 3:
                action_count   = 0;
                action_timer   = 300; // 攻击持续时间300ms
                action_MaxTime = action_timer;

                m_pdata->actionData.attackMode = AttackMode::MODE_3;
                break;
            case 4:
                action_count   = 0;
                action_timer   = 300; // 攻击持续时间300ms
                action_MaxTime = action_timer;

                m_pdata->actionData.attackMode = AttackMode::MODE_4;
                break;
            case 5:
                action_count   = 0;
                action_timer   = 300; // 攻击持续时间300ms
                action_MaxTime = action_timer;

                m_pdata->actionData.attackMode = AttackMode::MODE_5;
                break;
            case 6:
                action_count   = 0;
                action_timer   = 300; // 攻击持续时间300ms
                action_MaxTime = action_timer;

                m_pdata->actionData.attackMode = AttackMode::MODE_6;
                break;
            default:
                break;
            }
        }
    }
}

void XiangliuEnemy::doAction() {
    if (m_pdata->initData.isInited == false) {
        return;
    }

    // Implement enemy action logic
    if (m_pdata->deathData.isDead) {
        return;
    }
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
        // Move logic handled in think()
        break;
    case ActionState::ATTACKING:
        // 攻击持续时间，控制攻击频率
        action_count += controlDelayTime;

        // 攻击计时器
        if (action_timer >= controlDelayTime)
            action_timer -= controlDelayTime;
        else
            action_timer = 0;

        switch (m_pdata->actionData.attackMode) {
        //执行攻击逻辑
        case AttackMode::MODE_1:
            fireNineRowsBasicBullets();
            break;
        case AttackMode::MODE_2:
            fireThreeRowsLightningBullets();
            break;
        case AttackMode::MODE_3:
            fireThreeRowsFireballBullets();
            break;
        case AttackMode::MODE_4:
            summonThreeChiMeiMinions();
            break;
        case AttackMode::MODE_5:
            summonTwoFeilianMinions();
            break;
        case AttackMode::MODE_6:
            summonOneGudiaoMinion();
            break;
        default:
            break;
        }

        if (action_timer == 0) {
            m_pdata->actionData.currentState       = ActionState::IDLE;
            m_pdata->actionData.attackMode         = AttackMode::NONE;
            action_count                           = 0;
            m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime; // 攻击冷却等待时间
        }
        break;
    }
}

void XiangliuEnemy::drawRole() {
    if (m_pdata->img != nullptr && m_pdata->isActive && !m_pdata->deathData.isDead) {
        OLED_DrawImage(
            m_pdata->spatialData.currentPosX, m_pdata->spatialData.currentPosY, m_pdata->img, OLED_COLOR_NORMAL
        );
    }

    if (m_pdata->deathData.isDead) {
        // Draw death animation or effect
        // 敌人死亡时绘制一个逐渐扩大的圆环表示死亡效果
        uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
        uint8_t radius  = (XiangliuEnemyDeadTime - m_pdata->deathData.deathTimer) * 30
                         / XiangliuEnemyDeadTime; // 从0逐渐增大到最大值5
        radius = etl::max(radius, uint8_t(1));    // 最小半径为1

        OLED_DrawCircle(centerX, centerY, radius, OLED_COLOR_NORMAL);
    }
}

void XiangliuEnemy::die() {
    // Implement enemy death logic
    if (m_pdata->deathData.deathTimer > 0) {
        m_pdata->deathData.deathTimer -= controlDelayTime;
        m_pdata->deathData.deathTimer = etl::max(m_pdata->deathData.deathTimer, uint16_t(0));
        return;
    }

    m_pdata->isActive = false;
}

//攻击逻辑
void XiangliuEnemy::fireNineRowsBasicBullets() {
    //普通子弹
    if (action_count < action_MaxTime - 10) // 等待一段时间
        return;
    action_count = 0;

    uint8_t m_x = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t m_y = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
    //普通子弹，9排，纵向分布，中间两排不移动
    int8_t offsetYList[9] = {-31, -29, -27, -2, 0, 2, 27, 29, 31};
    for (uint8_t i = 0; i < 9; i++) {
        m_pdata->attackData.shootCooldownTimer = 0; // 攻击冷却等待时间，确保可以连续发射子弹
        shoot(m_x, m_y + offsetYList[i], BulletType::BASIC);
        m_pdata->attackData.shootCooldownTimer = 0; // 攻击冷却等待时间，确保可以连续发射子弹
        shoot(m_x + 18, m_y + offsetYList[i], BulletType::BASIC);
    }
}

void XiangliuEnemy::fireThreeRowsLightningBullets() {
    if (action_count < action_MaxTime - 10) // 等待一段时间
        return;
    action_count = 0;

    //设置位置
    m_pdata->spatialData.currentPosX = 64 + 14;
    m_pdata->spatialData.refPosX     = 64 + 14;
    m_pdata->spatialData.currentPosY = 1;
    m_pdata->spatialData.refPosY     = 1;

    //设置位置

    uint8_t m_y = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
    //普通子弹，3排，纵向分布
    int8_t offsetYList[3] = {-27, 0, 27};
    for (uint8_t i = 0; i < 3; i++) {
        m_pdata->attackData.shootCooldownTimer = 0; // 攻击冷却等待时间，确保可以连续发射子弹
        shoot(1, m_y + offsetYList[i], BulletType::LIGHTNING_LINE);
    }
}

void XiangliuEnemy::fireThreeRowsFireballBullets() {
    if (action_count < action_MaxTime - 10) // 等待一段时间
        return;
    action_count = 0;
    //设置位置
    uint8_t m_x = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t m_y = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
    //子弹，3排，纵向分布
    int8_t offsetYList[3] = {-21, 0, 21};
    for (uint8_t i = 0; i < 3; i++) {
        m_pdata->attackData.shootCooldownTimer = 0; // 攻击冷却等待时间，确保可以连续发射子弹
        shoot(m_x, m_y + offsetYList[i], BulletType::FIRE_BALL);
    }
}

void XiangliuEnemy::summonThreeChiMeiMinions() {
    if (action_count < action_MaxTime - 10) // 等待一段时间
        return;
    action_count = 0;

    uint8_t posX[3] = {20, 40, 60};
    uint8_t posY[3] = {1, 32, 50};

    for (uint8_t i = 0; i < 3; i++) {
        IRole *minion = new ChiMeiEnemy(posX[i], posY[i], posX[i], posY[i], rand() % 3 + 1);
        if (minion != nullptr) {
            if (!g_entityManager.addRole(minion)) delete minion; // Clean up if not added
        }
    }
}

void XiangliuEnemy::summonTwoFeilianMinions() {
    if (action_count < action_MaxTime - 10) // 等待一段时间
        return;
    action_count = 0;

    uint8_t posX[2] = {50, 50};
    uint8_t posY[2] = {0, 50};
    for (uint8_t i = 0; i < 2; i++) {
        IRole *minion = new FeilianEnemy(posX[i], posY[i], posX[i], posY[i], rand() % 3 + 1);
        if (minion != nullptr) {
            if (!g_entityManager.addRole(minion)) delete minion; // Clean up if not added
        }
    }
}

void XiangliuEnemy::summonOneGudiaoMinion() {
    if (action_count < action_MaxTime - 10) // 等待一段时间
        return;
    action_count = 0;

    uint8_t posX = 50;
    uint8_t posY = 25;

    IRole *minion = new GudiaoEnemy(posX, posY, posX, posY, rand() % 3 + 1);
    if (minion != nullptr) {
        if (!g_entityManager.addRole(minion)) delete minion; // Clean up if not added
    }
}

/*******************************************************************/

/*******************************************************************/
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

// 混沌 BOSS 构造函数

HundunEnemy::HundunEnemy(
    uint8_t startX, uint8_t startY, uint8_t initPosX, uint8_t initPosY, uint8_t level, uint16_t dropExp
)
: IRole() {
    // 图片信息
    m_pdata->img = &HundunImg;

    // 身份信息
    m_pdata->identity          = RoleIdentity::ENEMY;
    m_pdata->isActive          = true;
    m_pdata->initData.isInited = false;

    // 等级信息
    m_pdata->level = level;

    // 血量信息
    // 设置血量相关信息，随着等级增加，血量增加
    m_pdata->healthData.currentHealth = 40 + level * 1400;
    m_pdata->healthData.maxHealth     = 40 + level * 1400;

    // 治疗信息
    m_pdata->healthData.healValue       = 250; // 每次的治疗值
    m_pdata->healthData.healTimeCounter = 0;
    m_pdata->healthData.healResetTime   = 12000; // 治疗间隔
    m_pdata->healthData.healSpeed       = 5;

    // 空间移动信息
    m_pdata->spatialData.canCrossBorder            = true;
    m_pdata->spatialData.currentPosX               = startX;
    m_pdata->spatialData.currentPosY               = startY;
    m_pdata->spatialData.refPosX                   = startX;
    m_pdata->spatialData.refPosY                   = startY;
    m_pdata->spatialData.sizeX                     = m_pdata->img->w; // 68
    m_pdata->spatialData.sizeY                     = m_pdata->img->h; // 64
    m_pdata->spatialData.moveSpeed                 = 1;               // 移动速度
    m_pdata->spatialData.consecutiveCollisionCount = 0;

    // 初始化位置
    m_pdata->initData.posX = initPosX;
    m_pdata->initData.posY = initPosY;

    // 攻击信息
    m_pdata->attackData.attackPower            = 8 + level * 6; // 中等伤害
    m_pdata->attackData.shootCooldownSpeed     = 5;
    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 6000; // 6000ms 射击冷却时间
    m_pdata->attackData.bulletSpeed            = 1;

    m_pdata->attackData.bulletRange            = 12;             // 子弹射击范围
    m_pdata->attackData.bulletDamageMultiplier = 1.8f;           // 子弹伤害倍率
    m_pdata->attackData.collisionPower         = 15 + level * 8; // 碰撞威力

    // 热量信息（BOSS 特殊值）
    m_pdata->heatData.maxHeat          = 250;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 0; // BOSS 不消耗热量
    m_pdata->heatData.heatCoolDownRate = 10;

    // 状态信息
    m_pdata->deathData.deathTimer           = HundunEnemyDeadTime;
    m_pdata->deathData.isDead               = false;
    m_pdata->deathData.dropExperiencePoints = dropExp;

    // Initialize other HundunEnemy-specific data here
    positionChange     = false;
    aperturesGenerated = false;
    warningDisplayed   = false;
    spiralPhase        = 0;
    riftWaveCount      = 0;
}

void HundunEnemy::init() {
    m_pdata->initData.init_count += controlDelayTime;

    // 设置屏幕边缘移动的初始位置
    if (m_pdata->spatialData.currentPosX > m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 60) { // 每60ms移动一次
            m_pdata->spatialData.currentPosX -= 1;
            m_pdata->initData.init_count = 0;
        }
    } else if (m_pdata->spatialData.currentPosX < m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 60) {
            m_pdata->spatialData.currentPosX += 1;
            m_pdata->initData.init_count = 0;
        }
    } else {
        m_pdata->initData.isInited   = true;
        m_pdata->spatialData.refPosX = m_pdata->spatialData.currentPosX;
        m_pdata->spatialData.refPosY = m_pdata->spatialData.currentPosY;
        m_pdata->initData.init_count = 0;
    }
}

void HundunEnemy::think() {
    think_count += controlDelayTime;
    if (think_count < 100) // 每100ms进行一次判断
        return;

    think_count = 0;

    uint8_t randomAction = rand() % 6;
    // 随机判断: 0-3 移动, 4 停止, 5 攻击

    if (m_pdata->actionData.currentState == ActionState::IDLE) {
        // 移动动作
        if (randomAction == 0) {
            m_pdata->actionData.moveMode     = MoveMode::LEFT;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 1) {
            m_pdata->actionData.moveMode     = MoveMode::RIGHT;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 2) {
            m_pdata->actionData.moveMode     = MoveMode::DOWN;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 3) {
            m_pdata->actionData.moveMode     = MoveMode::UP;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 4) {
            m_pdata->actionData.moveMode     = MoveMode::NONE;
            m_pdata->actionData.currentState = ActionState::MOVING;
        }

        // 攻击动作
        else if (randomAction == 5) {
            if (m_pdata->attackData.shootCooldownTimer > 0) {
                // 冷却中，无法攻击，恢复到空闲状态
                m_pdata->actionData.moveMode     = MoveMode::NONE;
                m_pdata->actionData.currentState = ActionState::IDLE;
                return;
            }

            uint8_t randomAttackMode         = rand() % 6 + 1; // 1-6 攻击模式
            m_pdata->actionData.currentState = ActionState::ATTACKING;

            switch (randomAttackMode) {
            case 1:
                // MODE_1: 混沌涌动 - 屏幕中心闪电环绕8秒钟发射子弹
                action_timer   = ChaosSurgeTime; // 3000ms
                action_MaxTime = action_timer;
                action_count   = 0;
                positionChange = false;

                m_pdata->actionData.attackMode = AttackMode::MODE_1;
                break;

            case 2:
                // MODE_2: 封印七窍 - 封印7个七窍
                action_timer       = SealAperturesTime; // 2000ms
                action_MaxTime     = action_timer;
                action_count       = 0;
                aperturesGenerated = false;

                m_pdata->actionData.attackMode = AttackMode::MODE_2;
                break;

            case 3:
                // MODE_3: 虚空牵引 - 将玩家向混沌位置缓慢拉近
                action_timer   = VoidPullTime; // 2500ms
                action_MaxTime = action_timer;
                action_count   = 0;

                m_pdata->actionData.attackMode = AttackMode::MODE_3;
                break;

            case 4:
                // MODE_4: 混沌漩涡 - 螺旋式发射子弹
                action_timer   = ChaoticBarrageTime; // 2000ms
                action_MaxTime = action_timer;
                action_count   = 0;
                positionChange = false;
                spiralPhase    = 0; // 螺旋弹幕相位

                m_pdata->actionData.attackMode = AttackMode::MODE_4;
                break;

            case 5:
                // MODE_5: 时空裂隙 - 快速发射带随机缺口的弹幕墙
                action_timer   = TemporalRiftTime; // 2500ms
                action_MaxTime = action_timer;
                action_count   = 0;
                positionChange = false;
                riftWaveCount  = 0; // 裂隙波次计数

                m_pdata->actionData.attackMode = AttackMode::MODE_5;
                break;

            case 6:
                // MODE_6: 归于混沌 - 全屏弹幕攻击
                action_timer     = ReturnToChaosWarning + ReturnToChaosTime; // 600ms
                action_MaxTime   = action_timer;
                action_count     = 0;
                warningDisplayed = false;

                m_pdata->actionData.attackMode = AttackMode::MODE_6;
                break;

            default:
                break;
            }
        }
    }
}

void HundunEnemy::doAction() {
    if (m_pdata->initData.isInited == false) {
        return;
    }

    if (m_pdata->deathData.isDead) {
        return;
    }

    switch (m_pdata->actionData.currentState) {
    case ActionState::IDLE:
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

    case ActionState::ATTACKING:
        // 攻击计时
        action_count += controlDelayTime;

        // 攻击倒计时
        if (action_timer >= controlDelayTime)
            action_timer -= controlDelayTime;
        else
            action_timer = 0;

        // 执行相应攻击动作
        switch (m_pdata->actionData.attackMode) {
        case AttackMode::MODE_1:
            chaosSurge();
            break;
        case AttackMode::MODE_2:
            sealSevenApertures();
            break;
        case AttackMode::MODE_3:
            voidPull();
            break;
        case AttackMode::MODE_4:
            chaoticBarrage();
            break;
        case AttackMode::MODE_5:
            temporalRift();
            break;
        case AttackMode::MODE_6:
            returnToChaos();
            break;
        default:
            break;
        }

        // 动作完成
        if (action_timer == 0) {
            m_pdata->actionData.currentState       = ActionState::IDLE;
            m_pdata->actionData.attackMode         = AttackMode::NONE;
            action_count                           = 0;
            m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;

            //  重置BOSS攻击状态变量
            positionChange     = false;
            aperturesGenerated = false;
            warningDisplayed   = false;
            spiralPhase        = 0;
            riftWaveCount      = 0;
        }
        break;
    }
}

void HundunEnemy::drawRole() {
    if (m_pdata->img != nullptr && m_pdata->isActive && !m_pdata->deathData.isDead) {
        // 绘制BOSS本体
        OLED_DrawImage(
            m_pdata->spatialData.currentPosX, m_pdata->spatialData.currentPosY, m_pdata->img, OLED_COLOR_NORMAL
        );

        // MODE_2: 封印七窍 - 封印7个七窍
        if (m_pdata->actionData.attackMode == AttackMode::MODE_2 && aperturesGenerated) {
            // 封印效果闪烁时才显示
            bool showApertures = ((action_timer / 100) % 2 == 0);
            if (showApertures) {
                for (uint8_t i = 0; i < ApertureCount; i++) {
                    uint8_t ax = aperturePositions[i][0];
                    uint8_t ay = aperturePositions[i][1];
                    // 封印七窍的8x8像素矩形区域
                    OLED_DrawFilledRectangle(ax, ay, 8, 8, OLED_COLOR_NORMAL);
                }
            }
        }

        // MODE_6: 归于混沌 - 显示警告边框
        if (m_pdata->actionData.attackMode == AttackMode::MODE_6 && !warningDisplayed) {
            // 警告边框，整个屏幕边缘闪烁
            if ((action_timer / 50) % 2 == 0) {
                OLED_DrawRectangle(0, 0, 127, 63, OLED_COLOR_NORMAL);
                OLED_DrawRectangle(1, 1, 125, 61, OLED_COLOR_NORMAL);
            }
        }
    }

    // 死亡效果
    if (m_pdata->deathData.isDead) {
        uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
        uint8_t radius  = (HundunEnemyDeadTime - m_pdata->deathData.deathTimer) * 35 / HundunEnemyDeadTime;
        radius          = etl::max(radius, uint8_t(1));

        // 死亡效果 - 扩散的同心圆
        OLED_DrawCircle(centerX, centerY, radius, OLED_COLOR_NORMAL);
        if (radius > 5) {
            OLED_DrawCircle(centerX, centerY, radius - 5, OLED_COLOR_NORMAL);
        }
    }
}

void HundunEnemy::die() {
    if (m_pdata->deathData.deathTimer > 0) {
        m_pdata->deathData.deathTimer -= controlDelayTime;
        m_pdata->deathData.deathTimer = etl::max(m_pdata->deathData.deathTimer, uint16_t(0));
        return;
    }

    m_pdata->isActive = false;
}

//===========================  ===========================

/**
 * @brief MODE_1: 混沌涌动
 * @note  BOSS快速移动同时发射多方向弹幕（普通子弹+火球）
 *        对应攻击"涌动的混沌"效果，覆盖屏幕区域
 *        攻击结束后BOSS返回屏幕中央位置
 */
void HundunEnemy::chaosSurge() {
    if (action_count < ChaosSurgeInterval) // 每500ms执行一次
        return;
    action_count = 0;

    // 攻击即将结束时，移动到屏幕中央
    if (action_timer <= ChaosSurgeInterval) {
        // 最后一次闪烁，落地到屏幕中央
        uint8_t screenCenterX = 64;                                  // 屏幕中央X (128/2 - 宽度/2)
        int8_t  screenCenterY = 32 - m_pdata->spatialData.sizeY / 2; // 屏幕中央Y (64/2 - 高度/2)

        m_pdata->spatialData.currentPosX = screenCenterX;
        m_pdata->spatialData.currentPosY = screenCenterY;
        m_pdata->spatialData.refPosX     = screenCenterX;
        m_pdata->spatialData.refPosY     = screenCenterY;
        return;
    }

    // 生成新的随机位置
    uint8_t newX = 30 + rand() % 71;  // 30-100 范围X位置
    int8_t  newY = -20 + rand() % 70; // -20-49 范围Y位置，允许部分超出屏幕

    // 限制Y位置范围
    if (newY < -30) newY = -30;
    if (newY > 60) newY = 60;

    m_pdata->spatialData.currentPosX = newX;
    m_pdata->spatialData.currentPosY = newY;
    m_pdata->spatialData.refPosX     = newX;
    m_pdata->spatialData.refPosY     = newY;

    // 计算BOSS中心位置
    uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

    // 8个方向发射子弹（4正向 + 4斜向）
    int8_t directions[8][2] = {
        {-1, 0 }, // 左
        {1,  0 }, // 右
        {0,  -1}, // 上
        {0,  1 }, // 下
        {-1, -1}, // 左上
        {1,  -1}, // 右上
        {-1, 1 }, // 左下
        {1,  1 }  // 右下
    };

    // 发射8方向普通子弹
    for (uint8_t i = 0; i < 8; i++) {
        m_pdata->attackData.shootCooldownTimer = 0;
        uint8_t bulletX = centerX + directions[i][0] * 20;
        uint8_t bulletY = centerY + directions[i][1] * 20;
        shoot(bulletX, bulletY, BulletType::BASIC);
    }

    // 额外发射4方向第二层子弹（增加弹幕密度）
    for (uint8_t i = 0; i < 4; i++) {
        m_pdata->attackData.shootCooldownTimer = 0;
        uint8_t bulletX = centerX + directions[i][0] * 25 + (rand() % 6 - 3);
        uint8_t bulletY = centerY + directions[i][1] * 25 + (rand() % 6 - 3);
        shoot(bulletX, bulletY, BulletType::BASIC);
    }

    // 根据血量比例，血量越低越有可能发射火球
    float healthRatio = (float)m_pdata->healthData.currentHealth / (float)m_pdata->healthData.maxHealth;
    if (rand() % 100 < (100 - (int)(healthRatio * 80))) { // 血量低时概率更高
        m_pdata->attackData.shootCooldownTimer = 0;
        uint8_t fireDir = rand() % 4; // 随机一个正向方向
        shoot(centerX + directions[fireDir][0] * 15, centerY + directions[fireDir][1] * 15, BulletType::FIRE_BALL);
    }
}

/**
 * @brief MODE_2: 封印七窍
 * @note  在屏幕上随机生成7个光点作为封印七窍
 *        对应"封印七窍"的效果
 */
void HundunEnemy::sealSevenApertures() {
    // 仅在首次初始化时生成随机位置
    if (!aperturesGenerated) {
        for (uint8_t i = 0; i < ApertureCount; i++) {
            // 生成随机位置，确保位置合理
            aperturePositions[i][0] = rand() % 100 + 10; // 10-109 X位置
            aperturePositions[i][1] = rand() % 48 + 8;   // 8-55 Y位置
        }
        aperturesGenerated = true;
    }

    // 在drawRole()中绘制光点
    // 这里不需要额外逻辑，每帧根据aperturePositions绘制光点
}

/**
 * @brief MODE_3: 虚空牵引
 * @note  将玩家向BOSS位置缓慢拉近，同时发射火球弹
 *        对应攻击"虚空牵引"效果
 */
void HundunEnemy::voidPull() {
    if (action_count < VoidPullInterval) // 每300ms执行一次
        return;
    action_count = 0;

    IRole *player = g_entityManager.getPlayerRole();
    if (player == nullptr) return;

    // 计算BOSS和玩家的中心位置
    int16_t bossX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    int16_t bossY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

    int16_t playerX = player->getData()->spatialData.currentPosX + player->getData()->spatialData.sizeX / 2;
    int16_t playerY = player->getData()->spatialData.currentPosY + player->getData()->spatialData.sizeY / 2;

    int16_t deltaX = bossX - playerX;
    int16_t deltaY = bossY - playerY;

    int8_t dirX = 0, dirY = 0;
    if (deltaX < 0) dirX = -1;
    if (deltaX > 0) dirX = 1;
    if (deltaY < 0) dirY = -1;
    if (deltaY > 0) dirY = 1;

    // 将玩家向BOSS方向拉扯移动
    for (uint8_t i = 0; i < VoidPullDistance / 2; i++) {
        player->move(dirX * 2, dirY * 2, true);
    }

    // 同时BOSS发射火球，造成伤害
    m_pdata->attackData.shootCooldownTimer = 0;
    shoot(bossX, bossY, BulletType::FIRE_BALL);
}

/**
 * @brief MODE_4: 混沌漩涡
 * @note  螺旋式发射子弹，Y位置按正弦波扫射
 *        对应攻击"混沌漩涡"效果，覆盖屏幕区域
 */
void HundunEnemy::chaoticBarrage() {
    if (action_count < ChaoticBarrageInterval) // 每150ms执行一次
        return;
    action_count = 0;

    // 第一次执行时移动到中间位置
    if (!positionChange) {
        m_pdata->spatialData.currentPosX = 60;
        m_pdata->spatialData.currentPosY = 0;
        m_pdata->spatialData.refPosX     = 60;
        m_pdata->spatialData.refPosY     = 0;
        positionChange                   = true;
        spiralPhase                      = 0; // 螺旋弹幕初始相位
    }

    uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

    // 螺旋弹幕：Y位置根据相位变化上下摆动
    // 使用相位计算偏移，位置0-8对应Y偏移-25到+25之间，反向-25
    int8_t  yOffset = 0;
    uint8_t phase   = spiralPhase % 16; // 16个相位为一周期
    if (phase < 8) {
        yOffset = -25 + phase * 6; // 0->-25, 1->-19, ..., 7->17
    } else {
        yOffset = 25 - (phase - 8) * 6; // 8->25, 9->19, ..., 15->-17
    }

    m_pdata->attackData.shootCooldownTimer = 0;
    shoot(centerX, centerY + yOffset, BulletType::BASIC);

    spiralPhase++; // 相位递增，实现螺旋效果
}

/**
 * @brief MODE_5: 时空裂隙
 * @note  快速发射带随机缺口的弹幕墙，缺口位置每轮变化
 *        对应攻击"时空裂隙"效果
 */
void HundunEnemy::temporalRift() {
    if (action_count < TemporalRiftInterval) // 每 400ms 执行一次
        return;
    action_count = 0;

    // 移动到中间位置
    if (!positionChange) {
        m_pdata->spatialData.currentPosX = 60;
        m_pdata->spatialData.currentPosY = 0;
        m_pdata->spatialData.refPosX     = 60;
        m_pdata->spatialData.refPosY     = 0;
        positionChange                   = true;
    }

    uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;

    // 计算缺口位置和大小
    uint8_t gapCenter = 8 + rand() % 44; // 缺口中心的Y位置 8-51
    uint8_t gapSize   = RiftGapSize;     // 缺口大小为12个像素

    // 发射一排带缺口的子弹
    for (uint8_t y = 2; y < 62; y += 6) {
        // 判断是否在缺口范围内
        if (y >= gapCenter - gapSize / 2 && y <= gapCenter + gapSize / 2) {
            continue; // 跳过缺口区域
        }
        m_pdata->attackData.shootCooldownTimer = 0;
        shoot(centerX, y, BulletType::BASIC);
    }

    // 每2波在缺口中心发射一个火球
    riftWaveCount++;
    if (riftWaveCount % 2 == 0) {
        m_pdata->attackData.shootCooldownTimer = 0;
        shoot(centerX, gapCenter, BulletType::FIRE_BALL); // 在缺口中心位置发射火球
    }
}

/**
 * @brief MODE_6: 归于混沌
 * @note  全屏弹幕政击，中间有安全缺口，血量越低缺口越小
 *        对应攻击"归于混沌"效果
 */
void HundunEnemy::returnToChaos() {
    // 阶段1: 警告阶段 (前500ms)
    if (action_timer > ReturnToChaosTime) {
        // 警告阶段，在 drawRole() 中绘制边框
        return;
    }

    // 阶段2: 警告阶段 (前100ms只执行一次)
    if (!warningDisplayed) {
        warningDisplayed = true;

        // 移动到中间位置
        m_pdata->spatialData.currentPosX = 60;
        m_pdata->spatialData.currentPosY = 0;
        m_pdata->spatialData.refPosX     = 60;
        m_pdata->spatialData.refPosY     = 0;

        uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

        // 根据血量比例计算安全缺口大小
        // 血量越低，缺口越小，难度越高
        float  healthRatio = (float)m_pdata->healthData.currentHealth / (float)m_pdata->healthData.maxHealth;
        int8_t gapSize     = 8 + (int8_t)(healthRatio * 12); // 缺口大小 8-20

        // 全屏弹幕，中间留缺口
        for (int8_t offsetY = -30; offsetY <= 30; offsetY += 5) {
            // 跳过中间安全缺口区域
            if (offsetY >= -gapSize / 2 && offsetY <= gapSize / 2) {
                continue;
            }
            m_pdata->attackData.shootCooldownTimer = 0;
            shoot(centerX, centerY + offsetY, BulletType::BASIC);
            // 发射第二排子弹增加密度
            m_pdata->attackData.shootCooldownTimer = 0;
            shoot(centerX + 15, centerY + offsetY + 2, BulletType::BASIC);
        }

        // 在缺口两侧发射火球作为边界标记
        m_pdata->attackData.shootCooldownTimer = 0;
        shoot(centerX, centerY - gapSize / 2 - 5, BulletType::FIRE_BALL);
        m_pdata->attackData.shootCooldownTimer = 0;
        shoot(centerX, centerY + gapSize / 2 + 5, BulletType::FIRE_BALL);
    }
}

/*******************************************************************/
