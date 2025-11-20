#ifndef BULLET_HPP
#define BULLET_HPP

#include "font.h"
#include "basicData.hpp"

//子弹方向处理再daAction中进行


class IBullet {
public:
    BulletData* m_data = nullptr;
    uint8_t  bound_deadzone = 5 ; //边界死区，防止子弹贴边卡住

public:
    IBullet();
    IBullet(const IBullet&) = delete;
    IBullet& operator=(const IBullet&) = delete;
    virtual ~IBullet() ;

    virtual void move(int8_t dirX, int8_t dirY) = 0;
    virtual void update(CollisionResult collisionResult) = 0;
    virtual void doAction() {};
    
    bool isActive() ;
};

class BasicBullet : public IBullet {
public:
    BasicBullet(int8_t speed, uint8_t posX, uint8_t posY, uint8_t rg , uint8_t damage , RoleIdentity fromIdentity) ;
    ~BasicBullet() override = default;

    void doAction();
    void move(int8_t dirX, int8_t dirY) ;
    void update(CollisionResult collisionResult);

};

class FireBallBullet : public IBullet {
public:
    FireBallBullet( int8_t speed, uint8_t posX, uint8_t posY , uint8_t rg , uint8_t damage ) ;
    ~FireBallBullet() override = default;

    void doAction();
    void move(int8_t dirX, int8_t dirY) ;
    void update(CollisionResult collisionResult);
};

class LightningLineBullet : public IBullet {
public:
    LightningLineBullet( int8_t speed, uint8_t posX, uint8_t posY , uint8_t rg , uint8_t damage ) ;
    ~LightningLineBullet() override = default;

    void doAction();
    void move(int8_t dirX, int8_t dirY) ;
    void update(CollisionResult collisionResult);
};

#endif // BULLET_HPP
