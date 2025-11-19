#ifndef GAMEENTITYMANAGER_HPP
#define GAMEENTITYMANAGER_HPP

#include "math.h"
#include "role.hpp"
#include "etl/vector.h"
#include "task.h"

class GameEntityManager {
public:
    etl::vector<IRole*, 20> m_roles;//角色池，存放所有角色指针
    etl::vector<IBullet*, 100> m_bullets;//子弹池，存放所有子弹指针

public:
    GameEntityManager() = default;
    ~GameEntityManager() {
        cleanUpAllRoles(true);
        cleanUpAllBullets(true);
    }
    GameEntityManager(const GameEntityManager&) = delete;
    GameEntityManager& operator=(const GameEntityManager&) = delete;
public:
    bool addRole(IRole* role) {
        if(role == nullptr || m_roles.full())
            return false;

        taskENTER_CRITICAL();
        m_roles.push_back(role);
        taskEXIT_CRITICAL();

        return true;
    }

    bool addBullet(IBullet* bullet) {
        if(bullet == nullptr || m_bullets.full())
            return false;

        taskENTER_CRITICAL();
        m_bullets.push_back(bullet);
        taskEXIT_CRITICAL();

        return true;
    }

    bool removeAndDestroyRole(IRole* role) {
        if(role == nullptr)
            return false;

        taskENTER_CRITICAL();
        auto it = etl::find(m_roles.begin(), m_roles.end(), role);
        if(it != m_roles.end()) {
            m_roles.erase(it);
            delete role;
            taskEXIT_CRITICAL();
            return true;
        }
        taskEXIT_CRITICAL();
        return false;
    }

    bool removeAndDestroyBullet(IBullet* bullet) {
        if(bullet == nullptr)
            return false;

        taskENTER_CRITICAL();
        auto it = etl::find(m_bullets.begin(), m_bullets.end(), bullet);
        if(it != m_bullets.end()) {
            m_bullets.erase(it);
            delete bullet;
            taskEXIT_CRITICAL();
            return true;
        }
        taskEXIT_CRITICAL();
        return false;
    }

    void cleanupInvalidRoles() {
        taskENTER_CRITICAL();
        auto it = m_roles.begin();
        while(it != m_roles.end()) {
            IRole* rolePtr = *it;
            if( rolePtr != nullptr  && !rolePtr->isActive() && rolePtr->getData()->identity != RoleIdentity::Player ) {
                
                it = m_roles.erase(it);
                delete rolePtr;
                rolePtr = nullptr;
                
            } else {
                ++it;
            }
        }
        taskEXIT_CRITICAL();
    }

    void cleanupInvalidBullets() {
        taskENTER_CRITICAL();
        auto it = m_bullets.begin();
        while(it != m_bullets.end()) {
            IBullet* bulletPtr = *it;
            if( bulletPtr != nullptr && !bulletPtr->isActive() ) {
                it = m_bullets.erase(it);
                delete bulletPtr;
                bulletPtr = nullptr;
            } else {
                ++it;
            }
        }
        taskEXIT_CRITICAL();
    }

    void clearAllEntities(bool deleteObjects = false) {
        cleanUpAllRoles(deleteObjects);
        cleanUpAllBullets(deleteObjects);
    }

    CollisionResult checkRoleRefPositionCollision(IRole* role_A ) {

        CollisionResult collisionDetected = {false, CollisionDirection::NONE};

        if(role_A == nullptr)
            return collisionDetected;
        taskENTER_CRITICAL();
        for(auto role_B : m_roles) {
            if(role_B == nullptr || role_B == role_A)
                continue;
            // Simple AABB collision detection
            RoleData* dataA = role_A->getData();
            RoleData* dataB = role_B->getData();

            if (dataA->spatialData.refPosX < dataB->spatialData.currentPosX + dataB->spatialData.sizeX-1 &&
                dataA->spatialData.refPosX + dataA->spatialData.sizeX-1 > dataB->spatialData.currentPosX &&
                dataA->spatialData.refPosY < dataB->spatialData.currentPosY + dataB->spatialData.sizeY-1 &&
                dataA->spatialData.refPosY + dataA->spatialData.sizeY-1 > dataB->spatialData.currentPosY) {
                collisionDetected.isCollision = true;

                role_A->takeDamage(20); // Example damage value on collision
                role_B->takeDamage(20); // Example damage value on collision

                // Determine collision direction
                // if (dataA->spatialData.refPosX < dataB->spatialData.currentPosX) {
                //     collisionDetected.direction = CollisionDirection::LEFT;
                // } else if (dataA->spatialData.refPosX > dataB->spatialData.currentPosX) {
                //     collisionDetected.direction = CollisionDirection::RIGHT;
                // } else if (dataA->spatialData.refPosY < dataB->spatialData.currentPosY) {
                //     collisionDetected.direction = CollisionDirection::UP;
                // } else {
                //     collisionDetected.direction = CollisionDirection::DOWN;
                // }
                // break;

                 if (dataA->spatialData.refPosX < dataB->spatialData.currentPosX + dataB->spatialData.sizeX-1 &&
                dataA->spatialData.refPosX + dataA->spatialData.sizeX-1 > dataB->spatialData.currentPosX &&
                dataA->spatialData.refPosY < dataB->spatialData.currentPosY + dataB->spatialData.sizeY-1 &&
                dataA->spatialData.refPosY + dataA->spatialData.sizeY-1 > dataB->spatialData.currentPosY) {
                collisionDetected.isCollision = true;
                
                // --- 开始：严谨的碰撞方向判断 ---
                // 计算两个角色中心的坐标
                float centerA_X = dataA->spatialData.refPosX + dataA->spatialData.sizeX / 2.0f;
                float centerA_Y = dataA->spatialData.refPosY + dataA->spatialData.sizeY / 2.0f;
                float centerB_X = dataB->spatialData.currentPosX + dataB->spatialData.sizeX / 2.0f;
                float centerB_Y = dataB->spatialData.currentPosY + dataB->spatialData.sizeY / 2.0f;

                // 计算两个中心之间的向量
                float dx = centerA_X - centerB_X;
                float dy = centerA_Y - centerB_Y;

                // 计算两个矩形合并后的总宽度和总高度的一半
                float combinedHalfWidth = (dataA->spatialData.sizeX + dataB->spatialData.sizeX) / 2.0f;
                float combinedHalfHeight = (dataA->spatialData.sizeY + dataB->spatialData.sizeY) / 2.0f;

                // 计算X和Y轴上的重叠量
                float overlapX = combinedHalfWidth - fabsf(dx);
                float overlapY = combinedHalfHeight - fabsf(dy);

                // 比较重叠量，重叠小的那个轴是碰撞发生的轴
                if (overlapX < overlapY) {
                    // 水平碰撞
                    if (dx > 0) { // A的中心在B的右边
                        collisionDetected.direction = CollisionDirection::RIGHT;
                    } else { // A的中心在B的左边
                        collisionDetected.direction = CollisionDirection::LEFT;
                    }
                } else {
                    // 垂直碰撞
                    if (dy > 0) { // A的中心在B的下边
                        collisionDetected.direction = CollisionDirection::DOWN;
                    } else { // A的中心在B的上边
                        collisionDetected.direction = CollisionDirection::UP;
                    }
                }
                // --- 结束：严谨的碰撞方向判断 ---
                break;
            }

            }

        }
        taskEXIT_CRITICAL();
        return collisionDetected;
    }

    bool checkBulletRefPositionCollision(IBullet* bullet_A ) {
        bool collisionDetected = false;
        if(bullet_A == nullptr)
            return false;
        taskENTER_CRITICAL();
        for(auto role_B : m_roles) {
            if(role_B == nullptr)
                continue;
            // Simple AABB collision detection
            BulletData* dataA = bullet_A->m_data;
            RoleData* dataB = role_B->getData();

            if (dataA->spatialData.refPosX < dataB->spatialData.currentPosX + dataB->spatialData.sizeX-1 &&
                dataA->spatialData.refPosX + dataA->spatialData.sizeX-1 > dataB->spatialData.currentPosX &&
                dataA->spatialData.refPosY < dataB->spatialData.currentPosY + dataB->spatialData.sizeY-1 &&
                dataA->spatialData.refPosY + dataA->spatialData.sizeY-1 > dataB->spatialData.currentPosY) {
                collisionDetected = true;
                break;
            }

        }
        taskEXIT_CRITICAL();
        return collisionDetected;
    }

    void updateAllRoles() {
        taskENTER_CRITICAL();
        for(auto rolePtr : m_roles) {
            if(rolePtr != nullptr) {
                CollisionResult collisionResult = {false, CollisionDirection::NONE};
                if(rolePtr->getData()->isInited ) {
                //碰撞处理
                collisionResult = checkRoleRefPositionCollision(rolePtr);
                }
                rolePtr->update(collisionResult);
            }
        }
        taskEXIT_CRITICAL();
    }

private:
    void cleanUpAllRoles(bool deleteObjects = false) {
        if(!deleteObjects)
            return;
        for (auto rolePtr : m_roles) {
            delete rolePtr;
        }
        m_roles.clear();
    }
    void cleanUpAllBullets(bool deleteObjects = false) {
        if(!deleteObjects)
            return;
        for (auto bulletPtr : m_bullets) {
            delete bulletPtr;
        }
        m_bullets.clear();
    }


};


#endif // GAMEENTITYMANAGER_HPP
