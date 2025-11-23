#include "enemyRole.hpp"
#include "bullet.hpp"

#include "etl/algorithm.h"
#include "../Peripheral/OLED/oled.h"
#include "../gameEntityManager.hpp"

#include "FreeRTOS.h"
#include "task.h"

extern GameEntityManager g_entityManager;

//数值规范
//基础血量: 30（低血量），100（中血量），200（高血量）
//基础攻击力: 1-5 （低攻击），5-15（中攻击），15+（高攻击）
//基础移动速度: 1（低速），2（中速），3（高速）

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

//闪电链弹一束条的范围穿透伤害，multiplier*attackPower+10 点伤害

/*******************************************************************/
/**
 * @brief FeilianEnemy class
 * @note  中文：飞廉 ｜ 英文：Feilian,神话典故：中国古代神话中的风神，形如鹿、头生角、有翼，行走如飞，负责掌管八面来风。
 * @note  高速移动的小型敌人，体型小巧（12x12 像素），移动轨迹飘忽（呼应 “风” 的特性），单次攻击伤害低，但成群出现时压迫感强。
 * @note  只会发射普通子弹，但行动方式飘忽不定。
 */

//数值设定参考
//血量：30 + level * 1
//攻击力：1 + level * 1
//移动速度：3（高速）
//射击冷却时间：4000/5 ms
//热量机制：每次射击增加20点热量，最大热量100点，冷却速率10点/200ms

FeilianEnemy::FeilianEnemy(uint8_t startX, uint8_t startY, uint8_t initPosX, uint8_t initPosY, uint8_t level , uint16_t dropExp)
: IRole() {
    //图片信息
    m_pdata->img = &feilianImg;

    //身份信息
    m_pdata->identity          = RoleIdentity::ENEMY;
    m_pdata->isActive          = true;
    m_pdata->initData.isInited = false;

    //等级信息
    m_pdata->level = level;

    //血量信息
    m_pdata->healthData.currentHealth = 30 + level * 30;
    m_pdata->healthData.maxHealth     = 30 + level * 30;

    //回血信息
    m_pdata->healthData.healValue       = 0;
    m_pdata->healthData.healTimeCounter = 0;
    m_pdata->healthData.healResetTime   = 15000;
    m_pdata->healthData.healSpeed       = 0;

    //空间移动信息
    m_pdata->spatialData.canCrossBorder            = false;
    m_pdata->spatialData.currentPosX               = startX; // Starting X position
    m_pdata->spatialData.currentPosY               = startY; // Starting Y position
    m_pdata->spatialData.refPosX                   = startX;
    m_pdata->spatialData.refPosY                   = startY;
    m_pdata->spatialData.sizeX                     = m_pdata->img->w;
    m_pdata->spatialData.sizeY                     = m_pdata->img->h;
    m_pdata->spatialData.moveSpeed                 = 3; // Set movement speed
    m_pdata->spatialData.consecutiveCollisionCount = 0;

    //初始化位置
    m_pdata->initData.posX = initPosX;
    m_pdata->initData.posY = initPosY;

    //攻击信息
    m_pdata->attackData.attackPower            = 3 + level * 1;
    m_pdata->attackData.shootCooldownSpeed     = 5;
    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 4000;
    m_pdata->attackData.bulletSpeed            = 1;

    m_pdata->attackData.bulletRange            = 0; //只对火球弹生效
    m_pdata->attackData.bulletDamageMultiplier = 1.5f;

    m_pdata->attackData.collisionPower = 4 + level * 1;

    //热量信息
    m_pdata->heatData.maxHeat          = 100;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 20;
    m_pdata->heatData.heatCoolDownRate = 10; //每次冷却10点热量，每次冷却时间间隔由200ms

    //死亡状态信息
    m_pdata->deathData.deathTimer = feilianEnemyDeadTime;
    m_pdata->deathData.isDead     = false;
    m_pdata->deathData.dropExperiencePoints = dropExp;

    // Initialize other enemy-specific data here
}

void FeilianEnemy::shoot(uint8_t x, uint8_t y, BulletType type) {
    // Implement enemy shooting logic
    // Create bullet based on type

    switch (type) {
    case BulletType::BASIC:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot > m_pdata->heatData.maxHeat)
                return;                                             // 超过最大热量，无法射击
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // 冷却中，无法射击

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
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 2 > m_pdata->heatData.maxHeat)
                return;                                             // 超过最大热量，无法射击
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // 冷却中，无法射击

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
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 1.5 > m_pdata->heatData.maxHeat)
                return;                                             // 超过最大热量，无法射击
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // 冷却中，无法射击

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
}

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
    if (think_count < 100) // 每100ms决定一次行动
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

        //射击
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
        die();
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
        // 绘制死亡动画（例如一个简单的圆圈表示消失效果）
        uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
        uint8_t radius  = (feilianEnemyDeadTime - m_pdata->deathData.deathTimer) / 100; // 从0增长到最大值5
        radius          = etl::max(radius, uint8_t(1));                                 // 最小半径限制

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
 * @brief GudiaoEnemy class
 * @note  中文：蛊雕 ｜ 英文：Gudiao,神话典故：山海经中大型猛禽凶兽，以哭声诱捕人类，擅长飞行捕猎，是山中食人恶兽的代表。
 * @note  中速飞行的中型敌人，体型居中（15x15 像素），攻击力较高，适合伏击玩家。
 * @note  攻击方式为发射高伤害普通子弹，每次从身体两侧发射。
 */
GudiaoEnemy::GudiaoEnemy(uint8_t startX, uint8_t startY, uint8_t initPosX, uint8_t initPosY, uint8_t level , uint16_t dropExp )
: IRole() {
    //图片信息
    m_pdata->img = &GudiaoImg;

    //身份信息
    m_pdata->identity          = RoleIdentity::ENEMY;
    m_pdata->isActive          = true;
    m_pdata->initData.isInited = false;

    //等级信息
    m_pdata->level = level;

    //血量信息
    m_pdata->healthData.currentHealth = 330 + level * 100;
    m_pdata->healthData.maxHealth     = 330 + level * 100;

    //回血信息
    m_pdata->healthData.healValue       = 20;
    m_pdata->healthData.healTimeCounter = 0;
    m_pdata->healthData.healResetTime   = 15000;
    m_pdata->healthData.healSpeed       = 5;

    //空间移动信息
    m_pdata->spatialData.canCrossBorder            = false;
    m_pdata->spatialData.currentPosX               = startX; // Starting X position
    m_pdata->spatialData.currentPosY               = startY; // Starting Y position
    m_pdata->spatialData.refPosX                   = startX;
    m_pdata->spatialData.refPosY                   = startY;
    m_pdata->spatialData.sizeX                     = m_pdata->img->w;
    m_pdata->spatialData.sizeY                     = m_pdata->img->h;
    m_pdata->spatialData.moveSpeed                 = 1; // Set movement speed
    m_pdata->spatialData.consecutiveCollisionCount = 0;

    //初始化位置
    m_pdata->initData.posX = initPosX;
    m_pdata->initData.posY = initPosY;

    //攻击信息
    m_pdata->attackData.attackPower            = 8 + level * 2;
    m_pdata->attackData.shootCooldownSpeed     = 5;
    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 16000; //32000 ms
    m_pdata->attackData.bulletSpeed            = 1;

    m_pdata->attackData.bulletRange            = 10; //只对火球弹生效
    m_pdata->attackData.bulletDamageMultiplier = 1.5f;

    m_pdata->attackData.collisionPower = 4 + level * 1;

    //热量信息
    m_pdata->heatData.maxHeat          = 150;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 20;
    m_pdata->heatData.heatCoolDownRate = 10; //每次冷却10点热量，每次冷却时间间隔由200ms

    //死亡状态信息
    m_pdata->deathData.deathTimer = gudiaoEnemyDeadTime;
    m_pdata->deathData.isDead     = false;
    m_pdata->deathData.dropExperiencePoints = dropExp;

    // Initialize other enemy-specific data here
}

void GudiaoEnemy::shoot(uint8_t x, uint8_t y, BulletType type) {
    // Implement enemy shooting logic
    // Create bullet based on type
    switch (type) {
    case BulletType::BASIC:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot > m_pdata->heatData.maxHeat)
                return;                                             // 超过最大热量，无法射击
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // 冷却中，无法射击

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
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 2 > m_pdata->heatData.maxHeat)
                return;                                             // 超过最大热量，无法射击
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // 冷却中，无法射击

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
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 1.5 > m_pdata->heatData.maxHeat)
                return;                                             // 超过最大热量，无法射击
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // 冷却中，无法射击

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
}

void GudiaoEnemy::init() {
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

void GudiaoEnemy::think() {
    // Implement enemy AI logic
    think_count += controlDelayTime;
    if (think_count < 200) // 每200ms决定一次行动
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

        //射击
        else if (randomAction == 5) {
            if (m_pdata->attackData.shootCooldownTimer <= 0) {
                m_pdata->actionData.attackMode   = AttackMode::MODE_1;
                m_pdata->actionData.currentState = ActionState::ATTACKING;
            } else {
                // 如果在冷却中，则不进行攻击，保持空闲状态
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
        die();
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

        //检查冷却时间
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
            m_pdata->attackData.shootCooldownTimer = 0; //重置冷却时间，快速发射第二发子弹
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
        // 绘制死亡动画（例如一个简单的圆圈表示消失效果）
        uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
        uint8_t radius  = (gudiaoEnemyDeadTime - m_pdata->deathData.deathTimer) / 100; // 从0增长到最大值5
        radius          = etl::max(radius, uint8_t(1));                                // 最小半径限制

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

    //亡语：发射一个火球弹
    IBullet *newBullet = createBullet(
        m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2,
        m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2, BulletType::FIRE_BALL
    );
    if (newBullet != nullptr) {
        if (!g_entityManager.addBullet(newBullet)) {
            delete[] newBullet; // Clean up if not added
        }
    }
    m_pdata->isActive = false;
}

/*******************************************************************/

/*******************************************************************/
/**
 * @brief ChiMeiEnemy class
 * @note  中文：魑魅 ｜ 英文：ChiMei,神话典故：传说中息于山林间的妖怪，善于迷惑人心，引诱迷路的旅人深入山林，最终将其吞噬。
 * @note  高速移动的小型敌人，体型极小（8x8 像素），自杀式冲撞。
 */

ChiMeiEnemy::ChiMeiEnemy(uint8_t startX, uint8_t startY, uint8_t initPosX, uint8_t initPosY, uint8_t level , uint16_t dropExp)
: IRole() {
    //图片信息
    m_pdata->img = &ChiMeiImg;

    //身份信息
    m_pdata->identity          = RoleIdentity::ENEMY;
    m_pdata->isActive          = true;
    m_pdata->initData.isInited = false;

    //等级信息
    m_pdata->level = level;

    //血量信息
    m_pdata->healthData.currentHealth = 1 + level * 1;
    m_pdata->healthData.maxHealth     = 1 + level * 1;

    //回血信息
    m_pdata->healthData.healValue       = 0;
    m_pdata->healthData.healTimeCounter = 0;
    m_pdata->healthData.healResetTime   = 15000;
    m_pdata->healthData.healSpeed       = 0;

    //空间移动信息
    m_pdata->spatialData.canCrossBorder            = false;
    m_pdata->spatialData.currentPosX               = startX; // Starting X position
    m_pdata->spatialData.currentPosY               = startY; // Starting Y position
    m_pdata->spatialData.refPosX                   = startX;
    m_pdata->spatialData.refPosY                   = startY;
    m_pdata->spatialData.sizeX                     = m_pdata->img->w;
    m_pdata->spatialData.sizeY                     = m_pdata->img->h;
    m_pdata->spatialData.moveSpeed                 = 3; // Set movement speed
    m_pdata->spatialData.consecutiveCollisionCount = 0;

    //初始化位置
    m_pdata->initData.posX = initPosX;
    m_pdata->initData.posY = initPosY;

    //攻击信息
    m_pdata->attackData.attackPower            = 1 + level * 1;
    m_pdata->attackData.shootCooldownSpeed     = 5;
    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 4000;
    m_pdata->attackData.bulletSpeed            = 2;

    m_pdata->attackData.bulletRange            = 0;    //只对火球弹生效
    m_pdata->attackData.bulletDamageMultiplier = 1.5f; //只对闪电链弹生效

    m_pdata->attackData.collisionPower = 20;

    //热量信息
    m_pdata->heatData.maxHeat          = 100;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 20;
    m_pdata->heatData.heatCoolDownRate = 10; //每次冷却10点热量，每次冷却时间间隔由200ms

    //死亡状态信息
    m_pdata->deathData.deathTimer = chimeiEnemyDeadTime;
    m_pdata->deathData.isDead     = false;
    m_pdata->deathData.dropExperiencePoints = dropExp;

    // Initialize other enemy-specific data here
}

void ChiMeiEnemy::shoot(uint8_t x, uint8_t y, BulletType type) {
    // Implement enemy shooting logic
    // Create bullet based on type
    switch (type) {
    case BulletType::BASIC:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot > m_pdata->heatData.maxHeat)
                return;                                             // 超过最大热量，无法射击
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // 冷却中，无法射击

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
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 2 > m_pdata->heatData.maxHeat)
                return;                                             // 超过最大热量，无法射击
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // 冷却中，无法射击

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
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 1.5 > m_pdata->heatData.maxHeat)
                return;                                             // 超过最大热量，无法射击
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // 冷却中，无法射击

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
    if (think_count < 150) // 每150ms决定一次行动
        return;

    think_count = 0;

    //魑魅只会向左移动
    if (m_pdata->actionData.currentState == ActionState::IDLE) {
        //移动
        m_pdata->actionData.moveMode     = MoveMode::LEFT;
        m_pdata->actionData.currentState = ActionState::MOVING;
    }
}

void ChiMeiEnemy::doAction() {
    if (m_pdata->initData.isInited == false) {
        return;
    }

    //魑魅的特殊边界死亡检查
    if (m_pdata->spatialData.currentPosX <= boundary_deadzone) {
        m_pdata->deathData.isDead = true;
    }

    // Implement enemy action logic
    if (m_pdata->deathData.isDead) {
        die();
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
        // 绘制死亡动画（例如一个简单的圆圈表示消失效果）
        uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
        uint8_t radius  = (chimeiEnemyDeadTime - m_pdata->deathData.deathTimer) / 100; // 从0增长到最大值5
        radius          = etl::max(radius, uint8_t(1));                                // 最小半径限制

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

TaotieEnemy::TaotieEnemy(uint8_t startX, uint8_t startY, uint8_t initPosX, uint8_t initPosY, uint8_t level , uint16_t dropExp)
: IRole() {
    //图片信息
    m_pdata->img = &TaotieImg;

    //身份信息
    m_pdata->identity          = RoleIdentity::ENEMY;
    m_pdata->isActive          = true;
    m_pdata->initData.isInited = false;

    //等级信息
    m_pdata->level = level;

    //血量信息
    m_pdata->healthData.currentHealth = 1850 + level * 400;
    m_pdata->healthData.maxHealth     = 1850 + level * 400;

    //回血信息
    m_pdata->healthData.healValue       = 20;
    m_pdata->healthData.healTimeCounter = 0;
    m_pdata->healthData.healResetTime   = 15000;
    m_pdata->healthData.healSpeed       = 5;

    //空间移动信息
    m_pdata->spatialData.canCrossBorder            = true;
    m_pdata->spatialData.currentPosX               = startX; // Starting X position
    m_pdata->spatialData.currentPosY               = startY; // Starting Y position
    m_pdata->spatialData.refPosX                   = startX;
    m_pdata->spatialData.refPosY                   = startY;
    m_pdata->spatialData.sizeX                     = m_pdata->img->w;
    m_pdata->spatialData.sizeY                     = m_pdata->img->h;
    m_pdata->spatialData.moveSpeed                 = 1; // Set movement speed
    m_pdata->spatialData.consecutiveCollisionCount = 0;

    //初始化位置
    m_pdata->initData.posX = initPosX;
    m_pdata->initData.posY = initPosY;

    //攻击信息
    m_pdata->attackData.attackPower            = 5 + level * 2;
    m_pdata->attackData.shootCooldownSpeed     = 5;
    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 5000; //5000 ms
    m_pdata->attackData.bulletSpeed            = 1;

    m_pdata->attackData.bulletRange            = 10;   //只对火球弹生效
    m_pdata->attackData.bulletDamageMultiplier = 1.5f; //只对闪电链弹生效

    m_pdata->attackData.collisionPower = 20 + level * 5;

    //热量信息
    m_pdata->heatData.maxHeat          = 200;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 20;
    m_pdata->heatData.heatCoolDownRate = 10; //每次冷却10点热量，每次冷却时间间隔由200ms

    //死亡状态信息
    m_pdata->deathData.deathTimer = TaotieEnemyDeadTime;
    m_pdata->deathData.isDead     = false;
    m_pdata->deathData.dropExperiencePoints = dropExp;

    // Initialize other enemy-specific data here
}

void TaotieEnemy::shoot(uint8_t x, uint8_t y, BulletType type) {
    // Implement enemy shooting logic
    // Create bullet based on type
    switch (type) {
    case BulletType::BASIC:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot > m_pdata->heatData.maxHeat)
                return;                                             // 超过最大热量，无法射击
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // 冷却中，无法射击

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
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 2 > m_pdata->heatData.maxHeat)
                return;                                             // 超过最大热量，无法射击
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // 冷却中，无法射击

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
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 1.5 > m_pdata->heatData.maxHeat)
                return;                                             // 超过最大热量，无法射击
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // 冷却中，无法射击

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
}

void TaotieEnemy::init() {
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

void TaotieEnemy::think() {
    // Implement enemy AI logic
    think_count += controlDelayTime;
    if (think_count < 100) // 每100ms决定一次行动
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
                // 如果在冷却中，则不进行攻击，保持空闲状态
                m_pdata->actionData.moveMode     = MoveMode::NONE;
                m_pdata->actionData.currentState = ActionState::IDLE;
                return;
            }

            uint8_t randomAttackMode         = rand() % 5 + 1; // 1-5 攻击方式
            m_pdata->actionData.currentState = ActionState::ATTACKING;

            switch (randomAttackMode) {
            case 1:
                //攻击模式1 - 吞噬攻击
                action_timer                   = 1500; // 饕餮攻击动作持续时间1500ms
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                m_pdata->actionData.attackMode = AttackMode::MODE_1;
                break;
            case 2:
                //攻击模式2 - 三排子弹攻击
                action_timer                   = 1000; // 饕餮攻击动作持续时间1000ms
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                m_pdata->actionData.attackMode = AttackMode::MODE_2;
                break;

            case 3:
                //攻击模式3 - 冲锋撞击攻击
                //500ms冲锋+1000ms停顿+500ms回退
                action_timer                   = 2000; // 饕餮攻击动作持续时间2000ms
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                m_pdata->actionData.attackMode = AttackMode::MODE_3;
                break;
            case 4:
                //攻击模式4 - 向后碾压攻击
                //crush from left side
                action_timer                   = 4000; // 饕餮攻击动作持续时间4000ms
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                appearedForCrush               = false;
                comeBackForCrush               = false;
                m_pdata->actionData.attackMode = AttackMode::MODE_4;
                break;
            case 5:
                //攻击模式5 - 拉近并冲锋攻击
                action_timer                   = 3000; // 饕餮攻击动作持续时间3000ms
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                m_pdata->actionData.attackMode = AttackMode::MODE_5;
                break;
            default:
                //默认攻击模式1 - 吞噬攻击
                action_timer                   = 1500; // 饕餮攻击动作持续时间1500ms
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

    // Implement enemy action logic
    if (m_pdata->deathData.isDead) {
        die();
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
        //动作作用时间，用来调节动作发生的频率
        action_count += controlDelayTime;

        //动作倒计时
        if (action_timer >= controlDelayTime)
            action_timer -= controlDelayTime;
        else
            action_timer = 0;

        switch (m_pdata->actionData.attackMode) {
        //执行攻击动作
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
        default:
            break;
        }

        if (action_timer == 0) {
            m_pdata->actionData.currentState       = ActionState::IDLE;
            m_pdata->actionData.attackMode         = AttackMode::NONE;
            action_count                           = 0;
            m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime; // 攻击后进入冷却时间
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
        // 绘制死亡动画（例如一个简单的圆圈表示消失效果）
        uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
        uint8_t radius =
            (TaotieEnemyDeadTime - m_pdata->deathData.deathTimer) * 30 / TaotieEnemyDeadTime; // 从0增长到最大值5
        radius = etl::max(radius, uint8_t(1));                                                // 最小半径限制

        OLED_DrawCircle(centerX, centerY, radius, OLED_COLOR_NORMAL);
    }
}

void TaotieEnemy::die() {
    // Implement enemy death logic
    if (m_pdata->deathData.deathTimer > 0) {
        m_pdata->deathData.deathTimer -= controlDelayTime;
        m_pdata->deathData.deathTimer = etl::max(m_pdata->deathData.deathTimer, uint16_t(0));
        return;
    }

    m_pdata->isActive = false;
}

//攻击技能
void TaotieEnemy::pullPlayerAndDevourAttack() {
    //将玩家向自己拉近，进行吞噬攻击
    if (action_count < action_MaxTime / pullDistance) // 每50ms拉近一次
        return;
    action_count = 0;

    IRole *player = g_entityManager.getPlayerRole();
    if (player == nullptr) return;

    uint16_t dirX = 0;
    uint16_t dirY = 0;
    //计算方向向量

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

    //更新玩家位置
    player->move(dirX, dirY, true);
}

void TaotieEnemy::fireThreeRowsBasicBullets() {
    if (action_count < 500) // 每500ms发射一次
        return;
    action_count = 0;

    //发射三排普通子弹
    uint8_t m_x = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t m_y = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

    uint8_t m_x_1 = m_x;
    uint8_t m_y_1 = m_y - 6;
    uint8_t m_x_2 = m_x;
    uint8_t m_y_2 = m_y;
    uint8_t m_x_3 = m_x;
    uint8_t m_y_3 = m_y + 6;

    m_pdata->attackData.shootCooldownTimer = 0; //重置冷却时间，快速发射第一发子弹
    m_pdata->heatData.currentHeat          = 0; //重置热量信息
    shoot(m_x, m_y - 6, BulletType::BASIC);
    m_pdata->attackData.shootCooldownTimer = 0; //重置冷却时间，快速发射第二发子弹
    m_pdata->heatData.currentHeat          = 0; //重置热量信息
    shoot(m_x_2, m_y_2, BulletType::BASIC);
    m_pdata->attackData.shootCooldownTimer = 0; //重置冷却时间，快速发射第三发子弹
    m_pdata->heatData.currentHeat          = 0; //重置热量信息
    shoot(m_x_3, m_y_3, BulletType::BASIC);

    m_x_1 = m_x + 10;
    m_x_2 = m_x + 10;
    m_x_3 = m_x + 10;

    m_pdata->attackData.shootCooldownTimer = 0; //重置冷却时间，快速发射第一发子弹
    m_pdata->heatData.currentHeat          = 0; //重置热量信息
    shoot(m_x_1, m_y_1, BulletType::BASIC);
    m_pdata->attackData.shootCooldownTimer = 0; //重置冷却时间，快速发射第二发子弹
    m_pdata->heatData.currentHeat          = 0; //重置热量信息
    shoot(m_x_2, m_y_2, BulletType::BASIC);
    m_pdata->attackData.shootCooldownTimer = 0; //重置冷却时间，快速发射第三发子弹
    m_pdata->heatData.currentHeat          = 0; //重置热量信息
    shoot(m_x_3, m_y_3, BulletType::BASIC);
}

void TaotieEnemy::chargeForwardAndRamAttack() {
    //向前冲撞，进行撞击攻击
    //假设向左冲撞
    if (action_count < action_MaxTime / chargeDistance / 4) // 2000/30/4 ms移动一次
        return;
    action_count = 0;
    int8_t dir   = -1;                                                                        //向左冲撞
    if (action_timer < action_MaxTime * 3 / 4 && action_timer >= action_MaxTime / 4) dir = 0; //停顿
    if (action_timer < action_MaxTime / 4) dir = 1;                                           //向右回退
    move(dir, 0);
}

void TaotieEnemy::appearLeftAndRollBackCrushAttack() {
    // //向后碾压，从玩家左侧出现，进行碾压攻击

    //因为要算来回，所以距离触发时间除以2
    if (action_count < action_MaxTime / crushChargeDistance / 2) // 4000/100/2=20 ms移动一次
        return;
    action_count = 0;

    RoleData *taoTie = this->getData();
    if (taoTie == nullptr) return;

    if (action_timer >= action_MaxTime / 2) {
        //向后移动
        if (taoTie->spatialData.currentPosX < 120 && appearedForCrush == false) move(1, 0);

        //从左侧出现
        if (taoTie->spatialData.currentPosX >= 120 && appearedForCrush == false) {
            appearedForCrush                = true;
            taoTie->spatialData.refPosX     = -70; //重置位置
            taoTie->spatialData.refPosY     = 1;
            taoTie->spatialData.currentPosX = -70;
            taoTie->spatialData.currentPosY = 1;
        }
        //向右碾压
        if (appearedForCrush == true) {
            move(1, 0);
        }
    } else {
        //向左回退
        if (!comeBackForCrush) move(-1, 0);
        if (taoTie->spatialData.currentPosX <= -64) {
            //回到初始位置，结束攻击
            comeBackForCrush                = true;
            taoTie->spatialData.refPosX     = 120; //重置位置
            taoTie->spatialData.refPosY     = 1;
            taoTie->spatialData.currentPosX = 120;
            taoTie->spatialData.currentPosY = 1;
        }
        if (taoTie->spatialData.currentPosX > 64) {
            move(-1, 0);
        }
    }

    if (action_timer <= 50) {
        taoTie->spatialData.refPosX     = 64; //重置位置
        taoTie->spatialData.refPosY     = 1;
        taoTie->spatialData.currentPosX = 64;
        taoTie->spatialData.currentPosY = 1;
    }
}

void TaotieEnemy::pullPlayerAndChargeForwardAttack() {
    // //将玩家向自己拉近，同时向前冲撞

    //将玩家向自己拉近，进行吞噬攻击
    if (action_count < action_MaxTime / pullAndChargeDistance) // 每3000/120 ms 拉近一次
        return;
    action_count = 0;

    IRole *player = g_entityManager.getPlayerRole();
    if (player == nullptr) return;

    uint16_t dirX = 0;
    uint16_t dirY = 0;
    //计算方向向量

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

    //更新玩家位置
    player->move(dirX, dirY, true);

    //向前冲撞，进行撞击攻击
    //向左冲撞
    //冲撞距离=pullAndChargeDistance
    int8_t dir = -2;
    if (action_timer > action_MaxTime * 3 / 4) dir = -2;                                      //前半段时间向左冲撞
    if (action_timer <= action_MaxTime * 3 / 4 && action_timer > action_MaxTime / 4) dir = 0; //后半段时间停顿
    if (action_timer <= action_MaxTime / 4) dir = 2;                                          //最后时间向右回退
    move(dir, 0);
}

/*******************************************************************/

/*******************************************************************/
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
 * @note  攻击方式6，定向闪现，对齐玩家位置闪现，清除CD
 */

TaowuEnemy::TaowuEnemy(uint8_t startX, uint8_t startY, uint8_t initPosX, uint8_t initPosY, uint8_t level , uint16_t dropExp)
: IRole() {
    //图片信息
    m_pdata->img = &TaowuImg;

    //身份信息
    m_pdata->identity          = RoleIdentity::ENEMY;
    m_pdata->isActive          = true;
    m_pdata->initData.isInited = false;

    //等级信息
    m_pdata->level = level;

    //血量信息
    //血量较低，但攻击力高，移动速度快
    m_pdata->healthData.currentHealth = 830 + level * 300;
    m_pdata->healthData.maxHealth     = 830 + level * 300;

    //回血信息
    m_pdata->healthData.healValue       = 30;
    m_pdata->healthData.healTimeCounter = 0;
    m_pdata->healthData.healResetTime   = 15000;
    m_pdata->healthData.healSpeed       = 5;

    //空间移动信息
    m_pdata->spatialData.canCrossBorder            = true;
    m_pdata->spatialData.currentPosX               = startX; // Starting X position
    m_pdata->spatialData.currentPosY               = startY; // Starting Y position
    m_pdata->spatialData.refPosX                   = startX;
    m_pdata->spatialData.refPosY                   = startY;
    m_pdata->spatialData.sizeX                     = m_pdata->img->w;
    m_pdata->spatialData.sizeY                     = m_pdata->img->h;
    m_pdata->spatialData.moveSpeed                 = 3; // Set movement speed
    m_pdata->spatialData.consecutiveCollisionCount = 0;

    //初始化位置
    m_pdata->initData.posX = initPosX;
    m_pdata->initData.posY = initPosY;

    //攻击信息
    m_pdata->attackData.attackPower            = 10 + level * 2;
    m_pdata->attackData.shootCooldownSpeed     = 5;
    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 5000; //5000 ms
    m_pdata->attackData.bulletSpeed            = 1;

    m_pdata->attackData.bulletRange            = 10;   //只对火球弹生效
    m_pdata->attackData.bulletDamageMultiplier = 1.5f; //只对闪电链弹生效

    m_pdata->attackData.collisionPower = 7 + level * 3;

    //热量信息
    m_pdata->heatData.maxHeat          = 250;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 0;
    m_pdata->heatData.heatCoolDownRate = 10; //每次冷却10点热量，每次冷却时间间隔由200ms

    //死亡状态信息
    m_pdata->deathData.deathTimer = TaowuEnemyDeadTime;
    m_pdata->deathData.isDead     = false;
    m_pdata->deathData.dropExperiencePoints = dropExp;

    // Initialize other enemy-specific data here
}

void TaowuEnemy::shoot(uint8_t x, uint8_t y, BulletType type) {
    // Implement enemy shooting logic
    // Create bullet based on type
    switch (type) {
    case BulletType::BASIC:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot > m_pdata->heatData.maxHeat)
                return;                                             // 超过最大热量，无法射击
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // 冷却中，无法射击

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
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 2 > m_pdata->heatData.maxHeat)
                return;                                             // 超过最大热量，无法射击
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // 冷却中，无法射击

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
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 1.5 > m_pdata->heatData.maxHeat)
                return;                                             // 超过最大热量，无法射击
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // 冷却中，无法射击

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
    if (think_count < 100) // 每100ms决定一次行动
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
                // 如果在冷却中，则不进行攻击，保持空闲状态
                m_pdata->actionData.moveMode     = MoveMode::NONE;
                m_pdata->actionData.currentState = ActionState::IDLE;
                return;
            }

            uint8_t randomAttackMode         = rand() % 6 + 1; // 1-6 攻击方式
            m_pdata->actionData.currentState = ActionState::ATTACKING;

            switch (randomAttackMode) {
            case 1:
                //攻击模式1 - 随机位置发射大量普通子弹，血量越低，发射持续时间越长，基础时间为3秒，最多为6秒，每秒发射5发
                action_timer   = (uint16_t)(MassiveBasicBulletFireTime
                                          * float(
                                              (float)(m_pdata->healthData.maxHealth - m_pdata->healthData.currentHealth)
                                                  / (float)m_pdata->healthData.maxHealth
                                              + 1
                                          )); // 血量越低，攻击动作持续时间越长，基础时间为3000ms, 最多为6000ms
                action_MaxTime = action_timer;
                action_count   = 0;

                positionChange = false;

                m_pdata->actionData.attackMode = AttackMode::MODE_1;
                break;
            case 2:
                //攻击模式2 - 随机位置发射5个火球弹，持续时间3秒
                action_timer   = FiveFireballBulletFireTime; // 梼杌攻击动作持续时间3000ms
                action_MaxTime = action_timer;
                action_count   = 0;
                positionChange = false;

                m_pdata->actionData.attackMode = AttackMode::MODE_2;
                break;

            case 3:
                //攻击模式3 - 中间发射一颗火球弹，最边缘两侧发射各两颗普通子弹
                //1000ms
                action_timer   = CenterFireballAttackTime; // 梼杌攻击动作持续时间100ms
                action_MaxTime = action_timer;
                action_count   = 0;

                m_pdata->actionData.attackMode = AttackMode::MODE_3;
                break;
            case 4:
                //攻击模式4 - 发射一排特殊阵型的子弹，只有中间有缺口
                action_timer   = 1000; // 梼杌攻击动作持续时间1000ms
                action_MaxTime = action_timer;
                action_count   = 0;

                m_pdata->actionData.attackMode = AttackMode::MODE_4;
                break;
            case 5:
                //攻击模式5 - 随机闪现移动
                action_timer   = 1000; // 梼杌攻击动作持续时间1000ms
                action_MaxTime = action_timer;
                action_count   = 0;
                positionChange = false;

                m_pdata->actionData.attackMode = AttackMode::MODE_5;
                break;
            case 6:
                //攻击模式6 - 定向闪现，对齐玩家位置闪现
                action_timer   = 100; // 梼杌攻击动作持续时间100ms
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
        die();
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
        //动作作用时间，用来调节动作发生的频率
        action_count += controlDelayTime;

        //动作倒计时
        if (action_timer >= controlDelayTime)
            action_timer -= controlDelayTime;
        else
            action_timer = 0;

        switch (m_pdata->actionData.attackMode) {
        //执行攻击动作
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
            m_pdata->actionData.currentState       = ActionState::IDLE;
            m_pdata->actionData.attackMode         = AttackMode::NONE;
            action_count                           = 0;
            m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime; // 攻击后进入冷却时间

            //攻击模式6，清除CD
            if (m_pdata->actionData.attackMode == AttackMode::MODE_6) {
                m_pdata->attackData.shootCooldownTimer = 0; //清除冷却时间，快速攻击
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
        // 绘制死亡动画（例如一个简单的圆圈表示消失效果）
        uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
        uint8_t radius =
            (TaowuEnemyDeadTime - m_pdata->deathData.deathTimer) * 30 / TaowuEnemyDeadTime; // 从0增长到最大值5
        radius = etl::max(radius, uint8_t(1));                                              // 最小半径限制

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

//攻击技能
void TaowuEnemy::fireContinuousMassiveBasicBullets() {
    if (action_count < 1000 / BulletsPerSecond) // 每200ms发射一次
        return;
    action_count = 0;

    if (!positionChange) {
        //改变位置
        m_pdata->spatialData.currentPosX = 63;
        m_pdata->spatialData.currentPosY = 1;
        m_pdata->spatialData.refPosX     = 63;
        m_pdata->spatialData.refPosY     = 1;
        positionChange                   = true;
    }

    //发射大量普通子弹
    uint8_t m_x = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t m_y = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

    uint8_t offsetY = (rand() % 61) - 30; // -30 到 +30 的随机偏移

    //该BOSS无热量消耗，仅需重置冷却时间
    m_pdata->attackData.shootCooldownTimer = 0; //重置冷却时间，快速发射子弹
    shoot(m_x, m_y + offsetY, BulletType::BASIC);
}

//600ms 一发
void TaowuEnemy::fireFiveFireballBulletsAtRandom() {
    if (action_count < FiveFireballBulletFireTime / FireballCount) // 每600ms发射一次
        return;
    action_count = 0;

    if (!positionChange) {
        //改变位置
        m_pdata->spatialData.currentPosX = 63;
        m_pdata->spatialData.currentPosY = 1;
        m_pdata->spatialData.refPosX     = 63;
        m_pdata->spatialData.refPosY     = 1;
        positionChange                   = true;
    }

    //发射火球弹
    uint8_t m_x = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t m_y = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

    uint8_t offsetY = (rand() % 61) - 30; // -30 到 +30 的随机偏移

    m_pdata->attackData.shootCooldownTimer = 0; //重置冷却时间，快速发射子弹
    shoot(m_x, m_y + offsetY, BulletType::FIRE_BALL);
}

void TaowuEnemy::fireCenterFireballAndSideBasicBullets() {
    if (action_count < action_MaxTime - 10) // 1000ms 发射一次
        return;
    action_count = 0;
    //发射中间火球弹，侧边普通子弹
    uint8_t m_x = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t m_y = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
    //中间火球弹
    m_pdata->attackData.shootCooldownTimer = 0; //重置冷却时间，快速发射子弹
    shoot(m_x, m_y, BulletType::FIRE_BALL);
    //左侧普通子弹
    m_pdata->attackData.shootCooldownTimer = 0; //重置冷却时间，快速发射子弹
    shoot(m_x, m_y + 26, BulletType::BASIC);
    m_pdata->attackData.shootCooldownTimer = 0; //重置冷却时间，快速发射子弹
    shoot(m_x + 20, m_y + 30, BulletType::BASIC);
    //右侧普通子弹
    m_pdata->attackData.shootCooldownTimer = 0; //重置冷却时间，快速发射子弹
    shoot(m_x, m_y - 26, BulletType::BASIC);
    m_pdata->attackData.shootCooldownTimer = 0; //重置冷却时间，快速发射子弹
    shoot(m_x + 20, m_y - 30, BulletType::BASIC);
}

void TaowuEnemy::fireSingleRowNotchedBasicBullets() {
    if (action_count < action_MaxTime - 10) // 1000ms 发射一次
        return;
    action_count = 0;
    //发射一排特殊阵型的子弹，只有中间有缺口
    uint8_t m_x = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t m_y = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

    //发射子弹，间隔6个像素，跳过中间位置
    for (int8_t offsetY = -30; offsetY <= 30; offsetY += 6) {
        if (offsetY >= -12 && offsetY <= 12) {
            // 中间缺口，跳过发射
            continue;
        }
        m_pdata->attackData.shootCooldownTimer = 0; //重置冷却时间，快速发射子弹
        shoot(m_x, m_y + offsetY, BulletType::BASIC);
    }
}

void TaowuEnemy::blinkToRandomPosition() {
    if (action_count < 200) return;
    action_count = 0;

    if (!positionChange) {
        //改变位置
        m_pdata->spatialData.currentPosX = 140; // 40-80 随机位置
        m_pdata->spatialData.currentPosY = 1;   // 0-32 随机位置
        m_pdata->spatialData.refPosX     = m_pdata->spatialData.currentPosX;
        m_pdata->spatialData.refPosY     = m_pdata->spatialData.currentPosY;
        positionChange                   = true;
    } else {
        if (action_timer < 300) {
            //恢复原位置
            m_pdata->spatialData.currentPosX = 63;
            m_pdata->spatialData.currentPosY = 1;
            m_pdata->spatialData.refPosX     = 63;
            m_pdata->spatialData.refPosY     = 1;
        } else {
            //发射火球弹
            uint8_t m_x                            = 119;
            uint8_t m_y                            = rand() % 60 + 1;
            m_pdata->attackData.shootCooldownTimer = 0; //重置冷却时间，快速发射子弹
            shoot(m_x, m_y, BulletType::FIRE_BALL);
        }
    }
}

void TaowuEnemy::blinkToPlayerAlignedPosition() {
    if (action_count < action_MaxTime - 10) // 闪现一次
        return;
    action_count = 0;

    IRole *player = g_entityManager.getPlayerRole();
    if (player == nullptr) return;

    if (!positionChange) {
        //改变位置，对齐玩家位置
        uint8_t playerY = player->getData()->spatialData.currentPosY + player->getData()->spatialData.sizeY / 2;
        int8_t  targetY = playerY - m_pdata->spatialData.sizeY / 2;

        //X位置随机
        m_pdata->spatialData.currentPosX = 30 + (rand() % 51); // 30-80 随机位置

        //确保BOSS位置在屏幕内
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

XiangliuEnemy::XiangliuEnemy(uint8_t startX, uint8_t startY, uint8_t initPosX, uint8_t initPosY, uint8_t level , uint16_t dropExp)
: IRole() {
    //图片信息
    m_pdata->img = &XiangliuImg;

    //身份信息
    m_pdata->identity          = RoleIdentity::ENEMY;
    m_pdata->isActive          = true;
    m_pdata->initData.isInited = false;

    //等级信息
    m_pdata->level = level;

    //血量信息
    //血量较低，但攻击力高，移动速度快
    m_pdata->healthData.currentHealth = 830 + level * 400;
    m_pdata->healthData.maxHealth     = 830 + level * 400;

    //回血信息
    m_pdata->healthData.healValue       = 30;
    m_pdata->healthData.healTimeCounter = 0;
    m_pdata->healthData.healResetTime   = 15000;
    m_pdata->healthData.healSpeed       = 5;

    //空间移动信息
    m_pdata->spatialData.canCrossBorder            = true;
    m_pdata->spatialData.currentPosX               = startX; // Starting X position
    m_pdata->spatialData.currentPosY               = startY; // Starting Y position
    m_pdata->spatialData.refPosX                   = startX;
    m_pdata->spatialData.refPosY                   = startY;
    m_pdata->spatialData.sizeX                     = m_pdata->img->w;
    m_pdata->spatialData.sizeY                     = m_pdata->img->h;
    m_pdata->spatialData.moveSpeed                 = 2; // Set movement speed
    m_pdata->spatialData.consecutiveCollisionCount = 0;

    //初始化位置
    m_pdata->initData.posX = initPosX;
    m_pdata->initData.posY = initPosY;

    //攻击信息
    m_pdata->attackData.attackPower            = 7 + level * 3;
    m_pdata->attackData.shootCooldownSpeed     = 5;
    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 5000; //5000 ms
    m_pdata->attackData.bulletSpeed            = 1;
    //攻击速度 15000 ms

    m_pdata->attackData.bulletRange            = 10;   //只对火球弹生效
    m_pdata->attackData.bulletDamageMultiplier = 1.5f; //只对闪电链弹生效

    m_pdata->attackData.collisionPower = 12 + level * 4;

    //热量信息
    m_pdata->heatData.maxHeat          = 250;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 0;
    m_pdata->heatData.heatCoolDownRate = 10; //每次冷却10点热量，每次冷却时间间隔由200ms

    //死亡状态信息
    m_pdata->deathData.deathTimer = XiangliuEnemyDeadTime;
    m_pdata->deathData.isDead     = false;
    m_pdata->deathData.dropExperiencePoints = dropExp;

    // Initialize other enemy-specific data here
}

void XiangliuEnemy::shoot(uint8_t x, uint8_t y, BulletType type) {
    // Implement enemy shooting logic
    // Create bullet based on type
    switch (type) {
    case BulletType::BASIC:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot > m_pdata->heatData.maxHeat)
                return;                                             // 超过最大热量，无法射击
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // 冷却中，无法射击

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
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 2 > m_pdata->heatData.maxHeat)
                return;                                             // 超过最大热量，无法射击
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // 冷却中，无法射击

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
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 1.5 > m_pdata->heatData.maxHeat)
                return;                                             // 超过最大热量，无法射击
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // 冷却中，无法射击

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
    if (think_count < 100) // 每100ms决定一次行动
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
                // 如果在冷却中，则不进行攻击，保持空闲状态
                m_pdata->actionData.moveMode     = MoveMode::NONE;
                m_pdata->actionData.currentState = ActionState::IDLE;
                return;
            }

            uint8_t randomAttackMode = rand() % 6 + 1; // 1-6 攻击方式
            if (randomAttackMode > 3 && g_entityManager.m_roles.size() >= 4 )
                randomAttackMode -= 3; // 如果场上敌人过多，则减少召唤物攻击方式的概率

            m_pdata->actionData.currentState = ActionState::ATTACKING;

            switch (randomAttackMode) {
            case 1:
                action_count   = 0;
                action_timer   = 500; // 相柳攻击动作持续时间500ms
                action_MaxTime = action_timer;

                m_pdata->actionData.attackMode = AttackMode::MODE_1;
                break;
            case 2:
                action_count   = 0;
                action_timer   = 500; // 相柳攻击动作持续时间500ms
                action_MaxTime = action_timer;

                m_pdata->actionData.attackMode = AttackMode::MODE_2;
                break;

            case 3:
                action_count   = 0;
                action_timer   = 500; // 相柳攻击动作持续时间500ms
                action_MaxTime = action_timer;

                m_pdata->actionData.attackMode = AttackMode::MODE_3;
                break;
            case 4:
                action_count   = 0;
                action_timer   = 500; // 相柳攻击动作持续时间500ms
                action_MaxTime = action_timer;

                m_pdata->actionData.attackMode = AttackMode::MODE_4;
                break;
            case 5:
                action_count   = 0;
                action_timer   = 500; // 相柳攻击动作持续时间500ms
                action_MaxTime = action_timer;

                m_pdata->actionData.attackMode = AttackMode::MODE_5;
                break;
            case 6:
                action_count   = 0;
                action_timer   = 500; // 相柳攻击动作持续时间500ms
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
        die();
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
        //动作作用时间，用来调节动作发生的频率
        action_count += controlDelayTime;

        //动作倒计时
        if (action_timer >= controlDelayTime)
            action_timer -= controlDelayTime;
        else
            action_timer = 0;

        switch (m_pdata->actionData.attackMode) {
        //执行攻击动作
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
            m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime; // 攻击后进入冷却时间
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
        // 绘制死亡动画（例如一个简单的圆圈表示消失效果）
        uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
        uint8_t radius =
            (XiangliuEnemyDeadTime - m_pdata->deathData.deathTimer) * 30 / XiangliuEnemyDeadTime; // 从0增长到最大值5
        radius = etl::max(radius, uint8_t(1));                                                    // 最小半径限制

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

//攻击技能
void XiangliuEnemy::fireNineRowsBasicBullets() {
    //发射九排普通子弹
    if (action_count < action_MaxTime - 10) // 发射一次
        return;
    action_count = 0;

    uint8_t m_x = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t m_y = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
    //发射子弹，间隔2个像素，中间留有空隙
    int8_t offsetYList[9] = {-31,-29, -27, -2,0, 2, 27, 29,31};
    for (uint8_t i = 0; i < 9; i++) {
        m_pdata->attackData.shootCooldownTimer = 0; //重置冷却时间，快速发射子弹
        shoot(m_x, m_y + offsetYList[i], BulletType::BASIC);
        m_pdata->attackData.shootCooldownTimer = 0; //重置冷却时间，快速发射子弹
        shoot(m_x + 18, m_y + offsetYList[i], BulletType::BASIC);
    }
}

void XiangliuEnemy::fireThreeRowsLightningBullets() {
    if (action_count < action_MaxTime - 10) // 发射一次
        return;
    action_count = 0;

    //调整位置
    m_pdata->spatialData.currentPosX = 64 + 14;
    m_pdata->spatialData.refPosX     = 64+ 14;
    m_pdata->spatialData.currentPosY = 1;
    m_pdata->spatialData.refPosY     = 1;

    //发射三排闪电
    
    uint8_t m_y = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
    //发射子弹，间隔21个像素
    int8_t offsetYList[3] = {-27, 0, 27};
    for (uint8_t i = 0; i < 3; i++) {
        m_pdata->attackData.shootCooldownTimer = 0; //重置冷却时间，快速发射子弹
        shoot(1, m_y + offsetYList[i], BulletType::LIGHTNING_LINE);
    }
}

void XiangliuEnemy::fireThreeRowsFireballBullets() {
    if (action_count < action_MaxTime - 10) // 发射一次
        return;
    action_count = 0;
    //发射三排火球弹
    uint8_t m_x = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t m_y = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
    //发射子弹，间隔21个像素
    int8_t offsetYList[3] = {-21, 0, 21};
    for (uint8_t i = 0; i < 3; i++) {
        m_pdata->attackData.shootCooldownTimer = 0; //重置冷却时间，快速发射子弹
        shoot(m_x, m_y + offsetYList[i], BulletType::FIRE_BALL);
    }
}

void XiangliuEnemy::summonThreeChiMeiMinions() {
    if (action_count < action_MaxTime - 10) // 召唤一次
        return;
    action_count = 0;

    uint8_t posX[3] = {10, 30, 50};
    uint8_t posY[3] = {1, 32, 50};

    for (uint8_t i = 0; i < 3; i++) {
        IRole *minion = new ChiMeiEnemy(posX[i], posY[i], posX[i], posY[i], rand() % 3 + 1);
        if (minion != nullptr) {
            if (!g_entityManager.addRole(minion)) delete minion; // Clean up if not added
        }
    }
}

void XiangliuEnemy::summonTwoFeilianMinions() {
    if (action_count < action_MaxTime - 10) // 召唤一次
        return;
    action_count = 0;

    uint8_t posX[2] = {40, 40};
    uint8_t posY[2] = {0, 50};
    for (uint8_t i = 0; i < 2; i++) {
        IRole *minion = new FeilianEnemy(posX[i], posY[i], posX[i], posY[i], rand() % 3 + 1);
        if (minion != nullptr) {
            if (!g_entityManager.addRole(minion)) delete minion; // Clean up if not added
        }
    }
}

void XiangliuEnemy::summonOneGudiaoMinion() {
    if (action_count < action_MaxTime - 10) // 召唤一次
        return;
    action_count = 0;

    uint8_t posX = 40;
    uint8_t posY = 25;

    IRole *minion = new GudiaoEnemy(posX, posY, posX, posY, rand() % 3 + 1);
    if (minion != nullptr) {
        if (!g_entityManager.addRole(minion)) delete minion; // Clean up if not added
    }
}

/*******************************************************************/
