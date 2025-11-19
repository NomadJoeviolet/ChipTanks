#ifndef ROLE_HPP
#define ROLE_HPP

#include "basicData.hpp"
#include "etl/algorithm.h"

extern uint8_t controlDelayTime ;

class IRole {
protected:
    RoleData* m_pdata = nullptr ;

public:
    IRole()  {
        if(m_pdata == nullptr)
            m_pdata = new RoleData();
    }
    virtual ~IRole() {
        if(m_pdata != nullptr)
            delete [] m_pdata;
        m_pdata = nullptr;
    }

public:
    RoleData* getData() {
        return m_pdata;
    }

    virtual void init() = 0;
    // virtual void move(int8_t dirX, int8_t dirY) = 0;
    virtual void shoot() = 0;
    virtual void think() = 0;

    void move(int8_t dirX, int8_t dirY) {
        // Implement enemy movement logic
      uint8_t tmpX = m_pdata->spatialData.currentPosX + dirX * m_pdata->spatialData.moveSpeed;
      uint8_t tmpY = m_pdata->spatialData.currentPosY + dirY * m_pdata->spatialData.moveSpeed;
      // Boundary checks (assuming screen size 128x64)
      if (tmpX < 0 || tmpX > 128 - m_pdata->spatialData.sizeX)
          return;
      if (tmpY < 0 || tmpY > 64 - m_pdata->spatialData.sizeY)
          return;

      m_pdata->spatialData.refPosX = tmpX;
      m_pdata->spatialData.refPosY = tmpY;
    }

    void update(CollisionResult collisionResult) {
        //初始化
        if(!m_pdata->isInited) {
            //首次初始化
            init();
            return ;
        }

        think();

        //位置更新
        if(!collisionResult.isCollision) {
            m_pdata->spatialData.currentPosX = m_pdata->spatialData.refPosX;
            m_pdata->spatialData.currentPosY = m_pdata->spatialData.refPosY;
        } else {
            m_pdata->spatialData.refPosX = m_pdata->spatialData.currentPosX;
            m_pdata->spatialData.refPosY = m_pdata->spatialData.currentPosY;
        }

        //碰撞处理
        //碰撞处理在位置更新后进行
        if(collisionResult.isCollision ) {
            //碰撞后退回
            uint8_t backStep = 2 ;
            if(m_pdata->identity == RoleIdentity::Player)
                backStep = 5 ;
            if(collisionResult.direction == CollisionDirection::LEFT)
                m_pdata->spatialData.refPosX = etl::max(m_pdata->spatialData.currentPosX-backStep,0);
            else if(collisionResult.direction == CollisionDirection::RIGHT)
                m_pdata->spatialData.refPosX = etl::min(m_pdata->spatialData.currentPosX+backStep,128 - m_pdata->spatialData.sizeX);
            else if(collisionResult.direction == CollisionDirection::UP)
                m_pdata->spatialData.refPosY = etl::max(m_pdata->spatialData.currentPosY-backStep,0);
            else if(collisionResult.direction == CollisionDirection::DOWN)
                m_pdata->spatialData.refPosY = etl::min(m_pdata->spatialData.currentPosY+backStep,64 - m_pdata->spatialData.sizeY);
            //if(m_pdata->identity == RoleIdentity::Player)
                //takeDamage(20); // Example damage value on collision
        }

        //射击冷却处理
        if(m_pdata->attackData.shootCooldownTimer > 0) {
            uint16_t shootCooldownTime = m_pdata->attackData.shootCooldownSpeed ;
            if(controlDelayTime*shootCooldownTime >= m_pdata->attackData.shootCooldownTimer)
                m_pdata->attackData.shootCooldownTimer = 0;
            else
            m_pdata->attackData.shootCooldownTimer -= controlDelayTime*shootCooldownTime;
        }

        //热量冷却处理
        if (m_pdata->heatData.currentHeat > 0 )
        {
            m_pdata->heatData.heatCoolDownTimer += controlDelayTime ;
            if (m_pdata->heatData.heatCoolDownTimer >= 200) //每300ms降温一次
            {
                if (m_pdata->heatData.currentHeat <= m_pdata->heatData.heatCoolDownRate)
                    m_pdata->heatData.currentHeat = 0;
                else
                    m_pdata->heatData.currentHeat -= m_pdata->heatData.heatCoolDownRate;
                m_pdata->heatData.heatCoolDownTimer = 0 ;
            }
        }
        
    }
    
    void takeDamage(uint8_t damage) {
        if(damage >= m_pdata->currentHealth)
            m_pdata->currentHealth = 0;
        else
            m_pdata->currentHealth -= damage;
    }

    bool isActive() {
        return m_pdata->currentHealth > 0;
    }

};


#endif // ROLE_HPP
