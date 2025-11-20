#ifndef LEADINGROLE_HPP
#define LEADINGROLE_HPP

#include "role.hpp"

// class GameEntityManager; // 添加前向声明，避免重复引用
// extern GameEntityManager g_entityManager;

class LeadingRole : public IRole {
public:
    LeadingRole();
    ~LeadingRole() override = default;

    
    void init();
    void doAction() ;
    void die() ;
    void think() ;
    void shoot(uint8_t x , uint8_t y , BulletType type) ;
    
};

#endif // LEADINGROLE_HPP
