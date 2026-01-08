#ifndef LEADINGROLE_HPP
#define LEADINGROLE_HPP

#include "role.hpp"

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

//闪电链弹一束条的范围穿透伤害，mul*attackPower+30 点伤害

//role.cpp中的createBullet决定发射子弹的数值和机制

/**
 * @brief LeadingRole class
 * @note  中文：主角 ｜ 英文：LeadingRole,玩家操控的主要角色，具备平衡的属性和多样的攻击方式。
 * @note  中等体型（16x16 像素），中等血量，适中的移动速度，能够发射多种类型的子弹，适合各种战斗场景。
 */

class BulletTypeOwned {
public:
    uint8_t BulletOwnedTypeCount = 3 ;
    uint8_t basicBulletOwed = 1 ;
    uint8_t fireBallBulletOwed = 0 ;
    uint8_t lightningLineBulletOwed = 0 ;
public:
    BulletTypeOwned() = delete ;
    BulletTypeOwned(uint8_t basic , uint8_t fireball , uint8_t lightning ) {
        basicBulletOwed = basic ;
        fireBallBulletOwed = fireball ;
        lightningLineBulletOwed = lightning ;
        BulletOwnedTypeCount = 3 ;
    }
};

class LeadingRole : public IRole {
public:
    uint16_t              experiencePoints      = 0;
    
    //满级为 15 级
    uint16_t experienceToLevelUp[16] = {0, 40 , 100 , 125 , 180 , 220 , 265 , 320 ,  380 , 500 , 600 , 700 , 800 , 900 , 950 , 999}; // 每级所需经验值

    // ===== 僚机系统 =====
    bool phoenixWingmanOwned = false;   // 是否拥有凤凰僚机
    bool kuiniuWingmanOwned = false;    // 是否拥有夔牛僚机
    bool magicTimOwned = false;         // 是否拥有魔法时间
    
    uint16_t phoenixShootTimer = 0;     // 凤凰僚机射击计时器
    uint16_t kuiniuShootTimer = 0;      // 夔牛僚机射击计时器
    uint16_t magicTimTimer = 0;         // 魔法时间计时器
    
    static const uint16_t WINGMAN_SHOOT_INTERVAL = 2000; // 僚机射击间隔2.0秒

public:
    BulletTypeOwned bulletTypeOwned = BulletTypeOwned(1,0,0); // 默认只拥有普通子弹
public:
    LeadingRole();
    ~LeadingRole() override = default;

    void levelUp() ; // 升级函数声明
    void init() override;
    void doAction() override;
    void die() override;
    void think() override;
    void shoot(uint8_t x , uint8_t y , BulletType type) override;
    void drawRole() override;
    
    // 僚机相关函数
    void updateWingmans();              // 更新僚机状态（计时器和射击）
    void drawPhoenixWingman();          // 绘制凤凰僚机
    void drawKuiniuWingman();           // 绘制夔牛僚机
    void phoenixShoot();                // 凤凰发射火球
    void kuiniuShoot();                 // 夔牛发射雷电
    void magicTimeShoot();               // 魔法时间发射子弹
};

#endif // LEADINGROLE_HPP
