#ifndef BULLET_HPP
#define BULLET_HPP

#include "basicData.hpp"

class IBullet {
public:
    BulletData* m_data = nullptr;

public:
    IBullet() {
        m_data = new BulletData();
    }
    virtual ~IBullet() {
        if(m_data != nullptr)
            delete [] m_data;
        m_data = nullptr;
    }

    bool isActive() {
        // Implement logic to determine if the bullet is active
        return m_data->isActive; // Placeholder
    }
};

#endif // BULLET_HPP
