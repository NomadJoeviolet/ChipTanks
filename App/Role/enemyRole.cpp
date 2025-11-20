#include "enemyRole.hpp"
#include "bullet.hpp"

#include "../gameEntityManager.hpp"
extern GameEntityManager g_entityManager;

//数值规范
//基础血量: 30（低血量），100（中血量），200（高血量）
//基础攻击力: 1-5 （低攻击），5-15（中攻击），15+（高攻击）
//基础移动速度: 1（低速），2（中速），3（高速）


//controlDelayTime 由 threads.cpp 控制线程定义
//controlDelayTime = 10 
//设计冷却和热量机制查看role.cpp
//冷却时间=resetTime/ (Speed) ms

/**
 * @brief FeilianEnemy class
 * @note  中文：飞廉 ｜ 英文：Feilian,神话典故：中国古代神话中的风神，形如鹿、头生角、有翼，行走如飞，负责掌管八面来风。
 * @note  高速移动的小型敌人，体型小巧（12x12 像素），移动轨迹飘忽（呼应 “风” 的特性），单次攻击伤害低，但成群出现时压迫感强。
 */

//数值设定参考
//血量：30 + level * 1
//攻击力：1 + level * 1
//移动速度：3（高速）
//射击冷却时间：4000/5 ms
//热量机制：每次射击增加20点热量，最大热量100点，冷却速率10点/200ms

FeilianEnemy::FeilianEnemy(uint8_t startX, uint8_t startY , uint8_t initPosX , uint8_t initPosY , uint8_t level ): IRole() {

    //图片信息
    m_pdata->img      = &feilianImg;

    //身份信息
    m_pdata->identity = RoleIdentity::ENEMY;
    m_pdata->isActive = true;
    m_pdata->isInited = false ;
    m_pdata->isDead   = false ;

    //血量信息
    m_pdata->level         = level;
    m_pdata->healthData.currentHealth = 30+level*1 ;
    m_pdata->healthData.maxHealth     = 30+level*1 ;
    //回血
    m_pdata->healthData.healValue      = 0 ;
    m_pdata->healthData.healTimeCounter= 0 ;
    m_pdata->healthData.healResetTime  = 15000 ;
    m_pdata->healthData.healSpeed      = 0 ;

    //空间移动信息
    m_pdata->spatialData.currentPosX = startX; // Starting X position
    m_pdata->spatialData.currentPosY = startY; // Starting Y position
    m_pdata->spatialData.refPosX = startX;
    m_pdata->spatialData.refPosY = startY;
    m_pdata->spatialData.sizeX = m_pdata->img->w;
    m_pdata->spatialData.sizeY = m_pdata->img->h;
    m_pdata->spatialData.moveSpeed = 3 ; // Set mo1ement speed

    //初始化位置
    m_pdata->initData.posX           = initPosX;
    m_pdata->initData.posY           = initPosY;

    //攻击信息
    m_pdata->attackData.attackPower            = 0+level*1 ;
    m_pdata->attackData.shootCooldownSpeed     = 5;
    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 4000 ;
    m_pdata->attackData.bulletSpeed            = 1;

    //热量信息
    m_pdata->heatData.maxHeat          = 100;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 20;
    m_pdata->heatData.heatCoolDownRate = 10;//每次冷却10点热量，每次冷却时间间隔由200ms

    // Initialize other enemy-specific data here
}

void FeilianEnemy::shoot(uint8_t x , uint8_t y , BulletType type) {
    // Implement enemy shooting logic
    // 创建一个新的子弹实例
    IBullet* newBullet = createBullet(x, y, type);
    if (newBullet == nullptr) {
        return; // 无法创建子弹，可能是因为冷却时间未到或热量过高
    }
    // 将子弹添加到实体管理器中
    if (!g_entityManager.addBullet(newBullet)) {
        delete newBullet; // 添加失败则删除子弹实例以防内存泄漏
    } else {
        // 重置射击冷却计时器
        m_pdata->attackData.shootCooldownTimer += m_pdata->attackData.shootCooldownResetTime;
        // 增加热量
        m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot;
    }
}

void FeilianEnemy::init() {
    m_pdata->initData.init_count += controlDelayTime;
    // Initialize enemy role specifics

    if (m_pdata->spatialData.currentPosX > m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 100) { // 每100ms移动一次
            m_pdata->spatialData.currentPosX -= m_pdata->spatialData.moveSpeed;
            m_pdata->initData.init_count = 0;
        }
    } else {
        m_pdata->isInited            = true;
        m_pdata->spatialData.refPosX = m_pdata->spatialData.currentPosX;
        m_pdata->spatialData.refPosY = m_pdata->spatialData.currentPosY;
        m_pdata->initData.init_count = 0;
    }
}

void FeilianEnemy::think() {
    // Implement enemy AI logic
    action_count += controlDelayTime;
    if (action_count < 100) // 每100ms决定一次行动
        return;

    uint8_t randomAction = rand() % 6;
    // Random action: 0 - move left, 1 - move right, 2 - move down, 3 - move up, 4 - stay still, 5 - shoot
    if (m_pdata->actionData.currentState == ActionState::IDLE) {
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
        } else if (randomAction == 5) {
            m_pdata->actionData.attackMode   = AttackMode::SINGLE_SHOT;
            m_pdata->actionData.currentState = ActionState::ATTACKING;
        }
    }

    action_count = 0;
}

void FeilianEnemy::doAction() {
    // Implement enemy action logic
    if (m_pdata->isDead) {
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
        case AttackMode::SINGLE_SHOT:
            shoot(m_x, m_y, BulletType::BASIC );
            break;
        case AttackMode::BURST_FIRE:
            // Implement burst fire logic
            shoot(m_x, m_y, BulletType::BASIC );
            break;
        case AttackMode::CONTINUOUS_FIRE:
            // Implement continuous fire logic
            shoot(m_x, m_y, BulletType::BASIC );
            break;
        default:
            break;
        }
        m_pdata->actionData.currentState = ActionState::IDLE;
        m_pdata->actionData.attackMode   = AttackMode::NONE;
        break;
    }
}

void FeilianEnemy::die() {
    m_pdata->isActive = false;
}
