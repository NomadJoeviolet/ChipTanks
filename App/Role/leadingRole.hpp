#ifndef LEADINGROLE_HPP
#define LEADINGROLE_HPP

#include "role.hpp"

// class GameEntityManager; // 添加前向声明，避免重复引用
// extern GameEntityManager g_entityManager;

class LeadingRole : public IRole {
public:
    LeadingRole() {//会优先执行 基类构造函数
        m_pdata->identity = RoleIdentity::Player;
        m_pdata->img = &tankImg ; // Assign appropriate image pointer
        m_pdata->spatialData.currentPosX = -16; // Starting X position
        m_pdata->spatialData.currentPosY = 0; // Starting Y position
        m_pdata->spatialData.sizeX = m_pdata->img->w; // Width of the role
        m_pdata->spatialData.sizeY = m_pdata->img->h; // Height of the role
    }
    ~LeadingRole() override = default;

    void init() override {
        static uint8_t init_count = 0 ;
       if( m_pdata->spatialData.currentPosX < 0) {
            init_count += controlDelayTime ;
            if(init_count >= 100) { // 每50ms移动一次
                m_pdata->spatialData.currentPosX ++ ;
                init_count = 0 ;
            }
       }
       else {
            m_pdata->isInited = true ;
            m_pdata->spatialData.refPosX = m_pdata->spatialData.currentPosX ;
            m_pdata->spatialData.refPosY = m_pdata->spatialData.currentPosY ;
       }
    }

    void doAction() override {
        // Implement action logic for the leading role

        switch (m_pdata->actionData.currentState) {
        case ActionState::IDLE:
            // Do nothing
            break;
        case ActionState::MOVING:
            switch (m_pdata->actionData.moveMode) {
            case MoveMode::LEFT:
                move(-1, 0);
                break;
            case MoveMode::RIGHT:
                move(1, 0);
                break;
            case MoveMode::UP:
                move(0, -1);
                break;
            case MoveMode::DOWN:
                move(0, 1);
                break;
            default:
                break;
            }
            m_pdata->actionData.currentState = ActionState::IDLE;
            m_pdata->actionData.moveMode = MoveMode::NONE;
            break;
        }

        shoot();

    }
    
    void die() override {
        
    }

    void think() override {
        // Implement player-specific logic if needed

    }

    void shoot() override {
        // Implement shooting logic for the leading role
    }

};

#endif // LEADINGROLE_HPP
