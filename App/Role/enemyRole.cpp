#include "enemyRole.hpp"
#include "bullet.hpp"  

#include "../gameEntityManager.hpp"
extern GameEntityManager g_entityManager ;

/**
 * @brief FeilianEnemy class
 * @note  中文：飞廉 ｜ 英文：Feilian,神话典故：中国古代神话中的风神，形如鹿、头生角、有翼，行走如飞，负责掌管八面来风。
 * @note  高速移动的小型敌人，体型小巧（12x12 像素），移动轨迹飘忽（呼应 “风” 的特性），单次攻击伤害低，但成群出现时压迫感强。
 */

void FeilianEnemy::shoot() {
        // Implement enemy shooting logic
        // 创建一个新的子弹实例
        if( m_pdata->attackData.shootCooldownTimer == 0 && m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot <= m_pdata->heatData.maxHeat ) {

            IBullet* newBullet = new BasicBullet(
                m_pdata->attackData.bulletSpeed,
                m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2,
                m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2,
                1 , // Range
                m_pdata->attackData.attackPower ,
                m_pdata->identity
            );

            // 将子弹添加到实体管理器中
            if(!g_entityManager.addBullet(newBullet)) {
                delete newBullet; // 添加失败则删除子弹实例以防内存泄漏
            } else {
                // 重置射击冷却计时器
            m_pdata->attackData.shootCooldownTimer += m_pdata->attackData.shootCooldownResetTime ;
            // 增加热量
            m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot;
            }
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
                m_pdata->actionData.attackMode    = AttackMode::SINGLE_SHOT;
                m_pdata->actionData.currentState  = ActionState::ATTACKING;
            }
        }

        action_count = 0;
    }

    void FeilianEnemy::doAction() {
        // Implement enemy action logic
        if(m_pdata->isDead) {
            die();
            return ;
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
            m_pdata->actionData.moveMode = MoveMode::NONE;
            // Move logic handled in think()
            break;
        case ActionState::ATTACKING:
            switch (m_pdata->actionData.attackMode) {
            case AttackMode::SINGLE_SHOT:
                shoot();
                break;
            case AttackMode::BURST_FIRE:
                // Implement burst fire logic
                shoot();
                break;
            case AttackMode::CONTINUOUS_FIRE:
                // Implement continuous fire logic
                shoot();
                break;
            default:
                break;
            }
            m_pdata->actionData.currentState = ActionState::IDLE;
            m_pdata->actionData.attackMode = AttackMode::NONE;
            break;
        }
    }

    void FeilianEnemy::die() {
        m_pdata->isActive = false;
    }
