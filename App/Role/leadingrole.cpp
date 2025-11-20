#include "leadingRole.hpp"
#include "../gameEntityManager.hpp"

extern GameEntityManager g_entityManager;

LeadingRole::LeadingRole()
: IRole() { //会优先执行 基类构造函数
    m_pdata->identity                = RoleIdentity::Player;
    m_pdata->img                     = &tankImg;        // Assign appropriate image pointer
    m_pdata->spatialData.currentPosX = -16;             // Starting X position
    m_pdata->spatialData.currentPosY = 0;               // Starting Y position
    m_pdata->spatialData.sizeX       = m_pdata->img->w; // Width of the role
    m_pdata->spatialData.sizeY       = m_pdata->img->h; // Height of the role


    //血量
    m_pdata->healthData.healValue = 3 ;
    m_pdata->healthData.healTimeCounter = 0 ;
    m_pdata->healthData.healResetTime = 15000 ; 
    m_pdata->healthData.healSpeed = 5 ;

    m_pdata->attackData.attackPower            = 10 ;
    m_pdata->attackData.shootCooldownSpeed     = 5;
    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 4000 ;
    m_pdata->attackData.bulletSpeed            = 1;
    m_pdata->heatData.maxHeat          = 100;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 20;
    m_pdata->heatData.heatCoolDownRate = 10;//每次冷却10点热量，每次冷却时间间隔由200ms


}

void LeadingRole::init() {
    static uint8_t init_count = 0;
    if (m_pdata->spatialData.currentPosX < 0) {
        init_count += controlDelayTime;
        if (init_count >= 100) { // 每50ms移动一次
            m_pdata->spatialData.currentPosX++;
            init_count = 0;
        }
    } else {
        m_pdata->isInited            = true;
        m_pdata->spatialData.refPosX = m_pdata->spatialData.currentPosX;
        m_pdata->spatialData.refPosY = m_pdata->spatialData.currentPosY;
    }
}

void LeadingRole::doAction() {
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
        m_pdata->actionData.moveMode     = MoveMode::NONE;
        break;
    }

    uint8_t m_x = m_pdata->spatialData.currentPosX+m_pdata->spatialData.sizeX/2;
    uint8_t m_y = m_pdata->spatialData.currentPosY+m_pdata->spatialData.sizeY/2;
    shoot( m_x , m_y ,
          BulletType::BASIC);
}

void LeadingRole::die() { }

void LeadingRole::think() {
    // Implement player-specific logic if needed
}

void LeadingRole::shoot(uint8_t x, uint8_t y, BulletType type) {
    // Create bullet based on type
    IBullet* newBullet = createBullet(x, y, type);
    if (newBullet != nullptr) {
        // Add bullet to entity manager
        if (!g_entityManager.addBullet(newBullet)) {
            delete [] newBullet; // Prevent memory leak if adding fails
        } else {
            // Reset shoot cooldown timer
            m_pdata->attackData.shootCooldownTimer += m_pdata->attackData.shootCooldownResetTime;
            // Increase heat
            m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot;
        }
    }
}

