#include "role.hpp"

//一定要注意这里的子弹创建函数的实现和细节
//此处决定发射子弹的数值和类型

IBullet *IRole::createBullet(uint8_t x, uint8_t y, BulletType type) {
    // Implement bullet creation logic
    switch (type) {
    case BulletType::BASIC:
        {
            IBullet *newBullet = new BasicBullet(
                m_pdata->attackData.bulletSpeed, x, y,
                0, // Range
                m_pdata->attackData.attackPower, m_pdata->identity
            );
            return newBullet;
        }
        break;
    case BulletType::FIRE_BALL:
        {
            IBullet *newBullet = new FireBallBullet(
                m_pdata->attackData.bulletSpeed, x, y,
                m_pdata->attackData.bulletRange,     // Range
                m_pdata->attackData.attackPower+10 , // Fire Ball does more damage
                m_pdata->identity
            );
            return newBullet;
        }
        break;
    case BulletType::LIGHTNING_LINE:
        {
            // Implement Lightning Line bullet creation
            IBullet *newBullet = new LightningLineBullet(
                0 , x, y,
                0,                                    // Range
                m_pdata->attackData.attackPower *1.5 +10 , // Lightning Line does even more damage
                m_pdata->identity
            );
            return newBullet;
        }
        break;
    default:
        return nullptr;
    }
}

void IRole::move(int8_t dirX, int8_t dirY) {
    // Implement enemy movement logic
    uint8_t tmpX = m_pdata->spatialData.currentPosX + dirX * m_pdata->spatialData.moveSpeed;
    uint8_t tmpY = m_pdata->spatialData.currentPosY + dirY * m_pdata->spatialData.moveSpeed;
    // Boundary checks (assuming screen size 128x64)
    if (!m_pdata->spatialData.canCrossBorder) {
        if (tmpX < 0 || tmpX > 128 - m_pdata->spatialData.sizeX) return;
        if (tmpY < 0 || tmpY > 64 - m_pdata->spatialData.sizeY) return;
    }
    if(m_pdata->spatialData.canCrossBorder) {
        // Wrap around logic
       if(tmpX < -64 || tmpX > 192 ) return ;
       if(tmpY < -32 || tmpY > 56 ) return ;
    }

    m_pdata->spatialData.refPosX = tmpX;
    m_pdata->spatialData.refPosY = tmpY;
}

void IRole::update(CollisionResult collisionResult) {
    //初始化
    if (!m_pdata->initData.isInited) {
        //首次初始化
        init();
        return;
    }

    if (m_pdata->deathData.isDead) {
        die();
        return;
    }

    //位置更新
    if (!collisionResult.isCollision) {
        m_pdata->spatialData.currentPosX = m_pdata->spatialData.refPosX;
        m_pdata->spatialData.currentPosY = m_pdata->spatialData.refPosY;
    } else {
        m_pdata->spatialData.refPosX = m_pdata->spatialData.currentPosX;
        m_pdata->spatialData.refPosY = m_pdata->spatialData.currentPosY;
    }

    //碰撞处理
    //碰撞处理在位置更新后进行
    if (collisionResult.isCollision) {
        //用来避免单次碰撞一次退回的无法解决碰撞的问题
        m_pdata->spatialData.consecutiveCollisionCount++;
        //碰撞后退回
        uint8_t backStep = 2*m_pdata->spatialData.consecutiveCollisionCount ;
        if (m_pdata->identity == RoleIdentity::Player) backStep = 5*m_pdata->spatialData.consecutiveCollisionCount ;
        if (collisionResult.direction == CollisionDirection::LEFT)
            m_pdata->spatialData.refPosX = etl::max(m_pdata->spatialData.refPosX - backStep, 0);
        else if (collisionResult.direction == CollisionDirection::RIGHT)
            m_pdata->spatialData.refPosX =
                etl::min(m_pdata->spatialData.refPosX + backStep, 128 - m_pdata->spatialData.sizeX);
        else if (collisionResult.direction == CollisionDirection::UP)
            m_pdata->spatialData.refPosY = etl::max(m_pdata->spatialData.refPosY - backStep, 0);
        else if (collisionResult.direction == CollisionDirection::DOWN)
            m_pdata->spatialData.refPosY =
                etl::min(m_pdata->spatialData.refPosY + backStep, 64 - m_pdata->spatialData.sizeY);
    } else {
        m_pdata->spatialData.consecutiveCollisionCount = 0;
    }

    //射击冷却处理
    if (m_pdata->attackData.shootCooldownTimer > 0) {
        uint8_t shootCooldownSpeed = m_pdata->attackData.shootCooldownSpeed;
        if (controlDelayTime * shootCooldownSpeed >= m_pdata->attackData.shootCooldownTimer)
            m_pdata->attackData.shootCooldownTimer = 0;
        else
            m_pdata->attackData.shootCooldownTimer -= controlDelayTime * shootCooldownSpeed;
    }

    //热量冷却处理
    if (m_pdata->heatData.currentHeat > 0) {
        m_pdata->heatData.heatCoolDownTimer += controlDelayTime;
        if (m_pdata->heatData.heatCoolDownTimer >= 200) //每200ms降温一次
        {
            if (m_pdata->heatData.currentHeat <= m_pdata->heatData.heatCoolDownRate)
                m_pdata->heatData.currentHeat = 0;
            else
                m_pdata->heatData.currentHeat -= m_pdata->heatData.heatCoolDownRate;
            m_pdata->heatData.heatCoolDownTimer = 0;
        }
    }

    //回血处理
    if (m_pdata->healthData.currentHealth < m_pdata->healthData.maxHealth) {
        m_pdata->healthData.healTimeCounter += controlDelayTime * m_pdata->healthData.healSpeed;
        if (m_pdata->healthData.healTimeCounter >= m_pdata->healthData.healResetTime) {
            if (m_pdata->healthData.currentHealth + m_pdata->healthData.healValue >= m_pdata->healthData.maxHealth)
                m_pdata->healthData.currentHealth = m_pdata->healthData.maxHealth;
            else
                m_pdata->healthData.currentHealth += m_pdata->healthData.healValue;
            m_pdata->healthData.healTimeCounter = 0;
        }
    }

    //检查血量状态
    if (m_pdata->healthData.currentHealth == 0 && !m_pdata->deathData.isDead) {
        m_pdata->deathData.isDead = true;
    }
}

void IRole::takeDamage(uint8_t damage) {
    if (damage >= m_pdata->healthData.currentHealth)
        m_pdata->healthData.currentHealth = 0;
    else
        m_pdata->healthData.currentHealth -= damage;
}

bool IRole::isActive() {
    return m_pdata->isActive;
}
