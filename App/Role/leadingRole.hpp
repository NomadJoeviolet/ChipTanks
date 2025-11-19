#ifndef LEADINGROLE_HPP
#define LEADINGROLE_HPP

#include "role.hpp"

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

    void think() override {
        // Implement player-specific logic if needed

    }

    // void move(int8_t dirX, int8_t dirY) override {
    //   uint8_t tmpX = m_pdata->spatialData.currentPosX + dirX * m_pdata->spatialData.moveSpeed;
    //   uint8_t tmpY = m_pdata->spatialData.currentPosY + dirY * m_pdata->spatialData.moveSpeed;
    //   // Boundary checks (assuming screen size 128x64)
    //   if (tmpX < 0 || tmpX > 128 - m_pdata->spatialData.sizeX)
    //       return;
    //   if (tmpY < 0 || tmpY > 64 - m_pdata->spatialData.sizeY)
    //       return;

    //   m_pdata->spatialData.refPosX = tmpX;
    //   m_pdata->spatialData.refPosY = tmpY;

    // }

    void shoot() override {
        // Implement shooting logic for the leading role
    }

};

#endif // LEADINGROLE_HPP
