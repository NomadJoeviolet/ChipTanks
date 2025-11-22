#ifndef ROLE_HPP
#define ROLE_HPP

#include "basicData.hpp"
#include "bullet.hpp"
#include "etl/algorithm.h"


extern uint8_t controlDelayTime ;

/**
 * @brief 角色接口类
 * @note 所有角色类均需继承该接口类
 * @note think()函数用于实现AI逻辑，决定Action状态
 */
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

    virtual void shoot(uint8_t x , uint8_t y , BulletType type) = 0;
    virtual void doAction() = 0;
    virtual void init() = 0;
    virtual void die() = 0;
    virtual void think() = 0;
    virtual void drawRole() = 0; // 画角色函数声明
    
    IBullet* createBullet(uint8_t x , uint8_t y , BulletType type ) ;
    void move(int8_t dirX, int8_t dirY , bool ignoreSpeed  = false) ;
    void update(CollisionResult collisionResult) ;
    void takeDamage(uint8_t damage) ;
    bool isActive() ;

};


#endif // ROLE_HPP
