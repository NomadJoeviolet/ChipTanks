#ifndef BULLET_HPP
#define BULLET_HPP

#include "font.h"
#include "basicData.hpp"

class IBullet {
public:
    BulletData* m_data = nullptr;

public:
    IBullet() {
        if(m_data == nullptr)
            m_data = new BulletData();
    }

    IBullet(const IBullet&) = delete;
    IBullet& operator=(const IBullet&) = delete;

    virtual ~IBullet() {
        if(m_data != nullptr)
            delete [] m_data;
        m_data = nullptr;
    }

    virtual void move(int8_t dirX, int8_t dirY) = 0;
    virtual void update(CollisionResult collisionResult) = 0;
    virtual void doAction() {};
    

    bool isActive() {
        // Implement logic to determine if the bullet is active
        return m_data->isActive; // Placeholder
    }
};

class BasicBullet : public IBullet {
public:
    BasicBullet(int8_t speed, uint8_t posX, uint8_t posY, uint8_t rg , uint8_t damage , RoleIdentity fromIdentity): IBullet() {
        m_data->type = BulletType::NORMAL;
        m_data->damage = damage;
        m_data->img = &BasicBulletImg; // Assign appropriate image pointer
        m_data->isActive = true;
        m_data->range = rg;
        m_data->fromIdentity = fromIdentity;

        m_data->spatialData = SpatialMovementData(speed, posX, posY, posX, posY, m_data->img->w, m_data->img->h);
        

    }

    ~BasicBullet() override = default;

    void doAction() override {
        // Implement action logic for the basic bullet if needed
        if(m_data->fromIdentity == RoleIdentity::Player)
            move( 1, 0); // Move right by default
        else 
            move( -1, 0); // Move left by default
    }

    void move(int8_t dirX, int8_t dirY) override {
        // Implement enemy movement logic
      uint8_t tmpX = m_data->spatialData.currentPosX + dirX * m_data->spatialData.moveSpeed;
      uint8_t tmpY = m_data->spatialData.currentPosY + dirY * m_data->spatialData.moveSpeed;
      // Boundary checks (assuming screen size 128x64)
      if (tmpX < 0 || tmpX > 128 - m_data->spatialData.sizeX)
          return;
      if (tmpY < 0 || tmpY > 64 - m_data->spatialData.sizeY)
          return;

      m_data->spatialData.refPosX = tmpX;
      m_data->spatialData.refPosY = tmpY;
    }

    void update(CollisionResult collisionResult) override {
        //根据碰撞结果更新子弹状态
        if(collisionResult.isCollision) {
            //碰撞后，子弹失效
            m_data->isActive = false;
            return;
        }
        else {
            //越界处理
            if(m_data->spatialData.currentPosX <= 0 || m_data->spatialData.currentPosX+m_data->spatialData.sizeX-1 >= 128 ||
               m_data->spatialData.currentPosY <= 0 || m_data->spatialData.currentPosY+m_data->spatialData.sizeY-1 >= 64 ) {
                m_data->isActive = false;
                return;
            }

            //位置更新
            m_data->spatialData.currentPosX = m_data->spatialData.refPosX;
            m_data->spatialData.currentPosY = m_data->spatialData.refPosY;
        }        
    }

};

class FireBallBullet : public IBullet {
public:
    FireBallBullet( int8_t speed, uint8_t posX, uint8_t posY , uint8_t rg , uint8_t damage ) {
        m_data->type = BulletType::FIRE_BALL;
        // Initialize other FireBallBullet-specific data here
        m_data->img = &FireBallBulletImg;
        m_data->isActive = true;

        m_data->damage = damage;
        m_data->range = rg;
        m_data->spatialData = SpatialMovementData(speed, posX, posY, posX, posY, m_data->img->w, m_data->img->h);


    }

    ~FireBallBullet() override = default;
};

class LightningLineBullet : public IBullet {
public:
    LightningLineBullet( int8_t speed, uint8_t posX, uint8_t posY , uint8_t rg , uint8_t damage ) {
        m_data->type = BulletType::LIGHTNING_LINE;
        // Initialize other LightningLineBullet-specific data here
        m_data->img = &LightningLineBulletImg;
        m_data->isActive = true;

        m_data->damage = damage;
        m_data->range = rg;
        m_data->spatialData = SpatialMovementData(speed, posX, posY, posX, posY, m_data->img->w, m_data->img->h);
    }
    ~LightningLineBullet() override = default;
};

#endif // BULLET_HPP
