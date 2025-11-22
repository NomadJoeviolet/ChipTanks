#include "bullet.hpp"
#include "../Peripheral/OLED/oled.h"
#include "etl/algorithm.h"
//子弹方向处理再doAction中进行

extern uint8_t controlDelayTime ;

/// IBullet class implementations
/***************************************/
IBullet::IBullet() {
    if (m_data == nullptr) m_data = new BulletData();
}

IBullet::~IBullet() {
    if (m_data != nullptr) delete[] m_data;
    m_data = nullptr;
}

bool IBullet::isActive() {
    // Implement logic to determine if the bullet is active
    return m_data->isActive; // Placeholder
}
/***************************************/



/// BasicBullet class implementations
/***************************************/
BasicBullet::BasicBullet(
    int8_t speed, uint8_t posX, uint8_t posY, uint8_t rg, uint8_t damage, RoleIdentity fromIdentity
)
: IBullet() {
    //子弹类型
    m_data->type         = BulletType::BASIC;
    //激活
    m_data->isActive     = true;
    
    m_data->img          = &BasicBulletImg; // Assign appropriate image pointer

    m_data->damage       = damage;
    m_data->range        = rg;
    m_data->fromIdentity = fromIdentity;

    m_data->spatialData = SpatialMovementData( false,speed, posX, posY, posX, posY, m_data->img->w, m_data->img->h);

    m_data->deathData.deathTimer = 100;
    m_data->deathData.isDead     = false;
}

void BasicBullet::doAction() {
    // Implement action logic for the basic bullet if needed
    if (m_data->fromIdentity == RoleIdentity::Player)
        move(1, 0); // Move right by default
    else
        move(-1, 0); // Move left by default
}

void BasicBullet::move(int8_t dirX, int8_t dirY) {
    // Implement enemy movement logic
    uint8_t tmpX = m_data->spatialData.currentPosX + dirX * m_data->spatialData.moveSpeed;
    uint8_t tmpY = m_data->spatialData.currentPosY + dirY * m_data->spatialData.moveSpeed;
    // Boundary checks (assuming screen size 128x64)
    if (tmpX < 0 || tmpX > 128 - m_data->spatialData.sizeX+1) return;
    if (tmpY < 0 || tmpY > 64 - m_data->spatialData.sizeY+1 ) return;

    m_data->spatialData.refPosX = tmpX;
    m_data->spatialData.refPosY = tmpY;
}

void BasicBullet::update(CollisionResult collisionResult) {
    if(m_data->deathData.isDead) {
        die();
        return;
    }

    //根据碰撞结果更新子弹状态
    if (collisionResult.isCollision) {
        //碰撞后，子弹死亡
        m_data->deathData.isDead = true;
        return;
    } else {
        //越界处理
        if (m_data->spatialData.currentPosX <= 0
            || m_data->spatialData.currentPosX + m_data->spatialData.sizeX - 1 >= 128-1 - bound_deadzone
            || m_data->spatialData.currentPosY <= 0
            || m_data->spatialData.currentPosY + m_data->spatialData.sizeY - 1 >= 64-1 - bound_deadzone ) {
            m_data->isActive = false;
            return;
        }

        //位置更新
        m_data->spatialData.currentPosX = m_data->spatialData.refPosX;
        m_data->spatialData.currentPosY = m_data->spatialData.refPosY;
    }
}

void BasicBullet::drawBullet() {
    if (m_data != nullptr && m_data->img != nullptr && m_data->isActive && !m_data->deathData.isDead ) {
        OLED_DrawImage(
            m_data->spatialData.currentPosX,
            m_data->spatialData.currentPosY,
            m_data->img,
            OLED_COLOR_NORMAL
        );
    }
}

void BasicBullet::die() {
    // Implement bullet death logic
    if (m_data->deathData.deathTimer > 0) {
        m_data->deathData.deathTimer -= controlDelayTime;
        m_data->deathData.deathTimer = etl::max(m_data->deathData.deathTimer, uint16_t(0));
        return;
    }
    m_data->isActive = false;
}

/***************************************/


/// FireBallBullet class implementations
/***************************************/
FireBallBullet::FireBallBullet(
    int8_t speed, uint8_t posX, uint8_t posY, uint8_t rg, uint8_t damage, RoleIdentity fromIdentity
) {
    m_data->type     = BulletType::FIRE_BALL;
    m_data->isActive = true;
    
    m_data->img      = &FireBallBulletImg; // Assign appropriate image pointer

    m_data->damage   = damage;
    m_data->range    = rg;
    m_data->fromIdentity = fromIdentity;

    m_data->spatialData = SpatialMovementData( false,speed, posX, posY, posX, posY, m_data->img->w, m_data->img->h);

    m_data->deathData.deathTimer = death_count ;
    m_data->deathData.isDead     = false;
}

void FireBallBullet::doAction() {
    // Implement action logic for the fire ball bullet if needed
    if (m_data->fromIdentity == RoleIdentity::Player)
        move(1, 0); // Move right by default
    else
        move(-1, 0); // Move left by default
}

void FireBallBullet::move(int8_t dirX, int8_t dirY) {
    // Implement enemy movement logic
    uint8_t tmpX = m_data->spatialData.currentPosX + dirX * m_data->spatialData.moveSpeed;
    uint8_t tmpY = m_data->spatialData.currentPosY + dirY * m_data->spatialData.moveSpeed;
    // Boundary checks (assuming screen size 128x64)
    if (tmpX < 0 || tmpX > 128 - m_data->spatialData.sizeX+1) return;
    if (tmpY < 0 || tmpY > 64 - m_data->spatialData.sizeY+1 ) return;

    m_data->spatialData.refPosX = tmpX;
    m_data->spatialData.refPosY = tmpY;
}

void FireBallBullet::update(CollisionResult collisionResult) {
    if(m_data->deathData.isDead) {
        die();
        return;
    }

    //根据碰撞结果更新子弹状态
    if (collisionResult.isCollision) {
        //碰撞后，子弹死亡
        m_data->deathData.isDead = true;
        return;
    } else {
        //越界处理
        if (m_data->spatialData.currentPosX <= 0
            || m_data->spatialData.currentPosX + m_data->spatialData.sizeX - 1 >= 128-1 - bound_deadzone
            || m_data->spatialData.currentPosY <= 0
            || m_data->spatialData.currentPosY + m_data->spatialData.sizeY - 1 >= 64-1 - bound_deadzone ) {
            m_data->isActive = false;
            return;
        }

        //位置更新
        m_data->spatialData.currentPosX = m_data->spatialData.refPosX;
        m_data->spatialData.currentPosY = m_data->spatialData.refPosY;
    }
}

void FireBallBullet::drawBullet() {
    if (m_data != nullptr && m_data->img != nullptr && m_data->isActive && !m_data->deathData.isDead ) {
        OLED_DrawImage(
            m_data->spatialData.currentPosX,
            m_data->spatialData.currentPosY,
            m_data->img,
            OLED_COLOR_NORMAL
        );
    }
    if(m_data->deathData.isDead) {
        uint8_t centerX = m_data->spatialData.currentPosX + m_data->spatialData.sizeX / 2;
        uint8_t centerY = m_data->spatialData.currentPosY + m_data->spatialData.sizeY / 2;
        uint8_t radius  = (death_count - m_data->deathData.deathTimer)*m_data->range/death_count ; // 从0增长到最大值range
        radius          = etl::max(radius, uint8_t(1));                                 // 最小半径限制

        OLED_DrawCircle(centerX, centerY, radius, OLED_COLOR_NORMAL);
    }
}

void FireBallBullet::die() {
    // Implement bullet death logic
    if (m_data->deathData.deathTimer > 0) {
        m_data->deathData.deathTimer -= controlDelayTime;
        m_data->deathData.deathTimer = etl::max(m_data->deathData.deathTimer, uint16_t(0));
        return;
    }
    m_data->isActive = false;
}




/***************************************/



/// LightningLineBullet class implementations
/***************************************/
LightningLineBullet::LightningLineBullet(
    int8_t speed, uint8_t posX, uint8_t posY, uint8_t rg, uint8_t damage, RoleIdentity fromIdentity
) {
    m_data->type     = BulletType::LIGHTNING_LINE;
    m_data->isActive = true;
    
    m_data->img      = &LightningLineBulletImg; // Assign appropriate image pointer

    m_data->damage   = damage;
    m_data->range    = rg;
    m_data->fromIdentity = fromIdentity;

    m_data->spatialData = SpatialMovementData( true ,speed, posX, posY, posX, posY, m_data->img->w, m_data->img->h);

    m_data->deathData.deathTimer = death_count ;
    m_data->deathData.isDead     = false;
}

void LightningLineBullet::doAction() {
    // Implement action logic for the lightning line bullet if needed
}

//不会移动
void LightningLineBullet::move(int8_t dirX, int8_t dirY) {
    // Implement enemy movement logic
    
}

void LightningLineBullet::update(CollisionResult collisionResult) {
    if(m_data->deathData.isDead) {
        die();
        return;
    }

    if(collisionResult.isCollision) {
        //碰撞后，子弹死亡
        m_data->deathData.isDead = true;
        return;
    }

    lasting_count -= controlDelayTime;
    if (lasting_count <= 0 ) {
        m_data->deathData.isDead = true;
        return;
    } 
}

void LightningLineBullet::drawBullet() {
    if (m_data != nullptr && m_data->img != nullptr && m_data->isActive && !m_data->deathData.isDead ) {
        OLED_DrawImage(
            m_data->spatialData.currentPosX,
            m_data->spatialData.currentPosY,
            m_data->img,
            OLED_COLOR_NORMAL
        );
    }
    if(m_data->deathData.isDead) {
        OLED_DrawImage(
            m_data->spatialData.currentPosX,
            m_data->spatialData.currentPosY,
            m_data->img,
            OLED_COLOR_NORMAL
        );
    }
}

void LightningLineBullet::die() {
    // Implement bullet death logic
    if (m_data->deathData.deathTimer > 0) {
        m_data->deathData.deathTimer -= controlDelayTime;
        m_data->deathData.deathTimer = etl::max(m_data->deathData.deathTimer, uint16_t(0));
        return;
    }
    m_data->isActive = false;
}

/***************************************/


