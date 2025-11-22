#ifndef BULLET_HPP
#define BULLET_HPP

#include "font.h"
#include "basicData.hpp"

//子弹方向处理再daAction中进行

class IBullet {
public:
    BulletData *m_data         = nullptr;
    uint8_t     bound_deadzone = 5; //边界死区，防止子弹贴边卡住

public:
    IBullet();
    IBullet(const IBullet &)            = delete;
    IBullet &operator=(const IBullet &) = delete;
    virtual ~IBullet();

    virtual void die()                                   = 0;
    virtual void move(int8_t dirX, int8_t dirY)          = 0;
    virtual void update(CollisionResult collisionResult) = 0;
    virtual void doAction()                              = 0;
    virtual void drawBullet()                            = 0;

    bool isActive();
};

class BasicBullet : public IBullet {
public:
    BasicBullet(int8_t speed, uint8_t posX, uint8_t posY, uint8_t rg, uint8_t damage, RoleIdentity fromIdentity);
    ~BasicBullet() override = default;

    void drawBullet() override;
    void doAction() override;
    void move(int8_t dirX, int8_t dirY) override;
    void update(CollisionResult collisionResult) override;
    void die() override;
};

class FireBallBullet : public IBullet {
public:
    static const uint16_t death_count = 300 ; // 死亡动画时间，单位ms

public:
    FireBallBullet(int8_t speed, uint8_t posX, uint8_t posY, uint8_t rg, uint8_t damage, RoleIdentity fromIdentity);
    ~FireBallBullet() override = default;

    void drawBullet() override;
    void doAction() override;
    void move(int8_t dirX, int8_t dirY) override;
    void update(CollisionResult collisionResult) override;
    void die() override;
};

class LightningLineBullet : public IBullet {
public:
    uint16_t lasting_count = 300; // 持续时间，单位ms
    uint16_t death_count  = 100 ; // 死亡动画时间，单位ms

public:
    LightningLineBullet(int8_t speed, uint8_t posX, uint8_t posY, uint8_t rg, uint8_t damage, RoleIdentity fromIdentity);
    ~LightningLineBullet() override = default;

    void drawBullet() override;
    void doAction() override;
    void move(int8_t dirX, int8_t dirY) override;
    void update(CollisionResult collisionResult) override;
    void die() override;
};

#endif // BULLET_HPP
