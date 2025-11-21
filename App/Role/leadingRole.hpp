#ifndef LEADINGROLE_HPP
#define LEADINGROLE_HPP

#include "role.hpp"

// class GameEntityManager; // 添加前向声明，避免重复引用
// extern GameEntityManager g_entityManager;

class LeadingRole : public IRole {
public:
    LeadingRole();
    ~LeadingRole() override = default;

    
    void init() override;
    void doAction() override;
    void die() override;
    void think() override;
    void shoot(uint8_t x , uint8_t y , BulletType type) override;
    void drawRole() override;
    
};

#endif // LEADINGROLE_HPP
