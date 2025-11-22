#include "enemyRole.hpp"
#include "bullet.hpp"

#include "etl/algorithm.h"
#include "../Peripheral/OLED/oled.h"
#include "../gameEntityManager.hpp"
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

//闪电链弹一束条的范围穿透伤害，1.5*attackPower+10 点伤害

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

FeilianEnemy::FeilianEnemy(uint8_t startX, uint8_t startY, uint8_t initPosX, uint8_t initPosY, uint8_t level)
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
    m_pdata->healthData.currentHealth = 30 + level * 10;
    m_pdata->healthData.maxHealth     = 30 + level * 10;

    //回血信息
    m_pdata->healthData.healValue       = 0;
    m_pdata->healthData.healTimeCounter = 0;
    m_pdata->healthData.healResetTime   = 15000;
    m_pdata->healthData.healSpeed       = 0;

    //空间移动信息
    m_pdata->spatialData.canCrossBorder = false;
    m_pdata->spatialData.currentPosX    = startX; // Starting X position
    m_pdata->spatialData.currentPosY    = startY; // Starting Y position
    m_pdata->spatialData.refPosX        = startX;
    m_pdata->spatialData.refPosY        = startY;
    m_pdata->spatialData.sizeX          = m_pdata->img->w;
    m_pdata->spatialData.sizeY          = m_pdata->img->h;
    m_pdata->spatialData.moveSpeed      = 3; // Set movement speed
    m_pdata->spatialData.consecutiveCollisionCount = 0;

    //初始化位置
    m_pdata->initData.posX = initPosX;
    m_pdata->initData.posY = initPosY;

    //攻击信息
    m_pdata->attackData.attackPower            = 1 + level * 1;
    m_pdata->attackData.shootCooldownSpeed     = 5;
    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 4000;
    m_pdata->attackData.bulletSpeed            = 1;

    m_pdata->attackData.bulletRange = 0; //只对火球弹生效

    m_pdata->attackData.collisionPower = 10;

    //热量信息
    m_pdata->heatData.maxHeat          = 100;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 20;
    m_pdata->heatData.heatCoolDownRate = 10; //每次冷却10点热量，每次冷却时间间隔由200ms

    //死亡状态信息
    m_pdata->deathData.deathTimer = feilianEnemyDeadTime;
    m_pdata->deathData.isDead     = false;

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

    think_count = 0;
}

void FeilianEnemy::doAction() {
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
        uint8_t m_x = m_pdata->spatialData.currentPosX+m_pdata->spatialData.sizeX/2 ;
        uint8_t m_y = m_pdata->spatialData.currentPosY+m_pdata->spatialData.sizeY/2 ;
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
GudiaoEnemy::GudiaoEnemy(uint8_t startX, uint8_t startY, uint8_t initPosX, uint8_t initPosY, uint8_t level)
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
    m_pdata->healthData.currentHealth = 230 + level * 20;
    m_pdata->healthData.maxHealth     = 230 + level * 20;

    //回血信息
    m_pdata->healthData.healValue       = 20;
    m_pdata->healthData.healTimeCounter = 0;
    m_pdata->healthData.healResetTime   = 15000;
    m_pdata->healthData.healSpeed       = 5;

    //空间移动信息
    m_pdata->spatialData.canCrossBorder = false;
    m_pdata->spatialData.currentPosX    = startX; // Starting X position
    m_pdata->spatialData.currentPosY    = startY; // Starting Y position
    m_pdata->spatialData.refPosX        = startX;
    m_pdata->spatialData.refPosY        = startY;
    m_pdata->spatialData.sizeX          = m_pdata->img->w;
    m_pdata->spatialData.sizeY          = m_pdata->img->h;
    m_pdata->spatialData.moveSpeed      = 1; // Set movement speed
    m_pdata->spatialData.consecutiveCollisionCount = 0;

    //初始化位置
    m_pdata->initData.posX = initPosX;
    m_pdata->initData.posY = initPosY;

    //攻击信息
    m_pdata->attackData.attackPower            = 10 + level * 2;
    m_pdata->attackData.shootCooldownSpeed     = 5;
    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 16000; //32000 ms
    m_pdata->attackData.bulletSpeed            = 1;

    m_pdata->attackData.bulletRange = 10; //只对火球弹生效

    m_pdata->attackData.collisionPower = 20;

    //热量信息
    m_pdata->heatData.maxHeat          = 150;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 20;
    m_pdata->heatData.heatCoolDownRate = 10; //每次冷却10点热量，每次冷却时间间隔由200ms

    //死亡状态信息
    m_pdata->deathData.deathTimer = gudiaoEnemyDeadTime;
    m_pdata->deathData.isDead     = false;

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

    uint8_t randomAction = rand() % 6 ;
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
            if( m_pdata->attackData.shootCooldownTimer <= 0 ) {
                m_pdata->actionData.attackMode   = AttackMode::MODE_1;
                m_pdata->actionData.currentState = ActionState::ATTACKING;
            }
            else {
                // 如果在冷却中，则不进行攻击，保持空闲状态
                m_pdata->actionData.moveMode     = MoveMode::NONE;
                m_pdata->actionData.currentState = ActionState::IDLE;
            }
        }
    }

    think_count = 0;
}

void GudiaoEnemy::doAction() {
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
    IBullet* newBullet = createBullet(
        m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2,
        m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2,
        BulletType::FIRE_BALL
    );
    if (newBullet != nullptr) {
        if( !g_entityManager.addBullet(newBullet) ) {
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

ChiMeiEnemy::ChiMeiEnemy(uint8_t startX, uint8_t startY, uint8_t initPosX, uint8_t initPosY, uint8_t level)
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
    m_pdata->spatialData.canCrossBorder = false;
    m_pdata->spatialData.currentPosX    = startX; // Starting X position
    m_pdata->spatialData.currentPosY    = startY; // Starting Y position
    m_pdata->spatialData.refPosX        = startX;
    m_pdata->spatialData.refPosY        = startY;
    m_pdata->spatialData.sizeX          = m_pdata->img->w;
    m_pdata->spatialData.sizeY          = m_pdata->img->h;
    m_pdata->spatialData.moveSpeed      = 3; // Set movement speed
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

    m_pdata->attackData.bulletRange = 0; //只对火球弹生效

    m_pdata->attackData.collisionPower = 20;

    //热量信息
    m_pdata->heatData.maxHeat          = 100;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 20;
    m_pdata->heatData.heatCoolDownRate = 10; //每次冷却10点热量，每次冷却时间间隔由200ms

    //死亡状态信息
    m_pdata->deathData.deathTimer = chimeiEnemyDeadTime;
    m_pdata->deathData.isDead     = false;

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
    if (m_pdata->actionData.currentState == ActionState::IDLE) {
        //移动
        m_pdata->actionData.moveMode     = MoveMode::LEFT;
        m_pdata->actionData.currentState = ActionState::MOVING;
    }

    think_count = 0;
}

void ChiMeiEnemy::doAction() {
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
        uint8_t m_x_1 = m_pdata->spatialData.currentPosX;
        uint8_t m_y_1 = m_pdata->spatialData.currentPosY;
        uint8_t m_x_2 = m_x_1 + m_pdata->spatialData.sizeX;
        uint8_t m_y_2 = m_y_1 + m_pdata->spatialData.sizeY;
        switch (m_pdata->actionData.attackMode) {
        case AttackMode::MODE_1:
            shoot(m_x_1, m_y_1, BulletType::BASIC);
            shoot(m_x_2, m_y_1, BulletType::BASIC);
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
 * @note  BOSS级大型敌人，体型巨大（64x64 像素），高血量，高攻击力，中速移动，攻击方式多样且具有威胁性。
 * @note  攻击方式1，将玩家向自己拉近，进行吞噬攻击 
 * @note  攻击方式2，发射三排普通子弹
 * @note  攻击方式3，向前冲撞，进行撞击攻击
 * @note  攻击方式4, 向后碾压，从玩家左侧出现，进行碾压攻击
 * @note  攻击方式5, 将玩家向自己拉近，同时向前冲撞
 */

TaotieEnemy::TaotieEnemy(uint8_t startX, uint8_t startY, uint8_t initPosX, uint8_t initPosY, uint8_t level)
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
    m_pdata->healthData.currentHealth = 1230 + level * 200;
    m_pdata->healthData.maxHealth     = 1230 + level * 200;

    //回血信息
    m_pdata->healthData.healValue       = 20;
    m_pdata->healthData.healTimeCounter = 0;
    m_pdata->healthData.healResetTime   = 15000;
    m_pdata->healthData.healSpeed       = 5;

    //空间移动信息
    m_pdata->spatialData.canCrossBorder = true;
    m_pdata->spatialData.currentPosX    = startX; // Starting X position
    m_pdata->spatialData.currentPosY    = startY; // Starting Y position
    m_pdata->spatialData.refPosX        = startX;
    m_pdata->spatialData.refPosY        = startY;
    m_pdata->spatialData.sizeX          = m_pdata->img->w;
    m_pdata->spatialData.sizeY          = m_pdata->img->h;
    m_pdata->spatialData.moveSpeed      = 1; // Set movement speed
    m_pdata->spatialData.consecutiveCollisionCount = 0;

    //初始化位置
    m_pdata->initData.posX = initPosX;
    m_pdata->initData.posY = initPosY;

    //攻击信息
    m_pdata->attackData.attackPower            = 10 + level * 5;
    m_pdata->attackData.shootCooldownSpeed     = 5;
    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 16000; //32000 ms
    m_pdata->attackData.bulletSpeed            = 1;

    m_pdata->attackData.bulletRange = 10; //只对火球弹生效

    m_pdata->attackData.collisionPower = 40;

    //热量信息
    m_pdata->heatData.maxHeat          = 200;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 20;
    m_pdata->heatData.heatCoolDownRate = 10; //每次冷却10点热量，每次冷却时间间隔由200ms

    //死亡状态信息
    m_pdata->deathData.deathTimer = TaotieEnemyDeadTime;
    m_pdata->deathData.isDead     = false;

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

    uint8_t randomAction = rand() % 6 ;
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
            if( m_pdata->attackData.shootCooldownTimer <= 0 ) {
                m_pdata->actionData.attackMode   = AttackMode::MODE_1;
                m_pdata->actionData.currentState = ActionState::ATTACKING;
            }
            else {
                // 如果在冷却中，则不进行攻击，保持空闲状态
                m_pdata->actionData.moveMode     = MoveMode::NONE;
                m_pdata->actionData.currentState = ActionState::IDLE;
            }
        }
    }

    think_count = 0;
}

void TaotieEnemy::doAction() {
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
        uint8_t radius  = (TaotieEnemyDeadTime - m_pdata->deathData.deathTimer) * 30 / TaotieEnemyDeadTime ; // 从0增长到最大值5
        radius          = etl::max(radius, uint8_t(1));                                // 最小半径限制

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

/*******************************************************************/

/*******************************************************************/
/**
 * @brief TaowuEnemy class
 * @note  中文：饕餮 ｜ 英文：Taowu,神话典故：四凶之一，虎形犬毛、人面猪口、尾长一丈八尺；
 * @note  上古 “四凶” 之一，性格顽劣不可教化，在荒野中搅乱秩序、捕食人类，代表凶暴与叛逆。
 * @note  BOSS级大型敌人，体型巨大（64x64 像素），高血量，高攻击力，中速移动，攻击方式多样且具有威胁性。
 * @note  攻击方式1，随机位置发射大量普通子弹
 * @note  攻击方式2，随机位置发射5个火球弹
 * @note  攻击方式3，闪现移动
 
 */

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

/*******************************************************************/
