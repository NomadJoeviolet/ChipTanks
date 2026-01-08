#include "enemyRole.hpp"
#include "bullet.hpp"

#include "etl/algorithm.h"
#include "../Peripheral/OLED/oled.h"
#include "../gameEntityManager.hpp"

#include "FreeRTOS.h"
#include "task.h"

extern GameEntityManager g_entityManager;

//��ֵ�淶
//����Ѫ��: 30����Ѫ������100����Ѫ������200����Ѫ����
//����������: 1-5 ���͹�������5-15���й�������15+���߹�����
//�����ƶ��ٶ�: 1�����٣���2�����٣���3�����٣�

//controlDelayTime �� threads.cpp �����̶߳���
//controlDelayTime = 10
//������ȴ���������Ʋ鿴role.cpp
//������ȴʱ��=resetTime/ (Speed) ms
//������ȴ����= heatCoolDownRate ÿ����ȴʱ��������200ms

//��ͨ�ӵ��������ı��� 1
//�������������ı��� 2
//���������������ı��� 1.5

//role.cpp�е�createBullet���������ӵ�����ֵ�ͻ���
//��ͨ�ӵ����е��˺������˺��� attackPower ���˺�
//���򵯻��е��˺��Ի��еĵ�������һ���˺�������һ����Χ�����ɷ�Χ�˺������еĵ���Ҳ���ܵ���Χ�˺���
//�����˺���Ϊ attackPower +10 ���˺�

//��������һ�����ķ�Χ��͸�˺���multiplier*attackPower+30 ���˺�

/*******************************************************************/
/**
 * @brief FeilianEnemy class
 * @note  ���ģ����� �� Ӣ�ģ�Feilian,�񻰵��ʣ��й��Ŵ������еķ���������¹��ͷ���ǡ��������������ɣ������ƹܰ������硣
 * @note  �����ƶ���С�͵��ˣ�����С�ɣ�12x12 ���أ����ƶ��켣Ʈ������Ӧ ���硱 �����ԣ������ι����˺��ͣ�����Ⱥ����ʱѹ�ȸ�ǿ��
 * @note  ֻ�ᷢ����ͨ�ӵ������ж���ʽƮ��������
 */

//��ֵ�趨�ο�
//Ѫ����30 + level * 1
//��������1 + level * 1
//�ƶ��ٶȣ�3�����٣�
//������ȴʱ�䣺4000/5 ms
//�������ƣ�ÿ����������20����������������100�㣬��ȴ����10��/200ms

FeilianEnemy::FeilianEnemy(
    uint8_t startX, uint8_t startY, uint8_t initPosX, uint8_t initPosY, uint8_t level, uint16_t dropExp
)
: IRole() {
    //ͼƬ��Ϣ
    m_pdata->img = &feilianImg;

    //������Ϣ
    m_pdata->identity          = RoleIdentity::ENEMY;
    m_pdata->isActive          = true;
    m_pdata->initData.isInited = false;

    //�ȼ���Ϣ
    m_pdata->level = level;

    //Ѫ����Ϣ
    m_pdata->healthData.currentHealth = 10 + level * 20;
    m_pdata->healthData.maxHealth     = 10 + level * 20;

    //��Ѫ��Ϣ
    m_pdata->healthData.healValue       = 0;
    m_pdata->healthData.healTimeCounter = 0;
    m_pdata->healthData.healResetTime   = 15000;
    m_pdata->healthData.healSpeed       = 0;

    m_pdata->spatialData.canCrossBorder            = false;
    m_pdata->spatialData.currentPosX               = startX; // Starting X position
    m_pdata->spatialData.currentPosY               = startY; // Starting Y position
    m_pdata->spatialData.refPosX                   = startX;
    m_pdata->spatialData.refPosY                   = startY;
    m_pdata->spatialData.sizeX                     = m_pdata->img->w;
    m_pdata->spatialData.sizeY                     = m_pdata->img->h;
    m_pdata->spatialData.moveSpeed                 = 3; // Set movement speed
    m_pdata->spatialData.consecutiveCollisionCount = 0;

    m_pdata->initData.posX = initPosX;
    m_pdata->initData.posY = initPosY;


    m_pdata->attackData.attackPower            = 2 + level * 1;
    m_pdata->attackData.shootCooldownSpeed     = 5;
    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 4000;
    m_pdata->attackData.bulletSpeed            = 1;

    m_pdata->attackData.bulletRange            = 0; 
    m_pdata->attackData.bulletDamageMultiplier = 1.5f;

    m_pdata->attackData.collisionPower = 4 + level * 1;


    m_pdata->heatData.maxHeat          = 100;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 20;
    m_pdata->heatData.heatCoolDownRate = 10; 

    m_pdata->deathData.deathTimer           = feilianEnemyDeadTime;
    m_pdata->deathData.isDead               = false;
    m_pdata->deathData.dropExperiencePoints = dropExp;

    // Initialize other enemy-specific data here
}

void FeilianEnemy::shoot(uint8_t x, uint8_t y, BulletType type) {
    // Implement enemy shooting logic
    // Create bullet based on type

    switch (type) {
    case BulletType::BASIC:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot > m_pdata->heatData.maxHeat)
                return;                                             // ���������������޷�����
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // ��ȴ�У��޷�����

            IBullet *newBullet = createBullet(x, y, BulletType::BASIC);
            if (newBullet != nullptr) {
                if (g_entityManager.addBullet(newBullet)) {
                    // Successfully added bullet
                    m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
                    m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot;
                } else {
                    delete newBullet; // Clean up if not added
                }
            }
        }
        break;
    case BulletType::FIRE_BALL:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 2 > m_pdata->heatData.maxHeat)
                return;                                            
            if (m_pdata->attackData.shootCooldownTimer > 0) return; 

            IBullet *newBullet = createBullet(x, y, BulletType::FIRE_BALL);
            if (newBullet != nullptr) {
                if (g_entityManager.addBullet(newBullet)) {
                    // Successfully added bullet
                    m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
                    m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot;
                } else {
                    delete newBullet; // Clean up if not added
                }
            }
        }
        break;
    case BulletType::LIGHTNING_LINE:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 1.5 > m_pdata->heatData.maxHeat)
                return;                                             // ���������������޷�����
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // ��ȴ�У��޷�����

            IBullet *newBullet = createBullet(x, y, BulletType::LIGHTNING_LINE);
            if (newBullet != nullptr) {
                if (g_entityManager.addBullet(newBullet)) {
                    // Successfully added bullet
                    m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
                    m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot;
                } else {
                    delete newBullet; // Clean up if not added
                }
            }
        }
    }
}

//update��ʵ��
void FeilianEnemy::init() {
    m_pdata->initData.init_count += controlDelayTime;
    // Initialize enemy role specifics

    if (m_pdata->spatialData.currentPosX > m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 30) { // ÿ30ms�ƶ�һ��
            m_pdata->spatialData.currentPosX -= 1;
            m_pdata->initData.init_count = 0;
        }
    } else if (m_pdata->spatialData.currentPosX < m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 30) { // ÿ30ms�ƶ�һ��
            m_pdata->spatialData.currentPosX += 1;
            m_pdata->initData.init_count = 0;
        }
    } else {
        m_pdata->initData.isInited   = true;
        m_pdata->spatialData.refPosX = m_pdata->spatialData.currentPosX;
        m_pdata->spatialData.refPosY = m_pdata->spatialData.currentPosY;
        m_pdata->initData.init_count = 0;
    }
}

void FeilianEnemy::think() {
    // Implement enemy AI logic
    think_count += controlDelayTime;
    if (think_count < 100) // ÿ100ms����һ���ж�
        return;

    think_count = 0;

    uint8_t randomAction = rand() % 6;
    // Random action: 0 - move left, 1 - move right, 2 - move down, 3 - move up, 4 - stay still, 5 - shoot
    if (m_pdata->actionData.currentState == ActionState::IDLE) {
        //�ƶ�
        if (randomAction == 0) {
            m_pdata->actionData.moveMode     = MoveMode::LEFT;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 1) {
            m_pdata->actionData.moveMode     = MoveMode::RIGHT;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 2) {
            m_pdata->actionData.moveMode     = MoveMode::DOWN;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 3) {
            m_pdata->actionData.moveMode     = MoveMode::UP;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 4) {
            // Stay still
            m_pdata->actionData.moveMode     = MoveMode::NONE;
            m_pdata->actionData.currentState = ActionState::MOVING;
        }

        //����
        else if (randomAction == 5) {
            m_pdata->actionData.attackMode   = AttackMode::MODE_1;
            m_pdata->actionData.currentState = ActionState::ATTACKING;
        }
    }
}

void FeilianEnemy::doAction() {
    if (m_pdata->initData.isInited == false) {
        return;
    }

    // Implement enemy action logic
    if (m_pdata->deathData.isDead) {
        return;
    }
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
        // Move logic handled in think()
        break;
    case ActionState::ATTACKING:
        uint8_t m_x = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t m_y = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
        switch (m_pdata->actionData.attackMode) {
        case AttackMode::MODE_1:
            shoot(m_x, m_y, BulletType::BASIC);
            break;
        default:
            break;
        }
        m_pdata->actionData.currentState = ActionState::IDLE;
        m_pdata->actionData.attackMode   = AttackMode::NONE;
        break;
    }
}

void FeilianEnemy::drawRole() {
    if (m_pdata->img != nullptr && m_pdata->isActive && !m_pdata->deathData.isDead) {
        OLED_DrawImage(
            m_pdata->spatialData.currentPosX, m_pdata->spatialData.currentPosY, m_pdata->img, OLED_COLOR_NORMAL
        );
    }

    if (m_pdata->deathData.isDead) {
        // Draw death animation or effect
        // ������������������һ���򵥵�ԲȦ��ʾ��ʧЧ����
        uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
        uint8_t radius  = (feilianEnemyDeadTime - m_pdata->deathData.deathTimer) / 100; // ��0����������ֵ5
        radius          = etl::max(radius, uint8_t(1));                                 // ��С�뾶����

        OLED_DrawCircle(centerX, centerY, radius, OLED_COLOR_NORMAL);
    }
}

void FeilianEnemy::die() {
    // Implement enemy death logic
    if (m_pdata->deathData.deathTimer > 0) {
        m_pdata->deathData.deathTimer -= controlDelayTime;
        m_pdata->deathData.deathTimer = etl::max(m_pdata->deathData.deathTimer, uint16_t(0));
        return;
    }
    m_pdata->isActive = false;
}
/*******************************************************************/

/*******************************************************************/
/**
 * @brief GudiaoEnemy class
 * @note  ���ģ��Ƶ� �� Ӣ�ģ�Gudiao,�񻰵��ʣ�ɽ�����д����������ޣ��Կ����ղ����࣬�ó����в��ԣ���ɽ��ʳ�˶��޵Ĵ�����
 * @note  ���ٷ��е����͵��ˣ����;��У�15x15 ���أ����������ϸߣ��ʺϷ������ҡ�
 * @note  ������ʽΪ�������˺���ͨ�ӵ���ÿ�δ��������෢�䡣
 */
GudiaoEnemy::GudiaoEnemy(
    uint8_t startX, uint8_t startY, uint8_t initPosX, uint8_t initPosY, uint8_t level, uint16_t dropExp
)
: IRole() {
    //ͼƬ��Ϣ
    m_pdata->img = &GudiaoImg;

    //������Ϣ
    m_pdata->identity          = RoleIdentity::ENEMY;
    m_pdata->isActive          = true;
    m_pdata->initData.isInited = false;

    //�ȼ���Ϣ
    m_pdata->level = level;

    //Ѫ����Ϣ
    m_pdata->healthData.currentHealth = 60 + level * 100;
    m_pdata->healthData.maxHealth     = 60 + level * 100;

    //��Ѫ��Ϣ
    m_pdata->healthData.healValue       = 5;
    m_pdata->healthData.healTimeCounter = 0;
    m_pdata->healthData.healResetTime   = 15000;
    m_pdata->healthData.healSpeed       = 5;

    //�ռ��ƶ���Ϣ
    m_pdata->spatialData.canCrossBorder            = false;
    m_pdata->spatialData.currentPosX               = startX; // Starting X position
    m_pdata->spatialData.currentPosY               = startY; // Starting Y position
    m_pdata->spatialData.refPosX                   = startX;
    m_pdata->spatialData.refPosY                   = startY;
    m_pdata->spatialData.sizeX                     = m_pdata->img->w;
    m_pdata->spatialData.sizeY                     = m_pdata->img->h;
    m_pdata->spatialData.moveSpeed                 = 1; // Set movement speed
    m_pdata->spatialData.consecutiveCollisionCount = 0;

    //��ʼ��λ��
    m_pdata->initData.posX = initPosX;
    m_pdata->initData.posY = initPosY;

    //������Ϣ
    m_pdata->attackData.attackPower            = 8 + level * 2;
    m_pdata->attackData.shootCooldownSpeed     = 5;
    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 16000; //32000 ms
    m_pdata->attackData.bulletSpeed            = 1;

    m_pdata->attackData.bulletRange            = 10; //ֻ�Ի�������Ч
    m_pdata->attackData.bulletDamageMultiplier = 1.5f;

    m_pdata->attackData.collisionPower = 4 + level * 1;

    //������Ϣ
    m_pdata->heatData.maxHeat          = 150;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 20;
    m_pdata->heatData.heatCoolDownRate = 10; //ÿ����ȴ10��������ÿ����ȴʱ��������200ms

    //����״̬��Ϣ
    m_pdata->deathData.deathTimer           = gudiaoEnemyDeadTime;
    m_pdata->deathData.isDead               = false;
    m_pdata->deathData.dropExperiencePoints = dropExp;

    // Initialize other enemy-specific data here
}

void GudiaoEnemy::shoot(uint8_t x, uint8_t y, BulletType type) {
    // Implement enemy shooting logic
    // Create bullet based on type
    switch (type) {
    case BulletType::BASIC:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot > m_pdata->heatData.maxHeat)
                return;                                             // ���������������޷�����
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // ��ȴ�У��޷�����

            IBullet *newBullet = createBullet(x, y, BulletType::BASIC);
            if (newBullet != nullptr) {
                if (g_entityManager.addBullet(newBullet)) {
                    // Successfully added bullet
                    m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
                    m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot;
                } else {
                    delete newBullet; // Clean up if not added
                }
            }
        }
        break;
    case BulletType::FIRE_BALL:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 2 > m_pdata->heatData.maxHeat)
                return;                                             // ���������������޷�����
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // ��ȴ�У��޷�����

            IBullet *newBullet = createBullet(x, y, BulletType::FIRE_BALL);
            if (newBullet != nullptr) {
                if (g_entityManager.addBullet(newBullet)) {
                    // Successfully added bullet
                    m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
                    m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot;
                } else {
                    delete newBullet; // Clean up if not added
                }
            }
        }
        break;
    case BulletType::LIGHTNING_LINE:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 1.5 > m_pdata->heatData.maxHeat)
                return;                                             // ���������������޷�����
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // ��ȴ�У��޷�����

            IBullet *newBullet = createBullet(x, y, BulletType::LIGHTNING_LINE);
            if (newBullet != nullptr) {
                if (g_entityManager.addBullet(newBullet)) {
                    // Successfully added bullet
                    m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
                    m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot;
                } else {
                    delete newBullet; // Clean up if not added
                }
            }
        }
    }
}

void GudiaoEnemy::init() {
    m_pdata->initData.init_count += controlDelayTime;
    // Initialize enemy role specifics
    if (m_pdata->initData.init_count < 30) { // ÿ30ms�ƶ�һ��
        return;
    }
    m_pdata->initData.init_count = 0;

    if (m_pdata->spatialData.currentPosX > m_pdata->initData.posX) {
        m_pdata->spatialData.currentPosX -= 1;
    } else if (m_pdata->spatialData.currentPosX < m_pdata->initData.posX) {
        m_pdata->spatialData.currentPosX += 1;
    } else {
        m_pdata->initData.isInited   = true;
        m_pdata->spatialData.refPosX = m_pdata->spatialData.currentPosX;
        m_pdata->spatialData.refPosY = m_pdata->spatialData.currentPosY;
        m_pdata->initData.init_count = 0;
    }
}

void GudiaoEnemy::think() {
    // Implement enemy AI logic
    think_count += controlDelayTime;
    if (think_count < 200) // ÿ200ms����һ���ж�
        return;

    think_count = 0;

    uint8_t randomAction = rand() % 6;
    // Random action: 0 - move left, 1 - move right, 2 - move down, 3 - move up, 4 - stay still, 5 - shoot
    if (m_pdata->actionData.currentState == ActionState::IDLE) {
        //�ƶ�
        if (randomAction == 0) {
            m_pdata->actionData.moveMode     = MoveMode::LEFT;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 1) {
            m_pdata->actionData.moveMode     = MoveMode::RIGHT;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 2) {
            m_pdata->actionData.moveMode     = MoveMode::DOWN;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 3) {
            m_pdata->actionData.moveMode     = MoveMode::UP;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 4) {
            // Stay still
            m_pdata->actionData.moveMode     = MoveMode::NONE;
            m_pdata->actionData.currentState = ActionState::MOVING;
        }

        //����
        else if (randomAction == 5) {
            if (m_pdata->attackData.shootCooldownTimer <= 0) {
                m_pdata->actionData.attackMode   = AttackMode::MODE_1;
                m_pdata->actionData.currentState = ActionState::ATTACKING;
            } else {
                // ��������ȴ�У��򲻽��й��������ֿ���״̬
                m_pdata->actionData.moveMode     = MoveMode::NONE;
                m_pdata->actionData.currentState = ActionState::IDLE;
            }
        }
    }
}

void GudiaoEnemy::doAction() {
    if (m_pdata->initData.isInited == false) {
        return;
    }

    // Implement enemy action logic
    if (m_pdata->deathData.isDead) {
        return;
    }
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
        // Move logic handled in think()
        break;
    case ActionState::ATTACKING:

        //������ȴʱ��
        if (m_pdata->attackData.shootCooldownTimer > 0) break;

        uint8_t m_x = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t m_y = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

        uint8_t m_x_1 = m_x;
        uint8_t m_y_1 = m_y - 3;
        uint8_t m_x_2 = m_x;
        uint8_t m_y_2 = m_y + 3;
        switch (m_pdata->actionData.attackMode) {
        case AttackMode::MODE_1:
            shoot(m_x_1, m_y_1, BulletType::BASIC);
            m_pdata->attackData.shootCooldownTimer = 0; //������ȴʱ�䣬���ٷ����ڶ����ӵ�
            shoot(m_x_2, m_y_2, BulletType::BASIC);
            break;
        default:
            break;
        }
        m_pdata->actionData.currentState = ActionState::IDLE;
        m_pdata->actionData.attackMode   = AttackMode::NONE;
        break;
    }
}

void GudiaoEnemy::drawRole() {
    if (m_pdata->img != nullptr && m_pdata->isActive && !m_pdata->deathData.isDead) {
        OLED_DrawImage(
            m_pdata->spatialData.currentPosX, m_pdata->spatialData.currentPosY, m_pdata->img, OLED_COLOR_NORMAL
        );
    }

    if (m_pdata->deathData.isDead) {
        // Draw death animation or effect
        // ������������������һ���򵥵�ԲȦ��ʾ��ʧЧ����
        uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
        uint8_t radius  = (gudiaoEnemyDeadTime - m_pdata->deathData.deathTimer) / 100; // ��0����������ֵ5
        radius          = etl::max(radius, uint8_t(1));                                // ��С�뾶����

        OLED_DrawCircle(centerX, centerY, radius, OLED_COLOR_NORMAL);
    }
}

void GudiaoEnemy::die() {
    // Implement enemy death logic
    if (m_pdata->deathData.deathTimer > 0) {
        m_pdata->deathData.deathTimer -= controlDelayTime;
        m_pdata->deathData.deathTimer = etl::max(m_pdata->deathData.deathTimer, uint16_t(0));
        return;
    }

    //�������һ��������
    IBullet *newBullet = createBullet(
        m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2,
        m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2, BulletType::FIRE_BALL
    );
    if (newBullet != nullptr) {
        if (!g_entityManager.addBullet(newBullet)) {
            delete newBullet; // Clean up if not added
        }
    }
    m_pdata->isActive = false;
}

/*******************************************************************/

/*******************************************************************/
/**
 * @brief ChiMeiEnemy class
 * @note  ���ģ����� �� Ӣ�ģ�ChiMei,�񻰵��ʣ���˵��Ϣ��ɽ�ּ������֣������Ի����ģ�������·����������ɽ�֣����ս������ɡ�
 * @note  �����ƶ���С�͵��ˣ����ͼ�С��8x8 ���أ�����ɱʽ��ײ��
 */

ChiMeiEnemy::ChiMeiEnemy(
    uint8_t startX, uint8_t startY, uint8_t initPosX, uint8_t initPosY, uint8_t level, uint16_t dropExp
)
: IRole() {
    //ͼƬ��Ϣ
    m_pdata->img = &ChiMeiImg;

    //������Ϣ
    m_pdata->identity          = RoleIdentity::ENEMY;
    m_pdata->isActive          = true;
    m_pdata->initData.isInited = false;

    //�ȼ���Ϣ
    m_pdata->level = level;

    //Ѫ����Ϣ
    m_pdata->healthData.currentHealth = 1 + level * 1;
    m_pdata->healthData.maxHealth     = 1 + level * 1;

    //��Ѫ��Ϣ
    m_pdata->healthData.healValue       = 0;
    m_pdata->healthData.healTimeCounter = 0;
    m_pdata->healthData.healResetTime   = 15000;
    m_pdata->healthData.healSpeed       = 0;

    //�ռ��ƶ���Ϣ
    m_pdata->spatialData.canCrossBorder            = false;
    m_pdata->spatialData.currentPosX               = startX; // Starting X position
    m_pdata->spatialData.currentPosY               = startY; // Starting Y position
    m_pdata->spatialData.refPosX                   = startX;
    m_pdata->spatialData.refPosY                   = startY;
    m_pdata->spatialData.sizeX                     = m_pdata->img->w;
    m_pdata->spatialData.sizeY                     = m_pdata->img->h;
    m_pdata->spatialData.moveSpeed                 = 3; // Set movement speed
    m_pdata->spatialData.consecutiveCollisionCount = 0;

    //��ʼ��λ��
    m_pdata->initData.posX = initPosX;
    m_pdata->initData.posY = initPosY;

    //������Ϣ
    m_pdata->attackData.attackPower            = 1 + level * 1;
    m_pdata->attackData.shootCooldownSpeed     = 5;
    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 4000;
    m_pdata->attackData.bulletSpeed            = 2;

    m_pdata->attackData.bulletRange            = 0;    //ֻ�Ի�������Ч
    m_pdata->attackData.bulletDamageMultiplier = 1.5f; //ֻ������������Ч

    m_pdata->attackData.collisionPower = 20;

    //������Ϣ
    m_pdata->heatData.maxHeat          = 100;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 20;
    m_pdata->heatData.heatCoolDownRate = 10; //ÿ����ȴ10��������ÿ����ȴʱ��������200ms

    //����״̬��Ϣ
    m_pdata->deathData.deathTimer           = chimeiEnemyDeadTime;
    m_pdata->deathData.isDead               = false;
    m_pdata->deathData.dropExperiencePoints = dropExp;

    // Initialize other enemy-specific data here
}

void ChiMeiEnemy::shoot(uint8_t x, uint8_t y, BulletType type) {
    // Implement enemy shooting logic
    // Create bullet based on type
    switch (type) {
    case BulletType::BASIC:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot > m_pdata->heatData.maxHeat)
                return;                                             // ���������������޷�����
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // ��ȴ�У��޷�����

            IBullet *newBullet = createBullet(x, y, BulletType::BASIC);
            if (newBullet != nullptr) {
                if (g_entityManager.addBullet(newBullet)) {
                    // Successfully added bullet
                    m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
                    m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot;
                } else {
                    delete newBullet; // Clean up if not added
                }
            }
        }
        break;
    case BulletType::FIRE_BALL:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 2 > m_pdata->heatData.maxHeat)
                return;                                             // ���������������޷�����
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // ��ȴ�У��޷�����

            IBullet *newBullet = createBullet(x, y, BulletType::FIRE_BALL);
            if (newBullet != nullptr) {
                if (g_entityManager.addBullet(newBullet)) {
                    // Successfully added bullet
                    m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
                    m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot;
                } else {
                    delete newBullet; // Clean up if not added
                }
            }
        }
        break;
    case BulletType::LIGHTNING_LINE:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 1.5 > m_pdata->heatData.maxHeat)
                return;                                             // ���������������޷�����
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // ��ȴ�У��޷�����

            IBullet *newBullet = createBullet(x, y, BulletType::LIGHTNING_LINE);
            if (newBullet != nullptr) {
                if (g_entityManager.addBullet(newBullet)) {
                    // Successfully added bullet
                    m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
                    m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot;
                } else {
                    delete newBullet; // Clean up if not added
                }
            }
        }
    }
}

void ChiMeiEnemy::init() {
    m_pdata->initData.init_count += controlDelayTime;
    // Initialize enemy role specifics

    if (m_pdata->spatialData.currentPosX > m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 30) { // ÿ30ms�ƶ�һ��
            m_pdata->spatialData.currentPosX -= 1;
            m_pdata->initData.init_count = 0;
        }
    } else if (m_pdata->spatialData.currentPosX < m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 30) { // ÿ30ms�ƶ�һ��
            m_pdata->spatialData.currentPosX += 1;
            m_pdata->initData.init_count = 0;
        }
    } else {
        m_pdata->initData.isInited   = true;
        m_pdata->spatialData.refPosX = m_pdata->spatialData.currentPosX;
        m_pdata->spatialData.refPosY = m_pdata->spatialData.currentPosY;
        m_pdata->initData.init_count = 0;
    }
}

void ChiMeiEnemy::think() {
    // Implement enemy AI logic
    think_count += controlDelayTime;
    if (think_count < 150) // ÿ150ms����һ���ж�
        return;

    think_count = 0;

    //����ֻ�������ƶ�
    if (m_pdata->actionData.currentState == ActionState::IDLE) {
        //�ƶ�
        m_pdata->actionData.moveMode     = MoveMode::LEFT;
        m_pdata->actionData.currentState = ActionState::MOVING;
    }
}

void ChiMeiEnemy::doAction() {
    if (m_pdata->initData.isInited == false) {
        return;
    }

    //���ȵ������߽���������
    if (m_pdata->spatialData.currentPosX <= boundary_deadzone) {
        m_pdata->deathData.isDead = true;
    }

    // Implement enemy action logic
    if (m_pdata->deathData.isDead) {
        return;
    }
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
        // Move logic handled in think()
        break;
    case ActionState::ATTACKING:
        uint8_t m_x = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t m_y = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
        switch (m_pdata->actionData.attackMode) {
        case AttackMode::MODE_1:
            shoot(m_x, m_y, BulletType::BASIC);
            break;
        default:
            break;
        }
        m_pdata->actionData.currentState = ActionState::IDLE;
        m_pdata->actionData.attackMode   = AttackMode::NONE;
        break;
    }
}

void ChiMeiEnemy::drawRole() {
    if (m_pdata->img != nullptr && m_pdata->isActive && !m_pdata->deathData.isDead) {
        OLED_DrawImage(
            m_pdata->spatialData.currentPosX, m_pdata->spatialData.currentPosY, m_pdata->img, OLED_COLOR_NORMAL
        );
    }

    if (m_pdata->deathData.isDead) {
        // Draw death animation or effect
        // ������������������һ���򵥵�ԲȦ��ʾ��ʧЧ����
        uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
        uint8_t radius  = (chimeiEnemyDeadTime - m_pdata->deathData.deathTimer) / 100; // ��0����������ֵ5
        radius          = etl::max(radius, uint8_t(1));                                // ��С�뾶����

        OLED_DrawCircle(centerX, centerY, radius, OLED_COLOR_NORMAL);
    }
}

void ChiMeiEnemy::die() {
    // Implement enemy death logic
    if (m_pdata->deathData.deathTimer > 0) {
        m_pdata->deathData.deathTimer -= controlDelayTime;
        m_pdata->deathData.deathTimer = etl::max(m_pdata->deathData.deathTimer, uint16_t(0));
        return;
    }
    m_pdata->isActive = false;
}

/*******************************************************************/

/*******************************************************************/
/**
 * @brief BoEnemy class - �� ��Ӣ����
 * @note  ���ģ��� �� Ӣ�ģ�Bo
 * @note  �񻰵��ʣ���ɽ��������ɽ�������أ�"����֮ɽ�������ɣ���״������������β��һ�ǣ�����צ��
 *                ��������������Ի������ʳ����������������"
 * @note  ����һ���������ޣ�������β��ͷ�����ǣ��л�����צ���������ģ��ܲ�ʳ��������������������
 * 
 * @note  ��Ӣ�����͵��ˣ������еȣ�24x24 ���أ����е�Ѫ�����ϸ߹������������ƶ���
 * @note  ������ͨ����һͬ���֣�������ʽֱ�����ͣ���˲�Ƽ��ܡ�
 * 
 * @note  === ������ʽ ===
 * @note  MODE_1: ������̤ - �����ҷ�������ֱ�߳��棬������ײ�˺�
 *               �������� ChargeDistance=40����������ʱ�� ChargeTime=800ms
 * @note  MODE_2: ������צ - ��Ӧ"����צ"������3�������ε���ͨ�ӵ�
 *               ����ʱ�� ClawAttackTime=200ms
 * @note  MODE_3: �������� - ��Ӧ"��������"������һ�ź����������ӵ�
 *               ����ʱ�� DrumSoundTime=300ms
 */

//��ֵ�趨�ο�����Ӣ��������ͨ����ǿ����BOSS����
//Ѫ����80 + level * 80���е�Ѫ����
//��������6 + level * 2���ϸ߹�������
//�ƶ��ٶȣ�2�������ƶ���
//��ײ�˺���10 + level * 3���ϸ���ײ�˺���

BoEnemy::BoEnemy(uint8_t startX, uint8_t startY, uint8_t initPosX, uint8_t initPosY, uint8_t level, uint16_t dropExp)
: IRole() {
    //ͼƬ��Ϣ����Ҫ��font.c������BoImg��
    m_pdata->img = &BoImg;

    //������Ϣ
    m_pdata->identity          = RoleIdentity::ENEMY;
    m_pdata->isActive          = true;
    m_pdata->initData.isInited = false;

    //�ȼ���Ϣ
    m_pdata->level = level;

    //Ѫ����Ϣ
    m_pdata->healthData.currentHealth = 100 + level * 140;
    m_pdata->healthData.maxHealth     = 100 + level * 140;

    //��Ѫ��Ϣ
    m_pdata->healthData.healValue       = 2;
    m_pdata->healthData.healTimeCounter = 0;
    m_pdata->healthData.healResetTime   = 10000;
    m_pdata->healthData.healSpeed       = 2;

    //�ռ��ƶ���Ϣ
    m_pdata->spatialData.canCrossBorder            = false;
    m_pdata->spatialData.currentPosX               = startX;
    m_pdata->spatialData.currentPosY               = startY;
    m_pdata->spatialData.refPosX                   = startX;
    m_pdata->spatialData.refPosY                   = startY;
    m_pdata->spatialData.sizeX                     = m_pdata->img->w;
    m_pdata->spatialData.sizeY                     = m_pdata->img->h;
    m_pdata->spatialData.moveSpeed                 = 2; // �����ƶ�
    m_pdata->spatialData.consecutiveCollisionCount = 0;

    //��ʼ��λ��
    m_pdata->initData.posX = initPosX;
    m_pdata->initData.posY = initPosY;

    //������Ϣ
    m_pdata->attackData.attackPower            = 6 + level * 2;
    m_pdata->attackData.shootCooldownSpeed     = 5;
    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 8000; // 8000/5=1600ms��ȴ
    m_pdata->attackData.bulletSpeed            = 1;

    m_pdata->attackData.bulletRange            = 8; // ���򵯷�Χ
    m_pdata->attackData.bulletDamageMultiplier = 1.5f;

    m_pdata->attackData.collisionPower = 10 + level * 3; // �ϸ���ײ�˺�

    //������Ϣ
    m_pdata->heatData.maxHeat          = 120;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 15;
    m_pdata->heatData.heatCoolDownRate = 10;

    //����״̬��Ϣ
    m_pdata->deathData.deathTimer           = boEnemyDeadTime;
    m_pdata->deathData.isDead               = false;
    m_pdata->deathData.dropExperiencePoints = dropExp;

    // ��ʼ������ģʽ״̬����
    chargeStarted    = false;
    chargeDirectionX = 0;
    chargeDirectionY = 0;
}

void BoEnemy::shoot(uint8_t x, uint8_t y, BulletType type) {
    switch (type) {
    case BulletType::BASIC:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot > m_pdata->heatData.maxHeat) return;
            if (m_pdata->attackData.shootCooldownTimer > 0) return;

            IBullet *newBullet = createBullet(x, y, BulletType::BASIC);
            if (newBullet != nullptr) {
                taskENTER_CRITICAL();
                g_entityManager.addBullet(newBullet);
                taskEXIT_CRITICAL();
                m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot;
                m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
            }
        }
        break;
    case BulletType::FIRE_BALL:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 2 > m_pdata->heatData.maxHeat) return;
            if (m_pdata->attackData.shootCooldownTimer > 0) return;

            IBullet *newBullet = createBullet(x, y, BulletType::FIRE_BALL);
            if (newBullet != nullptr) {
                taskENTER_CRITICAL();
                g_entityManager.addBullet(newBullet);
                taskEXIT_CRITICAL();
                m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot * 2;
                m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
            }
        }
        break;
    case BulletType::LIGHTNING_LINE:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 1.5 > m_pdata->heatData.maxHeat) return;
            if (m_pdata->attackData.shootCooldownTimer > 0) return;

            IBullet *newBullet = createBullet(x, y, BulletType::LIGHTNING_LINE);
            if (newBullet != nullptr) {
                taskENTER_CRITICAL();
                g_entityManager.addBullet(newBullet);
                taskEXIT_CRITICAL();
                m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot * 1.5;
                m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
            }
        }
        break;
    }
}

void BoEnemy::init() {
    m_pdata->initData.init_count += controlDelayTime;

    // ���Ҳ��볡���ƶ�����ʼλ��
    if (m_pdata->spatialData.currentPosX > m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 20) { // ÿ20ms�ƶ�һ��
            m_pdata->spatialData.currentPosX -= 1;
            m_pdata->initData.init_count = 0;
        }
    } else if (m_pdata->spatialData.currentPosX < m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 20) { // ÿ20ms�ƶ�һ��
            m_pdata->spatialData.currentPosX += 1;
            m_pdata->initData.init_count = 0;
        }
    } else {
        m_pdata->initData.isInited       = true;
        m_pdata->spatialData.refPosX     = m_pdata->spatialData.currentPosX;
        m_pdata->spatialData.refPosY     = m_pdata->spatialData.currentPosY;
        m_pdata->actionData.currentState = ActionState::IDLE;
        m_pdata->initData.init_count     = 0;
    }
}

void BoEnemy::think() {
    think_count += controlDelayTime;
    if (think_count < 150) // ÿ150ms˼��һ��
        return;

    think_count = 0;

    if (m_pdata->actionData.currentState == ActionState::IDLE) {
        // ��ȡ����λ�����ھ���
        IRole *player = g_entityManager.getPlayerRole();
        (void)player; // ����δʹ�þ���

        uint8_t randomAction = rand() % 10;

        if (randomAction < 3) {
            // 30% �����ƶ�
            uint8_t moveDir                  = rand() % 4;
            m_pdata->actionData.currentState = ActionState::MOVING;
            switch (moveDir) {
            case 0:
                m_pdata->actionData.moveMode = MoveMode::LEFT;
                break;
            case 1:
                m_pdata->actionData.moveMode = MoveMode::RIGHT;
                break;
            case 2:
                m_pdata->actionData.moveMode = MoveMode::UP;
                break;
            case 3:
                m_pdata->actionData.moveMode = MoveMode::DOWN;
                break;
            }
        } else if (randomAction < 5) {
            // 20% ���ʴ���
            m_pdata->actionData.currentState = ActionState::IDLE;
        } else {
            // 50% ���ʹ���
            m_pdata->actionData.currentState = ActionState::ATTACKING;

            uint8_t attackChoice = rand() % 6;
            switch (attackChoice) {
            case 0:
            case 1:
                // MODE_1: ������̤ (Լ33%)
                action_timer                   = ChargeTime;
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                chargeStarted                  = false;
                m_pdata->actionData.attackMode = AttackMode::MODE_1;
                break;
            case 2:
            case 3:
                // MODE_2: ������צ (Լ33%)
                action_timer                   = ClawAttackTime;
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                m_pdata->actionData.attackMode = AttackMode::MODE_2;
                break;
            case 4:
            case 5:
                // MODE_3: �������� (Լ33%)
                action_timer                   = DrumSoundTime;
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                m_pdata->actionData.attackMode = AttackMode::MODE_3;
                break;
            }
        }
    }
}

void BoEnemy::doAction() {
    if (m_pdata->initData.isInited == false) {
        return;
    }

    if (m_pdata->deathData.isDead) {
        return;
    }

    switch (m_pdata->actionData.currentState) {
    case ActionState::IDLE:
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

    case ActionState::ATTACKING:
        // ���¼�����
        action_count += controlDelayTime;

        // ��������ʱ
        if (action_timer >= controlDelayTime)
            action_timer -= controlDelayTime;
        else
            action_timer = 0;

        // ִ�ж�Ӧ��������
        switch (m_pdata->actionData.attackMode) {
        case AttackMode::MODE_1:
            chargeTowardsPlayer();
            break;
        case AttackMode::MODE_2:
            tigerClawAttack();
            break;
        case AttackMode::MODE_3:
            drumSoundWave();
            break;
        default:
            break;
        }

        // ��������
        if (action_timer == 0) {
            m_pdata->actionData.currentState       = ActionState::IDLE;
            m_pdata->actionData.attackMode         = AttackMode::NONE;
            action_count                           = 0;
            m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;

            // ����״̬����
            chargeStarted = false;
        }
        break;
    }
}

void BoEnemy::drawRole() {
    if (m_pdata->img != nullptr && m_pdata->isActive && !m_pdata->deathData.isDead) {
        OLED_DrawImage(
            m_pdata->spatialData.currentPosX, m_pdata->spatialData.currentPosY, m_pdata->img, OLED_COLOR_NORMAL
        );

        // MODE_1����ʱ���Ƴ�����Ч����β������
        if (m_pdata->actionData.attackMode == AttackMode::MODE_1 && chargeStarted) {
            uint8_t tailX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX;
            uint8_t tailY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
            // ���������ƶ��߱�ʾ������β
            OLED_DrawLine(
                tailX - chargeDirectionX * 0, tailY - chargeDirectionY * 0, tailX - chargeDirectionX * 8,
                tailY - chargeDirectionY * 4, OLED_COLOR_NORMAL
            );
        }
    }

    // ��������
    if (m_pdata->deathData.isDead) {
        uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
        uint8_t radius  = (boEnemyDeadTime - m_pdata->deathData.deathTimer) * 15 / boEnemyDeadTime;
        radius          = etl::max(radius, uint8_t(1));

        // ������Ч������ɢԲ��
        OLED_DrawCircle(centerX, centerY, radius, OLED_COLOR_NORMAL);
    }
}

void BoEnemy::die() {
    if (m_pdata->deathData.deathTimer > 0) {
        m_pdata->deathData.deathTimer -= controlDelayTime;
        m_pdata->deathData.deathTimer = etl::max(m_pdata->deathData.deathTimer, uint16_t(0));
        return;
    }
    m_pdata->isActive = false;
}

//=========================== ��������ʵ�� ===========================

/**
 * @brief MODE_1: ������̤
 * @note  �����ҷ�������ֱ�߳��棬��Ӧ�������޵�����
 *        �������������ɸ���ײ�˺�
 */
void BoEnemy::chargeTowardsPlayer() {
    // ���濪ʼʱ���㷽��
    static int16_t safe_dis = 40; // ��ȫ���룬��������
    if (!chargeStarted) {
        chargeStarted = true;

        IRole *player = g_entityManager.getPlayerRole();
        if (player != nullptr) {
            safe_dis = player->getData()->spatialData.sizeX / 2 + m_pdata->spatialData.sizeX / 2 + 50;
            int16_t playerX =
                player->getData()->spatialData.currentPosX + player->getData()->spatialData.sizeX / 2 + safe_dis;
            int16_t playerY = player->getData()->spatialData.currentPosY + player->getData()->spatialData.sizeY / 2;
            int16_t myX     = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
            int16_t myY     = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

            int16_t deltaX = playerX - myX;
            int16_t deltaY = playerY - myY;

            // �������淽���������ҷ�����
            chargeDirectionX = (deltaX > 0) ? 1 : -1; // deltaX > 0 ��ʾ�������ұߣ����ҳ���
            chargeDirectionY = 0;
            if (deltaY < -5)
                chargeDirectionY = -1;
            else if (deltaY > 5)
                chargeDirectionY = 1;

            if (deltaX == 0) {
                chargeDirectionX = 0;
            }
            if (deltaY == 0) {
                chargeDirectionY = 0;
            }

        } else {
            // ������ʱ��������
            chargeDirectionX = -1;
            chargeDirectionY = 0;
        }
    }

    // ÿ20ms�ƶ�һ�Σ����ٳ��棩
    if (action_count >= 30) {
        action_count = 0;
        // �����ƶ����ٶȷ�����
        move(chargeDirectionX * 2, chargeDirectionY, true);
    }
}

/**
 * @brief MODE_2: ������צ
 * @note  ����3�������ε���ͨ�ӵ�����Ӧ"����צ"
 *        һ���Է��䣬���ǽϴ���Χ
 */
void BoEnemy::tigerClawAttack() {
    // ֻ�ڼ��ܿ�ʼʱ����һ��
    if (action_count < 50) return;
    if (action_timer < action_MaxTime - 100) return; // ֻ�ڿ�ʼ100ms�ڴ���

    uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

    // ����3�������ӵ����м䡢��ƫ����ƫ��
    m_pdata->attackData.shootCooldownTimer = 0;
    shoot(centerX, centerY, BulletType::BASIC); // �м�
    m_pdata->attackData.shootCooldownTimer = 0;
    shoot(centerX, centerY - 3, BulletType::BASIC); // ��ƫ
    m_pdata->attackData.shootCooldownTimer = 0;
    shoot(centerX, centerY + 3, BulletType::BASIC); // ��ƫ
}

/**
 * @brief MODE_3: ��������
 * @note  ����һ�ź����������ӵ�����Ӧ"��������"
 *        ���ǽϿ���Y�᷶Χ
 */
void BoEnemy::drumSoundWave() {
    // ֻ�ڼ��ܿ�ʼʱ����һ��
    if (action_count < 50) return;
    if (action_timer < action_MaxTime - 150) return; // ֻ�ڿ�ʼ150ms�ڴ���

    uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

    // ����һ��5���ӵ���������������
    for (int8_t offset = -10; offset <= 10; offset += 5) {
        m_pdata->attackData.shootCooldownTimer = 0;
        shoot(centerX, centerY + offset, BulletType::BASIC);
    }
}

/*******************************************************************/
/**
 * @brief ShengyuEnemy class - ʤ�� ��Ӣ���ˣ�ˮ�������ͣ�
 * @note  ���ģ�ʤ�� �� Ӣ�ģ�Shengyu
 * @note  �񻰵��ʣ���ɫҰ��״���ޣ��������أ����ּ�������ˮ�ֶ���
 * @note  ����������"ˮ"�󶨣������Ի����뷶Χѹ������
 * @note  ˮ�����������ɶ������������ڵ�����
 * @note  �鲨��ӿ����ǰ�Ƴ������������ӷ���λ��������ɢ
 * @note  ��������������һ���׵��ӵ�
 */
/*******************************************************************/

ShengyuEnemy::ShengyuEnemy(
    uint8_t startX, uint8_t startY, uint8_t initPosX, uint8_t initPosY, uint8_t level, uint16_t dropExp
)
: IRole() {
    //ͼƬ��Ϣ����Ҫ��font.c������ShengyuImg��
    m_pdata->img = &ShengyuImg;

    //������Ϣ
    m_pdata->identity          = RoleIdentity::ENEMY;
    m_pdata->isActive          = true;
    m_pdata->initData.isInited = false;

    //�ȼ���Ϣ
    m_pdata->level = level;

    //Ѫ����Ϣ: 50 + level * 50���ϵ�Ѫ����ƫ��Ƥ��
    m_pdata->healthData.currentHealth = 50 + level * 150;
    m_pdata->healthData.maxHealth     = 50 + level * 150;

    //��Ѫ��Ϣ
    m_pdata->healthData.healValue       = 1;
    m_pdata->healthData.healTimeCounter = 0;
    m_pdata->healthData.healResetTime   = 12000;
    m_pdata->healthData.healSpeed       = 2;

    //�ռ��ƶ���Ϣ
    m_pdata->spatialData.canCrossBorder            = false;
    m_pdata->spatialData.currentPosX               = startX;
    m_pdata->spatialData.currentPosY               = startY;
    m_pdata->spatialData.refPosX                   = startX;
    m_pdata->spatialData.refPosY                   = startY;
    m_pdata->spatialData.sizeX                     = m_pdata->img->w;
    m_pdata->spatialData.sizeY                     = m_pdata->img->h;
    m_pdata->spatialData.moveSpeed                 = 2; // �����ƶ�
    m_pdata->spatialData.consecutiveCollisionCount = 0;

    //��ʼ��λ��
    m_pdata->initData.posX = initPosX;
    m_pdata->initData.posY = initPosY;

    //������Ϣ: 4 + level * 1���ϵ͹���������Ҫ�����ţ�
    m_pdata->attackData.attackPower            = 4 + level * 2;
    m_pdata->attackData.shootCooldownSpeed     = 4;
    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 6000; // 6000/4=1500ms��ȴ
    m_pdata->attackData.bulletSpeed            = 1;

    m_pdata->attackData.bulletRange            = 6;    // ���򵯷�Χ
    m_pdata->attackData.bulletDamageMultiplier = 1.5f; // ���������˺�����

    //��ײ�˺�: 6 + level * 2���ϵ���ײ�˺���
    m_pdata->attackData.collisionPower = 6 + level * 2;

    //������Ϣ
    m_pdata->heatData.maxHeat          = 80;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 10;
    m_pdata->heatData.heatCoolDownRate = 12;

    //����״̬��Ϣ
    m_pdata->deathData.deathTimer           = shengyuEnemyDeadTime;
    m_pdata->deathData.isDead               = false;
    m_pdata->deathData.dropExperiencePoints = dropExp;

    // ��ʼ������ģʽ״̬����
    mistGenerated        = false;
    floodWaveLaunched    = false;
    floodWaveCurrentLen  = 0;
    thunderFiredCount    = 0;
    for (uint8_t i = 0; i < MistCloudCount; i++) {
        mistPosX[i] = 0;
        mistPosY[i] = 0;
    }
}

void ShengyuEnemy::shoot(uint8_t x, uint8_t y, BulletType type) {
    switch (type) {
    case BulletType::BASIC:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot > m_pdata->heatData.maxHeat) return;
            if (m_pdata->attackData.shootCooldownTimer > 0) return;

            IBullet *newBullet = createBullet(x, y, BulletType::BASIC);
            if (newBullet != nullptr) {
                taskENTER_CRITICAL();
                g_entityManager.addBullet(newBullet);
                taskEXIT_CRITICAL();
                m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot;
                m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
            }
        }
        break;
    case BulletType::FIRE_BALL:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 2 > m_pdata->heatData.maxHeat) return;
            if (m_pdata->attackData.shootCooldownTimer > 0) return;

            IBullet *newBullet = createBullet(x, y, BulletType::FIRE_BALL);
            if (newBullet != nullptr) {
                taskENTER_CRITICAL();
                g_entityManager.addBullet(newBullet);
                taskEXIT_CRITICAL();
                m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot * 2;
                m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
            }
        }
        break;
    case BulletType::LIGHTNING_LINE:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 1.5 > m_pdata->heatData.maxHeat) return;
            if (m_pdata->attackData.shootCooldownTimer > 0) return;

            IBullet *newBullet = createBullet(x, y, BulletType::LIGHTNING_LINE);
            if (newBullet != nullptr) {
                taskENTER_CRITICAL();
                g_entityManager.addBullet(newBullet);
                taskEXIT_CRITICAL();
                m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot * 1.5;
                m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
            }
        }
        break;
    }
}

void ShengyuEnemy::init() {
    m_pdata->initData.init_count += controlDelayTime;

    // ���Ҳ��볡���ƶ�����ʼλ��
    if (m_pdata->spatialData.currentPosX > m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 25) {
            m_pdata->spatialData.currentPosX -= 1;
            m_pdata->initData.init_count = 0;
        }
    } else if (m_pdata->spatialData.currentPosX < m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 25) {
            m_pdata->spatialData.currentPosX += 1;
            m_pdata->initData.init_count = 0;
        }
    } else {
        m_pdata->initData.isInited       = true;
        m_pdata->spatialData.refPosX     = m_pdata->spatialData.currentPosX;
        m_pdata->spatialData.refPosY     = m_pdata->spatialData.currentPosY;
        m_pdata->actionData.currentState = ActionState::IDLE;
        m_pdata->initData.init_count     = 0;
    }
}

void ShengyuEnemy::think() {
    think_count += controlDelayTime;
    if (think_count < 350) return;  // 350ms˼�������������͵��˽���������

    think_count = 0;

    if (m_pdata->actionData.currentState == ActionState::IDLE) {
        uint8_t randomAction = rand() % 10;

        if (randomAction < 4) {  // 40%�����ƶ�
            uint8_t moveDir                  = rand() % 4;
            m_pdata->actionData.currentState = ActionState::MOVING;
            switch (moveDir) {
            case 0:
                m_pdata->actionData.moveMode = MoveMode::LEFT;
                break;
            case 1:
                m_pdata->actionData.moveMode = MoveMode::RIGHT;
                break;
            case 2:
                m_pdata->actionData.moveMode = MoveMode::UP;
                break;
            case 3:
                m_pdata->actionData.moveMode = MoveMode::DOWN;
                break;
            }
        } else if (randomAction < 6) {  // 20%���ʴ���
            m_pdata->actionData.currentState = ActionState::IDLE;
        } else {  // 40%���ʹ���
            m_pdata->actionData.currentState = ActionState::ATTACKING;

            uint8_t attackChoice = rand() % 10;
            if (attackChoice < 4) {  // 40%���� MODE_1
                action_timer                   = MistCloudTime;
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                mistGenerated                  = false;
                m_pdata->actionData.attackMode = AttackMode::MODE_1;
            } else if (attackChoice < 7) {  // 30%���� MODE_2
                action_timer                   = FloodWaveTime;
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                floodWaveLaunched              = false;
                floodWaveCurrentLen            = 0;
                m_pdata->actionData.attackMode = AttackMode::MODE_2;
            } else {  // 30%���� MODE_3
                action_timer                   = RedThunderTime;
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                thunderFiredCount              = 0;
                m_pdata->actionData.attackMode = AttackMode::MODE_3;
            }
        }
    }
}

void ShengyuEnemy::doAction() {
    if (!m_pdata->initData.isInited || m_pdata->deathData.isDead) return;

    switch (m_pdata->actionData.currentState) {
    case ActionState::IDLE:
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

    case ActionState::ATTACKING:
        action_count += controlDelayTime;

        if (action_timer >= controlDelayTime)
            action_timer -= controlDelayTime;
        else
            action_timer = 0;

        switch (m_pdata->actionData.attackMode) {
        case AttackMode::MODE_1:
            mistCloud();
            break;
        case AttackMode::MODE_2:
            floodWave();
            break;
        case AttackMode::MODE_3:
            redThunder();
            break;
        default:
            break;
        }

        // ��������
        if (action_timer == 0) {
            m_pdata->actionData.currentState       = ActionState::IDLE;
            m_pdata->actionData.attackMode         = AttackMode::NONE;
            action_count                           = 0;
            m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;

            // ����״̬����
            mistGenerated        = false;
            floodWaveLaunched    = false;
            floodWaveCurrentLen  = 0;
            thunderFiredCount    = 0;
        }
        break;
    default:
        break;
    }
}

void ShengyuEnemy::drawRole() {
    if (m_pdata->img != nullptr && m_pdata->isActive && !m_pdata->deathData.isDead) {
        OLED_DrawImage(
            m_pdata->spatialData.currentPosX, m_pdata->spatialData.currentPosY, m_pdata->img, OLED_COLOR_NORMAL
        );

        // MODE_1 �����������򣨽�ʵ��20x20�� + ��̬װ�Σ�
        if (m_pdata->actionData.attackMode == AttackMode::MODE_1 && mistGenerated) {
            uint8_t flickerPhase = (action_count / 120) % 3; // ��˸��λ��3���л�
            
            for (uint8_t i = 0; i < MistCloudCount; i++) {
                uint8_t px = mistPosX[i];
                uint8_t py = mistPosY[i];
                
                // ���ƽ�ʵ�����䣨�ܼ����ߣ���1���ؼ�϶����������
                for (uint8_t dy = 0; dy < MistSize; dy++) {
                    // ������˸��λ������Щ�л���
                    if (dy % 3 != flickerPhase) {
                        OLED_DrawLine(px, py + dy, px + MistSize - 1, py + dy, OLED_COLOR_NORMAL);
                    }
                }
                
                // �Ľ�װ�Σ���̬��˸�ĽǱ꣬����20x20��
                if (flickerPhase != 0) {
                    // ���Ͻ�
                    OLED_DrawLine(px, py, px + 3, py, OLED_COLOR_NORMAL);
                    OLED_DrawLine(px, py, px, py + 3, OLED_COLOR_NORMAL);
                    // ���Ͻ�
                    OLED_DrawLine(px + MistSize - 4, py, px + MistSize - 1, py, OLED_COLOR_NORMAL);
                    OLED_DrawLine(px + MistSize - 1, py, px + MistSize - 1, py + 3, OLED_COLOR_NORMAL);
                    // ���½�
                    OLED_DrawLine(px, py + MistSize - 1, px + 3, py + MistSize - 1, OLED_COLOR_NORMAL);
                    OLED_DrawLine(px, py + MistSize - 4, px, py + MistSize - 1, OLED_COLOR_NORMAL);
                    // ���½�
                    OLED_DrawLine(px + MistSize - 4, py + MistSize - 1, px + MistSize - 1, py + MistSize - 1, OLED_COLOR_NORMAL);
                    OLED_DrawLine(px + MistSize - 1, py + MistSize - 4, px + MistSize - 1, py + MistSize - 1, OLED_COLOR_NORMAL);
                }
                
                // ����ʮ���ƣ������������ظУ�����20x20��
                if (flickerPhase == 1) {
                    uint8_t cx = px + MistSize / 2;
                    uint8_t cy = py + MistSize / 2;
                    OLED_DrawLine(cx - 3, cy, cx + 3, cy, OLED_COLOR_NORMAL);
                    OLED_DrawLine(cx, cy - 3, cx, cy + 3, OLED_COLOR_NORMAL);
                }
            }
        }

        // MODE_2 ���ƺ鲨������������ǰ�ƽ��ڵ�Ч�������㲨��������
        if (m_pdata->actionData.attackMode == AttackMode::MODE_2 && floodWaveLaunched) {
            if (floodWaveCurrentLen > 0) {
                // ��ʤ��λ��������ǰ��������
                uint8_t drawEndX = floodWaveEndX;
                uint8_t drawStartX = (drawEndX > floodWaveCurrentLen) ? (drawEndX - floodWaveCurrentLen) : 0;
                uint8_t wavePhase = (action_count / 80) % 4; // ������λ
                
                // �ϱ߿�������
                OLED_DrawLine(drawStartX, floodWaveY, drawEndX, floodWaveY, OLED_COLOR_NORMAL);
                // �±߿�������
                OLED_DrawLine(drawStartX, floodWaveY + FloodWaveHeight - 1, drawEndX, floodWaveY + FloodWaveHeight - 1, OLED_COLOR_NORMAL);
                
                // �ڲ���̬ˮ���ƣ����㽻�棩
                for (uint8_t dy = 2; dy < FloodWaveHeight - 2; dy++) {
                    // ����ʱ����Yλ�ò�����̬����Ч��
                    uint8_t lineOffset = ((dy + wavePhase) % 4);
                    if (lineOffset < 2) {
                        // �����������ߣ�ʵ�߶Σ�
                        OLED_DrawLine(drawStartX, floodWaveY + dy, drawEndX, floodWaveY + dy, OLED_COLOR_NORMAL);
                    } else if (lineOffset == 2) {
                        // ���ƴβ��ƣ���������Ч�������߶Σ�
                        for (uint8_t sx = drawStartX; sx < drawEndX; sx += 6) {
                            uint8_t segEnd = (sx + 3 < drawEndX) ? (sx + 3) : drawEndX;
                            OLED_DrawLine(sx, floodWaveY + dy, segEnd, floodWaveY + dy, OLED_COLOR_NORMAL);
                        }
                    }
                    // lineOffset == 3 ʱ�����ƣ��γɿ�϶
                }
                
                // ����ǰ�����ߣ�ǿ����ͷ��
                if (drawStartX > 2) {
                    OLED_DrawLine(drawStartX, floodWaveY + 2, drawStartX, floodWaveY + FloodWaveHeight - 3, OLED_COLOR_NORMAL);
                    OLED_DrawLine(drawStartX + 1, floodWaveY + 1, drawStartX + 1, floodWaveY + FloodWaveHeight - 2, OLED_COLOR_NORMAL);
                }
            }
        }
    }

    // ��������
    if (m_pdata->deathData.isDead) {
        uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
        uint8_t radius  = (shengyuEnemyDeadTime - m_pdata->deathData.deathTimer) * 12 / shengyuEnemyDeadTime;
        radius          = etl::max(radius, uint8_t(1));
        OLED_DrawCircle(centerX, centerY, radius, OLED_COLOR_NORMAL);
    }
}

void ShengyuEnemy::die() {
    if (m_pdata->deathData.deathTimer > 0) {
        m_pdata->deathData.deathTimer -= controlDelayTime;
        m_pdata->deathData.deathTimer = etl::max(m_pdata->deathData.deathTimer, uint16_t(0));
        return;
    }
    m_pdata->isActive = false;
}

/**
 * @brief MODE_1: ˮ������
 * @note  ��Ӧ"��ˮ"���ʣ��ڳ��������ɶ������������ڵ���������
 *        ���������ڼ��ܳ����ڼ䱣����ʾ
 */
void ShengyuEnemy::mistCloud() {
    // ���ܿ�ʼ50ms����������λ��
    if (!mistGenerated && action_count >= 50) {
        mistGenerated = true;
        
        // ����Ļ���벿����������������������Ҫ�ڵ�������Ұ������
        for (uint8_t i = 0; i < MistCloudCount; i++) {
            // ����λ�ã�X��10-70֮�䣨���һ���򣩣�Y��5-50֮��
            mistPosX[i] = 10 + (rand() % 60);
            mistPosY[i] = 5 + (rand() % 45);
        }
    }
    // ������drawRole�л��ƣ����������ܽ���
}

/**
 * @brief MODE_2: �鲨��ӿ
 * @note  ��ǰ���������Ƴ�һ�������������ڵ�����
 *        �ȿ�����ǰ���죬Ȼ����β��������ɢ
 */
void ShengyuEnemy::floodWave() {
    // ���ܿ�ʼ50ms�������鲨
    if (!floodWaveLaunched && action_count >= 50) {
        floodWaveLaunched = true;
        
        // �鲨�յ��̶���ʤ������
        floodWaveEndX = m_pdata->spatialData.currentPosX;
        // �鲨Yλ����ʤ�����Ķ���
        floodWaveY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2 - FloodWaveHeight / 2;
        
        // ȷ��Yλ������Ļ��Χ��
        if (floodWaveY < 2) floodWaveY = 2;
        if (floodWaveY + FloodWaveHeight > 62) floodWaveY = 62 - FloodWaveHeight;
        floodWaveCurrentLen = 0;
    }
    
    if (floodWaveLaunched) {
        // ǰ���Σ�������ǰ���죨50-1000ms��
        if (action_count < 1000) {
            // ÿ40ms����12���س��ȣ���ƽ��������
            if (action_count > 50) {
                floodWaveCurrentLen = ((action_count - 50) / 40) * 12;
            }
            if (floodWaveCurrentLen > FloodWaveLength) floodWaveCurrentLen = FloodWaveLength;
        }
        // �м��Σ��������󳤶ȣ�1000-1800ms��
        else if (action_count < 1800) {
            floodWaveCurrentLen = FloodWaveLength;
        }
        // �����Σ���β��������ɢ��1800-2800ms��
        else {
            uint16_t fadeTime = action_count - 1800;
            uint8_t fadeAmount = (fadeTime / 80) * 11; // ÿ80ms��ɢ11����
            if (fadeAmount >= FloodWaveLength) {
                floodWaveCurrentLen = 0;
            } else {
                floodWaveCurrentLen = FloodWaveLength - fadeAmount;
            }
        }
    }
}

/**
 * @brief MODE_3: ��������
 * @note  ��Ӧ"��"ɫ��"����¼"������������������������һ����ͨ�ӵ�
 *        ģ����ɫ��ëɢ����Ч��
 */
void ShengyuEnemy::redThunder() {
    // ÿ ThunderInterval ms ����һ���ӵ�
    if (action_count >= ThunderInterval * (thunderFiredCount + 1) && thunderFiredCount < ThunderBulletCount) {
        thunderFiredCount++;
        
        uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
        
        // ÿ���ӵ�Yλ����Сƫ�ƣ��γ�ɢ��Ч��
        int8_t yOffset = (thunderFiredCount % 2 == 0) ? (thunderFiredCount - 3) : (3 - thunderFiredCount);
        
        m_pdata->attackData.shootCooldownTimer = 0; // ������ȴ
        shoot(centerX, centerY + yOffset, BulletType::BASIC);
    }
}

/*******************************************************************/
/**
 * @brief LiliEnemy class -  ��Ӣ����
 * ��ӿͻ�̣�׷������Yλ�� + ����ƫ�ƣ���Taowu��������Ļ��ͬ
ⲷ��𲨣�5���̶����Σ���BoEnemy��3�����κ�HundunEnemy����������ͬ
���س��棺���� + �����켣�����ϻ��ƣ����صĹ��ؼ汸����
 */
/*******************************************************************/

LiliEnemy::LiliEnemy(
    uint8_t startX, uint8_t startY, uint8_t initPosX, uint8_t initPosY, uint8_t level, uint16_t dropExp
)
: IRole() {
    //ͼƬ��Ϣ
    m_pdata->img = &LiliImg;

    //������Ϣ
    m_pdata->identity          = RoleIdentity::ENEMY;
    m_pdata->isActive          = true;
    m_pdata->initData.isInited = false;

    //�ȼ���Ϣ
    m_pdata->level = level;

    //Ѫ����Ϣ: 60 + level * 60���е�Ѫ�����Ȳ�������
    m_pdata->healthData.currentHealth = 60 + level * 160;
    m_pdata->healthData.maxHealth     = 60 + level * 160;

    //��Ѫ��Ϣ
    m_pdata->healthData.healValue       = 1;
    m_pdata->healthData.healTimeCounter = 0;
    m_pdata->healthData.healResetTime   = 12000;
    m_pdata->healthData.healSpeed       = 2;

    //�ռ��ƶ���Ϣ
    m_pdata->spatialData.canCrossBorder            = false;
    m_pdata->spatialData.currentPosX               = startX;
    m_pdata->spatialData.currentPosY               = startY;
    m_pdata->spatialData.refPosX                   = startX;
    m_pdata->spatialData.refPosY                   = startY;
    m_pdata->spatialData.sizeX                     = m_pdata->img->w;
    m_pdata->spatialData.sizeY                     = m_pdata->img->h;
    m_pdata->spatialData.moveSpeed                 = 2; // �����ƶ�
    m_pdata->spatialData.consecutiveCollisionCount = 0;

    //��ʼ��λ��
    m_pdata->initData.posX = initPosX;
    m_pdata->initData.posY = initPosY;

    //������Ϣ: 5 + level * 2���еȹ�������
    m_pdata->attackData.attackPower            = 5 + level * 2;
    m_pdata->attackData.shootCooldownSpeed     = 4;
    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 6000; // 6000/4=1500ms��ȴ
    m_pdata->attackData.bulletSpeed            = 1;

    m_pdata->attackData.bulletRange            = 6; // ���򵯷�Χ
    m_pdata->attackData.bulletDamageMultiplier = 1.2f;

    //��ײ�˺�: 8 + level * 2���е���ײ�˺���
    m_pdata->attackData.collisionPower = 8 + level * 2;

    //������Ϣ
    m_pdata->heatData.maxHeat          = 100;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 12;
    m_pdata->heatData.heatCoolDownRate = 12;

    //����״̬��Ϣ
    m_pdata->deathData.deathTimer           = liliEnemyDeadTime;
    m_pdata->deathData.isDead               = false;
    m_pdata->deathData.dropExperiencePoints = dropExp;

    // ��ʼ������ģʽ״̬����
    barkFired       = false;
    earthSurgeCount = 0;
    trapPlaced      = false;
    trapExploded    = false;
    for (uint8_t i = 0; i < TrapCount; i++) {
        trapPosX[i] = 0;
        trapPosY[i] = 0;
    }
}

void LiliEnemy::init() {
    m_pdata->initData.init_count += controlDelayTime;

    if (m_pdata->spatialData.currentPosX > m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 25) {
            m_pdata->spatialData.currentPosX -= 1;
            m_pdata->initData.init_count = 0;
        }
    } else if (m_pdata->spatialData.currentPosX < m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 25) {
            m_pdata->spatialData.currentPosX += 1;
            m_pdata->initData.init_count = 0;
        }
    } else {
        m_pdata->initData.isInited       = true;
        m_pdata->spatialData.refPosX     = m_pdata->spatialData.currentPosX;
        m_pdata->spatialData.refPosY     = m_pdata->spatialData.currentPosY;
        m_pdata->actionData.currentState = ActionState::IDLE;
        m_pdata->initData.init_count     = 0;
    }
}

void LiliEnemy::think() {
    think_count += controlDelayTime;
    if (think_count < 300) return;  // ����˼��������300ms��ԭ180ms��

    think_count = 0;

    if (m_pdata->actionData.currentState == ActionState::IDLE) {
        uint8_t randomAction = rand() % 10;

        if (randomAction < 6 ) {  // 60%�����ƶ���ԭ30%��
            uint8_t moveDir                  = rand() % 4;
            m_pdata->actionData.currentState = ActionState::MOVING;
            switch (moveDir) {
            case 0:
                m_pdata->actionData.moveMode = MoveMode::LEFT;
                break;
            case 1:
                m_pdata->actionData.moveMode = MoveMode::RIGHT;
                break;
            case 2:
                m_pdata->actionData.moveMode = MoveMode::UP;
                break;
            case 3:
                m_pdata->actionData.moveMode = MoveMode::DOWN;
                break;
            }
        } else if (randomAction < 7) {  // 10%���ʴ�����ԭ10%��
            m_pdata->actionData.currentState = ActionState::IDLE;
        } else {  // 40%���ʹ�����ԭ60%��
            m_pdata->actionData.currentState = ActionState::ATTACKING;

            uint8_t attackChoice = rand() % 6;
            switch (attackChoice) {
            case 0:
            case 1:
                action_timer                   = EarthSurgeTime;
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                earthSurgeCount                = 0;
                m_pdata->actionData.attackMode = AttackMode::MODE_1;
                break;
            case 2:
            case 3:
                action_timer                   = BarkWaveTime;
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                barkFired                      = false;
                m_pdata->actionData.attackMode = AttackMode::MODE_2;
                break;
            case 4:
            case 5:
                action_timer                   = BurrowTrapTime;
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                trapPlaced                     = false;
                trapExploded                   = false;
                m_pdata->actionData.attackMode = AttackMode::MODE_3;
                break;
            }
        }
    }
}

void LiliEnemy::doAction() {
    if (!m_pdata->initData.isInited || m_pdata->deathData.isDead) return;

    switch (m_pdata->actionData.currentState) {
    case ActionState::IDLE:
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

    case ActionState::ATTACKING:
        action_count += controlDelayTime;

        if (action_timer >= controlDelayTime)
            action_timer -= controlDelayTime;
        else
            action_timer = 0;

        switch (m_pdata->actionData.attackMode) {
        case AttackMode::MODE_1:
            earthSurge();
            break;
        case AttackMode::MODE_2:
            barkWave();
            break;
        case AttackMode::MODE_3:
            burrowTrap();
            break;
        default:
            break;
        }

        // ��������
        if (action_timer == 0) {
            m_pdata->actionData.currentState       = ActionState::IDLE;
            m_pdata->actionData.attackMode         = AttackMode::NONE;
            action_count                           = 0;
            m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;

            // ����״̬����
            barkFired       = false;
            earthSurgeCount = 0;
            trapPlaced      = false;
            trapExploded    = false;
        }
        break;
    default:
        break;
    }
}

void LiliEnemy::drawRole() {
    if (m_pdata->img != nullptr && m_pdata->isActive && !m_pdata->deathData.isDead) {
        OLED_DrawImage(
            m_pdata->spatialData.currentPosX, m_pdata->spatialData.currentPosY, m_pdata->img, OLED_COLOR_NORMAL
        );

        // MODE_3�������ǻ��� - �ڱ�ըǰ��ʾ��������
        if (m_pdata->actionData.attackMode == AttackMode::MODE_3 && trapPlaced && !trapExploded) {
            for (uint8_t i = 0; i < TrapCount; i++) {
                // �������徯�����ǣ�X��ͼ��
                OLED_DrawLine(trapPosX[i] - 3, trapPosY[i] - 3, trapPosX[i] + 3, trapPosY[i] + 3, OLED_COLOR_NORMAL);
                OLED_DrawLine(trapPosX[i] - 3, trapPosY[i] + 3, trapPosX[i] + 3, trapPosY[i] - 3, OLED_COLOR_NORMAL);
            }
        }
    }

    if (m_pdata->deathData.isDead) {
        uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
        uint8_t radius  = (liliEnemyDeadTime - m_pdata->deathData.deathTimer) * 12 / liliEnemyDeadTime;
        radius          = etl::max(radius, uint8_t(1));
        OLED_DrawCircle(centerX, centerY, radius, OLED_COLOR_NORMAL);
    }
}

void LiliEnemy::die() {
    if (m_pdata->deathData.deathTimer > 0) {
        m_pdata->deathData.deathTimer -= controlDelayTime;
        m_pdata->deathData.deathTimer = etl::max(m_pdata->deathData.deathTimer, uint16_t(0));
        return;
    }
    m_pdata->isActive = false;
}

void LiliEnemy::shoot(uint8_t x, uint8_t y, BulletType type) {
    switch (type) {
    case BulletType::BASIC:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot > m_pdata->heatData.maxHeat) return;
            if (m_pdata->attackData.shootCooldownTimer > 0) return;

            IBullet *newBullet = createBullet(x, y, BulletType::BASIC);
            if (newBullet != nullptr) {
                taskENTER_CRITICAL();
                g_entityManager.addBullet(newBullet);
                taskEXIT_CRITICAL();
                m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot;
                m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
            }
        }
        break;
    case BulletType::FIRE_BALL:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 2 > m_pdata->heatData.maxHeat) return;
            if (m_pdata->attackData.shootCooldownTimer > 0) return;

            IBullet *newBullet = createBullet(x, y, BulletType::FIRE_BALL);
            if (newBullet != nullptr) {
                taskENTER_CRITICAL();
                g_entityManager.addBullet(newBullet);
                taskEXIT_CRITICAL();
                m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot * 2;
                m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
            }
        }
        break;
    case BulletType::LIGHTNING_LINE:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 1.5 > m_pdata->heatData.maxHeat) return;
            if (m_pdata->attackData.shootCooldownTimer > 0) return;

            IBullet *newBullet = createBullet(x, y, BulletType::LIGHTNING_LINE);
            if (newBullet != nullptr) {
                taskENTER_CRITICAL();
                g_entityManager.addBullet(newBullet);
                taskEXIT_CRITICAL();
                m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot * 1.5;
                m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
            }
        }
        break;
    }
}

/**
 * @brief MODE_1: ��ӿͻ��
 * @note  ��Ӧ"����"���ʣ���ǰ�������������򵯣����鱬ը��
 *        ���򵯻����ɷ�Χ�˺�
 */
void LiliEnemy::earthSurge() {
    // ÿ EarthSurgeInterval ms ����һ������
    if (action_count >= EarthSurgeInterval * (earthSurgeCount + 1)) {
        earthSurgeCount++;

        uint8_t shootX  = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

        // ��ȡ����λ�ã������ҷ�������
        IRole *player        = g_entityManager.getPlayerRole();
        int8_t targetYOffset = 0;
        if (player != nullptr) {
            int16_t playerY = player->getData()->spatialData.currentPosY + player->getData()->spatialData.sizeY / 2;
            int16_t deltaY  = playerY - centerY;
            // ��������ƫ�ƣ�ģ���������䲻������
            targetYOffset = (deltaY > 5) ? 4 : ((deltaY < -5) ? -4 : 0);
            targetYOffset += (rand() % 5) - 2; // -2 �� +2 ����ƫ��
        }

        // �������򵯣����飩
        shoot(shootX, centerY + targetYOffset, BulletType::FIRE_BALL);
    }
}

/**
 * @brief MODE_2: ⲷ�����
 * @note  ��Ӧ"�����繷��"���ʣ�������������
 *        һ���Է���5��������ͨ�ӵ������ǽϴ���Χ
 */
void LiliEnemy::barkWave() {
    // ֻ�ڼ��ܿ�ʼ50ms������һ��
    if (barkFired || action_count < 50) return;

    barkFired       = true;
    uint8_t shootX  = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

    // ����5��������ͨ�ӵ���������ɢЧ����
    for (int8_t i = -2; i <= 2; i++) {
        m_pdata->attackData.shootCooldownTimer = 0; // ������ȴ����������
        shoot(shootX, centerY + i * 2, BulletType::BASIC);
    }
}

/**
 * @brief MODE_3: Ѩ������
 * @note  ��Ӧ"�������ض�����"���ʣ����������ھ�����
 *        ������λ�÷���2���������ǣ��ӳٺ���ը��������
 *        ������Ҫ��������λ��
 */
void LiliEnemy::burrowTrap() {
    // �׶�1���������壨���ܿ�ʼʱ��
    if (!trapPlaced && action_count >= 50) {
        trapPlaced = true;
        
        // �����Ҹ�������λ�÷�������
        IRole *player = g_entityManager.getPlayerRole();
        if (player != nullptr) {
            uint8_t playerX = player->getData()->spatialData.currentPosX + player->getData()->spatialData.sizeX / 2;
            uint8_t playerY = player->getData()->spatialData.currentPosY + player->getData()->spatialData.sizeY / 2;
            
            for (uint8_t i = 0; i < TrapCount; i++) {
                // ����λ�ã�����ǰ������λ�ã�X: +30~+50, Y: ��15����ƫ�ƣ�
                int16_t randomOffsetX = 30 + (rand() % 20);
                int16_t randomOffsetY = (rand() % 31) - 15; // -15 �� +15
                
                trapPosX[i] = etl::clamp<int16_t>(playerX + randomOffsetX, 10, 120);
                trapPosY[i] = etl::clamp<int16_t>(playerY + randomOffsetY, 5, 58);
            }
        } else {
            // ������ʱ����Ļ�м�������������
            for (uint8_t i = 0; i < TrapCount; i++) {
                trapPosX[i] = 30 + (rand() % 60);
                trapPosY[i] = 10 + (rand() % 44);
            }
        }
    }
    
    // �׶�2�����屬ը���ӳٺ���
    if (trapPlaced && !trapExploded && action_count >= TrapExplodeDelay) {
        trapExploded = true;
        
        // ��ÿ������λ�÷���������
        for (uint8_t i = 0; i < TrapCount; i++) {
            m_pdata->attackData.shootCooldownTimer = 0; // ������ȴ
            shoot(trapPosX[i], trapPosY[i], BulletType::FIRE_BALL);
        }
    }
}

/*******************************************************************/

/*******************************************************************/
/**
 * @brief TaotieEnemy class
 * @note  ���ģ����� �� Ӣ�ģ�Taotie,�񻰵��ʣ�����֮һ���������桢����Ҹ�¡�������צ��������Ӥ����
 * @note  �Ϲ� �����ס� ֮һ��̰���޶ȣ�����ʳ�������רʳ��������������������̰����
 * @note  BOSS�����͵��ˣ����;޴���64x64 ���أ�����Ѫ�����߹������������ƶ���������ʽ�����Ҿ�����в�ԣ��ó���ս��
 * @note  ������ʽ1�����������Լ��������������ɹ��� 
 * @note  ������ʽ2������������ͨ�ӵ�
 * @note  ������ʽ3����ǰ��ײ������ײ������
 * @note  ������ʽ4, ������ѹ���������������֣�������ѹ����
 * @note  ������ʽ5, ���������Լ�������ͬʱ��ǰ��ײ
 */

TaotieEnemy::TaotieEnemy(
    uint8_t startX, uint8_t startY, uint8_t initPosX, uint8_t initPosY, uint8_t level, uint16_t dropExp
)
: IRole() {
    //ͼƬ��Ϣ
    m_pdata->img = &TaotieImg;

    //������Ϣ
    m_pdata->identity          = RoleIdentity::ENEMY;
    m_pdata->isActive          = true;
    m_pdata->initData.isInited = false;

    //�ȼ���Ϣ
    m_pdata->level = level;

    //Ѫ����Ϣ
    m_pdata->healthData.currentHealth = 350 + level * 1100;
    m_pdata->healthData.maxHealth     = 350 + level * 1100;

    //��Ѫ��Ϣ
    m_pdata->healthData.healValue       = 20;
    m_pdata->healthData.healTimeCounter = 0;
    m_pdata->healthData.healResetTime   = 15000;
    m_pdata->healthData.healSpeed       = 5;

    //�ռ��ƶ���Ϣ
    m_pdata->spatialData.canCrossBorder            = true;
    m_pdata->spatialData.currentPosX               = startX; // Starting X position
    m_pdata->spatialData.currentPosY               = startY; // Starting Y position
    m_pdata->spatialData.refPosX                   = startX;
    m_pdata->spatialData.refPosY                   = startY;
    m_pdata->spatialData.sizeX                     = m_pdata->img->w;
    m_pdata->spatialData.sizeY                     = m_pdata->img->h;
    m_pdata->spatialData.moveSpeed                 = 1; // Set movement speed
    m_pdata->spatialData.consecutiveCollisionCount = 0;

    //��ʼ��λ��
    m_pdata->initData.posX = initPosX;
    m_pdata->initData.posY = initPosY;

    //������Ϣ
    m_pdata->attackData.attackPower            = 2 + level * 4;
    m_pdata->attackData.shootCooldownSpeed     = 5;
    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 5000; //5000 ms
    m_pdata->attackData.bulletSpeed            = 1;

    m_pdata->attackData.bulletRange            = 10;   //ֻ�Ի�������Ч
    m_pdata->attackData.bulletDamageMultiplier = 1.5f; //ֻ������������Ч

    m_pdata->attackData.collisionPower = 10 + level * 10;

    //������Ϣ
    m_pdata->heatData.maxHeat          = 200;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 20;
    m_pdata->heatData.heatCoolDownRate = 10; //ÿ����ȴ10��������ÿ����ȴʱ��������200ms

    //����״̬��Ϣ
    m_pdata->deathData.deathTimer           = TaotieEnemyDeadTime;
    m_pdata->deathData.isDead               = false;
    m_pdata->deathData.dropExperiencePoints = dropExp;

    // Initialize other enemy-specific data here
}

void TaotieEnemy::shoot(uint8_t x, uint8_t y, BulletType type) {
    // Implement enemy shooting logic
    // Create bullet based on type
    switch (type) {
    case BulletType::BASIC:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot > m_pdata->heatData.maxHeat)
                return;                                             // ���������������޷�����
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // ��ȴ�У��޷�����

            IBullet *newBullet = createBullet(x, y, BulletType::BASIC);
            if (newBullet != nullptr) {
                if (g_entityManager.addBullet(newBullet)) {
                    // Successfully added bullet
                    m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
                    m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot;
                } else {
                    delete newBullet; // Clean up if not added
                }
            }
        }
        break;
    case BulletType::FIRE_BALL:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 2 > m_pdata->heatData.maxHeat)
                return;                                             // ���������������޷�����
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // ��ȴ�У��޷�����

            IBullet *newBullet = createBullet(x, y, BulletType::FIRE_BALL);
            if (newBullet != nullptr) {
                if (g_entityManager.addBullet(newBullet)) {
                    // Successfully added bullet
                    m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
                    m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot;
                } else {
                    delete newBullet; // Clean up if not added
                }
            }
        }
        break;
    case BulletType::LIGHTNING_LINE:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 1.5 > m_pdata->heatData.maxHeat)
                return;                                             // ���������������޷�����
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // ��ȴ�У��޷�����

            IBullet *newBullet = createBullet(x, y, BulletType::LIGHTNING_LINE);
            if (newBullet != nullptr) {
                if (g_entityManager.addBullet(newBullet)) {
                    // Successfully added bullet
                    m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
                    m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot;
                } else {
                    delete newBullet; // Clean up if not added
                }
            }
        }
    }
}

void TaotieEnemy::init() {
    m_pdata->initData.init_count += controlDelayTime;
    // Initialize enemy role specifics

    if (m_pdata->spatialData.currentPosX > m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 60) { // ÿ60ms�ƶ�һ��
            m_pdata->spatialData.currentPosX -= 1;
            m_pdata->initData.init_count = 0;
        }
    } else if (m_pdata->spatialData.currentPosX < m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 60) { // ÿ60ms�ƶ�һ��
            m_pdata->spatialData.currentPosX += 1;
            m_pdata->initData.init_count = 0;
        }
    } else {
        m_pdata->initData.isInited   = true;
        m_pdata->spatialData.refPosX = m_pdata->spatialData.currentPosX;
        m_pdata->spatialData.refPosY = m_pdata->spatialData.currentPosY;
        m_pdata->initData.init_count = 0;
    }
}

void TaotieEnemy::think() {
    // Implement enemy AI logic
    think_count += controlDelayTime;
    if (think_count < 100) // ÿ100ms����һ���ж�
        return;

    think_count = 0;

    uint8_t randomAction = rand() % 6;
    // Random action: 0 - move left, 1 - move right, 2 - move down, 3 - move up, 4 - stay still, 5 - shoot
    if (m_pdata->actionData.currentState == ActionState::IDLE) {
        //�ƶ�
        if (randomAction == 0) {
            m_pdata->actionData.moveMode     = MoveMode::LEFT;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 1) {
            m_pdata->actionData.moveMode     = MoveMode::RIGHT;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 2) {
            m_pdata->actionData.moveMode     = MoveMode::DOWN;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 3) {
            m_pdata->actionData.moveMode     = MoveMode::UP;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 4) {
            // Stay still
            m_pdata->actionData.moveMode     = MoveMode::NONE;
            m_pdata->actionData.currentState = ActionState::MOVING;
        }

        //����
        else if (randomAction == 5) {
            if (m_pdata->attackData.shootCooldownTimer > 0) {
                // ��������ȴ�У��򲻽��й��������ֿ���״̬
                m_pdata->actionData.moveMode     = MoveMode::NONE;
                m_pdata->actionData.currentState = ActionState::IDLE;
                return;
            }

            uint8_t randomAttackMode         = rand() % 5 + 1; // 1-5 ������ʽ
            m_pdata->actionData.currentState = ActionState::ATTACKING;

            switch (randomAttackMode) {
            case 1:
                //����ģʽ1 - ���ɹ���
                action_timer                   = 1500; // ���ѹ�����������ʱ��1500ms
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                m_pdata->actionData.attackMode = AttackMode::MODE_1;
                break;
            case 2:
                //����ģʽ2 - �����ӵ�����
                action_timer                   = 1000; // ���ѹ�����������ʱ��1000ms
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                m_pdata->actionData.attackMode = AttackMode::MODE_2;
                break;

            case 3:
                //����ģʽ3 - ����ײ������
                //500ms����+1000msͣ��+500ms����
                action_timer                   = 2000; // ���ѹ�����������ʱ��2000ms
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                m_pdata->actionData.attackMode = AttackMode::MODE_3;
                break;
            case 4:
                //����ģʽ4 - ������ѹ����
                //crush from left side
                action_timer                   = 4000; // ���ѹ�����������ʱ��4000ms
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                appearedForCrush               = false;
                comeBackForCrush               = false;
                m_pdata->actionData.attackMode = AttackMode::MODE_4;
                break;
            case 5:
                //����ģʽ5 - ���������湥��
                action_timer                   = 3000; // ���ѹ�����������ʱ��3000ms
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                m_pdata->actionData.attackMode = AttackMode::MODE_5;
                break;
            default:
                //Ĭ�Ϲ���ģʽ1 - ���ɹ���
                action_timer                   = 1500; // ���ѹ�����������ʱ��1500ms
                action_MaxTime                 = action_timer;
                action_count                   = 0;
                m_pdata->actionData.attackMode = AttackMode::MODE_1;
            }
        }
    }
}

void TaotieEnemy::doAction() {
    if (m_pdata->initData.isInited == false) {
        return;
    }

    // Implement enemy action logic
    if (m_pdata->deathData.isDead) {
        return;
    }

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
        // Move logic handled in think()
        break;
    case ActionState::ATTACKING:
        //��������ʱ�䣬�������ڶ���������Ƶ��
        action_count += controlDelayTime;

        //��������ʱ
        if (action_timer >= controlDelayTime)
            action_timer -= controlDelayTime;
        else
            action_timer = 0;

        switch (m_pdata->actionData.attackMode) {
        //ִ�й�������
        case AttackMode::MODE_1:
            pullPlayerAndDevourAttack();
            break;
        case AttackMode::MODE_2:
            fireThreeRowsBasicBullets();
            break;

        case AttackMode::MODE_3:
            chargeForwardAndRamAttack();
            break;
        case AttackMode::MODE_4:
            appearLeftAndRollBackCrushAttack();
            break;
        case AttackMode::MODE_5:
            pullPlayerAndChargeForwardAttack();
            break;
        default:
            break;
        }

        if (action_timer == 0) {
            m_pdata->actionData.currentState       = ActionState::IDLE;
            m_pdata->actionData.attackMode         = AttackMode::NONE;
            action_count                           = 0;
            m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime; // ������������ȴʱ��
        }
        break;
    }
}

void TaotieEnemy::drawRole() {
    if (m_pdata->img != nullptr && m_pdata->isActive && !m_pdata->deathData.isDead) {
        OLED_DrawImage(
            m_pdata->spatialData.currentPosX, m_pdata->spatialData.currentPosY, m_pdata->img, OLED_COLOR_NORMAL
        );
    }

    if (m_pdata->deathData.isDead) {
        // Draw death animation or effect
        // ������������������һ���򵥵�ԲȦ��ʾ��ʧЧ����
        uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
        uint8_t radius =
            (TaotieEnemyDeadTime - m_pdata->deathData.deathTimer) * 30 / TaotieEnemyDeadTime; // ��0����������ֵ5
        radius = etl::max(radius, uint8_t(1));                                                // ��С�뾶����

        OLED_DrawCircle(centerX, centerY, radius, OLED_COLOR_NORMAL);
    }
}

void TaotieEnemy::die() {
    // Implement enemy death logic
    if (m_pdata->deathData.deathTimer > 0) {
        m_pdata->deathData.deathTimer -= controlDelayTime;
        m_pdata->deathData.deathTimer = etl::max(m_pdata->deathData.deathTimer, uint16_t(0));
        return;
    }

    m_pdata->isActive = false;
}

//��������
void TaotieEnemy::pullPlayerAndDevourAttack() {
    //���������Լ��������������ɹ���
    if (action_count < action_MaxTime / pullDistance) // ÿ50ms����һ��
        return;
    action_count = 0;

    IRole *player = g_entityManager.getPlayerRole();
    if (player == nullptr) return;

    uint16_t dirX = 0;
    uint16_t dirY = 0;
    //���㷽������

    int16_t deltaX = (m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2)
                     - (player->getData()->spatialData.currentPosX + player->getData()->spatialData.sizeX / 2);
    int16_t deltaY = (m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2)
                     - (player->getData()->spatialData.currentPosY + player->getData()->spatialData.sizeY / 2);

    if (deltaX < 0) dirX = -1;
    if (deltaX > 0) dirX = 1;
    if (deltaX == 0) dirX = 1;
    if (deltaY < 0) dirY = -1;
    if (deltaY > 0) dirY = 1;
    if (deltaY == 0) dirY = 0;

    //��������λ��
    player->move(dirX, dirY, true);
}

void TaotieEnemy::fireThreeRowsBasicBullets() {
    if (action_count < 500) // ÿ500ms����һ��
        return;
    action_count = 0;

    //����������ͨ�ӵ�
    uint8_t m_x = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t m_y = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

    uint8_t m_x_1 = m_x;
    uint8_t m_y_1 = m_y - 6;
    uint8_t m_x_2 = m_x;
    uint8_t m_y_2 = m_y;
    uint8_t m_x_3 = m_x;
    uint8_t m_y_3 = m_y + 6;

    m_pdata->attackData.shootCooldownTimer = 0; //������ȴʱ�䣬���ٷ�����һ���ӵ�
    m_pdata->heatData.currentHeat          = 0; //����������Ϣ
    shoot(m_x, m_y - 6, BulletType::BASIC);
    m_pdata->attackData.shootCooldownTimer = 0; //������ȴʱ�䣬���ٷ����ڶ����ӵ�
    m_pdata->heatData.currentHeat          = 0; //����������Ϣ
    shoot(m_x_2, m_y_2, BulletType::BASIC);
    m_pdata->attackData.shootCooldownTimer = 0; //������ȴʱ�䣬���ٷ����������ӵ�
    m_pdata->heatData.currentHeat          = 0; //����������Ϣ
    shoot(m_x_3, m_y_3, BulletType::BASIC);

    m_x_1 = m_x + 10;
    m_x_2 = m_x + 10;
    m_x_3 = m_x + 10;

    m_pdata->attackData.shootCooldownTimer = 0; //������ȴʱ�䣬���ٷ�����һ���ӵ�
    m_pdata->heatData.currentHeat          = 0; //����������Ϣ
    shoot(m_x_1, m_y_1, BulletType::BASIC);
    m_pdata->attackData.shootCooldownTimer = 0; //������ȴʱ�䣬���ٷ����ڶ����ӵ�
    m_pdata->heatData.currentHeat          = 0; //����������Ϣ
    shoot(m_x_2, m_y_2, BulletType::BASIC);
    m_pdata->attackData.shootCooldownTimer = 0; //������ȴʱ�䣬���ٷ����������ӵ�
    m_pdata->heatData.currentHeat          = 0; //����������Ϣ
    shoot(m_x_3, m_y_3, BulletType::BASIC);
}

void TaotieEnemy::chargeForwardAndRamAttack() {
    //��ǰ��ײ������ײ������
    //����������ײ
    if (action_count < action_MaxTime / chargeDistance / 4) // 2000/30/4 ms�ƶ�һ��
        return;
    action_count = 0;
    int8_t dir   = -1;                                                                        //������ײ
    if (action_timer < action_MaxTime * 3 / 4 && action_timer >= action_MaxTime / 4) dir = 0; //ͣ��
    if (action_timer < action_MaxTime / 4) dir = 1;                                           //���һ���
    move(dir, 0);
}

void TaotieEnemy::appearLeftAndRollBackCrushAttack() {
    // //������ѹ���������������֣�������ѹ����

    //��ΪҪ�����أ����Ծ��봥��ʱ������2
    if (action_count < action_MaxTime / crushChargeDistance / 2) // 4000/100/2=20 ms�ƶ�һ��
        return;
    action_count = 0;

    RoleData *taoTie = this->getData();
    if (taoTie == nullptr) return;

    if (action_timer >= action_MaxTime / 2) {
        //�����ƶ�
        if (taoTie->spatialData.currentPosX < 120 && appearedForCrush == false) move(1, 0);

        //����������
        if (taoTie->spatialData.currentPosX >= 120 && appearedForCrush == false) {
            appearedForCrush                = true;
            taoTie->spatialData.refPosX     = -70; //����λ��
            taoTie->spatialData.refPosY     = 1;
            taoTie->spatialData.currentPosX = -70;
            taoTie->spatialData.currentPosY = 1;
        }
        //������ѹ
        if (appearedForCrush == true) {
            move(1, 0);
        }
    } else {
        //��������
        if (!comeBackForCrush) move(-1, 0);
        if (taoTie->spatialData.currentPosX <= -64) {
            //�ص���ʼλ�ã���������
            comeBackForCrush                = true;
            taoTie->spatialData.refPosX     = 120; //����λ��
            taoTie->spatialData.refPosY     = 1;
            taoTie->spatialData.currentPosX = 120;
            taoTie->spatialData.currentPosY = 1;
        }
        if (taoTie->spatialData.currentPosX > 64) {
            move(-1, 0);
        }
    }

    if (action_timer <= 50) {
        taoTie->spatialData.refPosX     = 64; //����λ��
        taoTie->spatialData.refPosY     = 1;
        taoTie->spatialData.currentPosX = 64;
        taoTie->spatialData.currentPosY = 1;
    }
}

void TaotieEnemy::pullPlayerAndChargeForwardAttack() {
    // //���������Լ�������ͬʱ��ǰ��ײ

    //���������Լ��������������ɹ���
    if (action_count < action_MaxTime / pullAndChargeDistance) // ÿ3000/120 ms ����һ��
        return;
    action_count = 0;

    IRole *player = g_entityManager.getPlayerRole();
    if (player == nullptr) return;

    uint16_t dirX = 0;
    uint16_t dirY = 0;
    //���㷽������

    int16_t deltaX = (m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2)
                     - (player->getData()->spatialData.currentPosX + player->getData()->spatialData.sizeX / 2);
    int16_t deltaY = (m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2)
                     - (player->getData()->spatialData.currentPosY + player->getData()->spatialData.sizeY / 2);

    if (deltaX < 0) dirX = -1;
    if (deltaX > 0) dirX = 1;
    if (deltaX == 0) dirX = 1;
    if (deltaY < 0) dirY = -1;
    if (deltaY > 0) dirY = 1;
    if (deltaY == 0) dirY = 0;

    //��������λ��
    player->move(dirX, dirY, true);

    //��ǰ��ײ������ײ������
    //������ײ
    //��ײ����=pullAndChargeDistance
    int8_t dir = -2;
    if (action_timer > action_MaxTime * 3 / 4) dir = -2;                                      //ǰ����ʱ��������ײ
    if (action_timer <= action_MaxTime * 3 / 4 && action_timer > action_MaxTime / 4) dir = 0; //������ʱ��ͣ��
    if (action_timer <= action_MaxTime / 4) dir = 2;                                          //����ʱ�����һ���
    move(dir, 0);
}

/*******************************************************************/

/*******************************************************************/
/**
 * @brief TaowuEnemy class - ���� BOSS
 * @note  ���ģ����� �� Ӣ�ģ�Taowu
 * @note  �񻰵��ʣ�����֮һ������Ȯë���������ڡ�β��һ�ɰ˳ߣ�
 * @note  �Ϲ� "����" ֮һ���Ը����Ӳ��ɽ̻����ڻ�Ұ�н������򡢲�ʳ���࣬�����ױ������档
 * 
 * @note  BOSS�����͵��ˣ����;޴���64x64 ���أ�����Ѫ�����߹������������ƶ���
 * @note  ������ʽ�����Ҿ�����в�ԣ��ó������ƶ���������Ļ��
 * 
 * @note  === ������ʽ === 
 * @note  MODE_1: �������м�λ��(63,1)������λ�÷���������ͨ�ӵ�
 *               Ѫ��Խ�ͷ�������ʱ��Խ��������ʱ�� MassiveBasicBulletFireTime=3000ms������6000ms
 *               ����Ƶ�ʣ�ÿ 1000/BulletsPerSecond=125ms һ��
 * @note  MODE_2: �������м�λ��(63,1)������λ�÷�������������
 *               ����ʱ�� FiveFireballBulletFireTime=3000ms
 *               ����Ƶ�ʣ�ÿ 3000/FireballCount=375ms һ������ FireballCount=8 ��
 * @note  MODE_3: ԭ��˲�����м䷢��һ�Ż��򵯣�����Ե����������������ͨ�ӵ�
 *               ����ʱ�� CenterFireballAttackTime=100ms
 * @note  MODE_4: ����һ���������͵��ӵ���ֻ���м���ȱ�ڣ�ȱ�ڷ�Χ 12���أ�
 *               ����ʱ�� NotchedBulletsAttackTime=500ms
 * @note  MODE_5: ������Զ��(140,1)������3�Ż��򵯣�����Yλ�ã���Ȼ�󷵻�(63,1)
 *               ����ʱ�� BlinkRandomTime=1000ms�������׶�ִ��
 * @note  MODE_6: �������֣���������Yλ�ã�Xλ������(30-80)����������������CD
 *               ����ʱ�� BlinkAlignedTime=100ms
 */

TaowuEnemy::TaowuEnemy(
    uint8_t startX, uint8_t startY, uint8_t initPosX, uint8_t initPosY, uint8_t level, uint16_t dropExp
)
: IRole() {
    //ͼƬ��Ϣ
    m_pdata->img = &TaowuImg;

    //������Ϣ
    m_pdata->identity          = RoleIdentity::ENEMY;
    m_pdata->isActive          = true;
    m_pdata->initData.isInited = false;

    //�ȼ���Ϣ
    m_pdata->level = level;

    //Ѫ����Ϣ
    //Ѫ���ϵͣ����������ߣ��ƶ��ٶȿ�
    m_pdata->healthData.currentHealth = 130 + level * 800;
    m_pdata->healthData.maxHealth     = 130 + level * 800;

    //��Ѫ��Ϣ
    m_pdata->healthData.healValue       = 30;
    m_pdata->healthData.healTimeCounter = 0;
    m_pdata->healthData.healResetTime   = 15000;
    m_pdata->healthData.healSpeed       = 5;

    //�ռ��ƶ���Ϣ
    m_pdata->spatialData.canCrossBorder            = true;
    m_pdata->spatialData.currentPosX               = startX; // Starting X position
    m_pdata->spatialData.currentPosY               = startY; // Starting Y position
    m_pdata->spatialData.refPosX                   = startX;
    m_pdata->spatialData.refPosY                   = startY;
    m_pdata->spatialData.sizeX                     = m_pdata->img->w;
    m_pdata->spatialData.sizeY                     = m_pdata->img->h;
    m_pdata->spatialData.moveSpeed                 = 3; // Set movement speed
    m_pdata->spatialData.consecutiveCollisionCount = 0;

    //��ʼ��λ��
    m_pdata->initData.posX = initPosX;
    m_pdata->initData.posY = initPosY;

    //������Ϣ
    m_pdata->attackData.attackPower            = 10 + level * 5;
    m_pdata->attackData.shootCooldownSpeed     = 5;
    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 5000; //5000 ms
    m_pdata->attackData.bulletSpeed            = 1;

    m_pdata->attackData.bulletRange            = 10;   //ֻ�Ի�������Ч
    m_pdata->attackData.bulletDamageMultiplier = 1.5f; //ֻ������������Ч

    m_pdata->attackData.collisionPower = 7 + level * 5;

    //������Ϣ
    m_pdata->heatData.maxHeat          = 250;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 0;
    m_pdata->heatData.heatCoolDownRate = 10; //ÿ����ȴ10��������ÿ����ȴʱ��������200ms

    //����״̬��Ϣ
    m_pdata->deathData.deathTimer           = TaowuEnemyDeadTime;
    m_pdata->deathData.isDead               = false;
    m_pdata->deathData.dropExperiencePoints = dropExp;

    // Initialize other enemy-specific data here
}

void TaowuEnemy::shoot(uint8_t x, uint8_t y, BulletType type) {
    // Implement enemy shooting logic
    // Create bullet based on type
    switch (type) {
    case BulletType::BASIC:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot > m_pdata->heatData.maxHeat)
                return;                                             // ���������������޷�����
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // ��ȴ�У��޷�����

            IBullet *newBullet = createBullet(x, y, BulletType::BASIC);
            if (newBullet != nullptr) {
                if (g_entityManager.addBullet(newBullet)) {
                    // Successfully added bullet
                    m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
                    m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot;
                } else {
                    delete newBullet; // Clean up if not added
                }
            }
        }
        break;
    case BulletType::FIRE_BALL:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 2 > m_pdata->heatData.maxHeat)
                return;                                             // ���������������޷�����
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // ��ȴ�У��޷�����

            IBullet *newBullet = createBullet(x, y, BulletType::FIRE_BALL);
            if (newBullet != nullptr) {
                if (g_entityManager.addBullet(newBullet)) {
                    // Successfully added bullet
                    m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
                    m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot;
                } else {
                    delete newBullet; // Clean up if not added
                }
            }
        }
        break;
    case BulletType::LIGHTNING_LINE:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 1.5 > m_pdata->heatData.maxHeat)
                return;                                             // ���������������޷�����
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // ��ȴ�У��޷�����

            IBullet *newBullet = createBullet(x, y, BulletType::LIGHTNING_LINE);
            if (newBullet != nullptr) {
                if (g_entityManager.addBullet(newBullet)) {
                    // Successfully added bullet
                    m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
                    m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot;
                } else {
                    delete newBullet; // Clean up if not added
                }
            }
        }
    }
}

void TaowuEnemy::init() {
    m_pdata->initData.init_count += controlDelayTime;
    // Initialize enemy role specifics

    if (m_pdata->spatialData.currentPosX > m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 60) { // ÿ60ms�ƶ�һ��
            m_pdata->spatialData.currentPosX -= 1;
            m_pdata->initData.init_count = 0;
        }
    } else if (m_pdata->spatialData.currentPosX < m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 60) { // ÿ60ms�ƶ�һ��
            m_pdata->spatialData.currentPosX += 1;
            m_pdata->initData.init_count = 0;
        }
    } else {
        m_pdata->initData.isInited   = true;
        m_pdata->spatialData.refPosX = m_pdata->spatialData.currentPosX;
        m_pdata->spatialData.refPosY = m_pdata->spatialData.currentPosY;
        m_pdata->initData.init_count = 0;
    }
}

void TaowuEnemy::think() {
    // Implement enemy AI logic
    think_count += controlDelayTime;
    if (think_count < 100) // ÿ100ms����һ���ж�
        return;

    think_count = 0;

    uint8_t randomAction = rand() % 6;
    // Random action: 0 - move left, 1 - move right, 2 - move down, 3 - move up, 4 - stay still, 5 - shoot
    if (m_pdata->actionData.currentState == ActionState::IDLE) {
        //�ƶ�
        if (randomAction == 0) {
            m_pdata->actionData.moveMode     = MoveMode::LEFT;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 1) {
            m_pdata->actionData.moveMode     = MoveMode::RIGHT;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 2) {
            m_pdata->actionData.moveMode     = MoveMode::DOWN;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 3) {
            m_pdata->actionData.moveMode     = MoveMode::UP;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 4) {
            // Stay still
            m_pdata->actionData.moveMode     = MoveMode::NONE;
            m_pdata->actionData.currentState = ActionState::MOVING;
        }

        //����
        else if (randomAction == 5) {
            if (m_pdata->attackData.shootCooldownTimer > 0) {
                // ��������ȴ�У��򲻽��й��������ֿ���״̬
                m_pdata->actionData.moveMode     = MoveMode::NONE;
                m_pdata->actionData.currentState = ActionState::IDLE;
                return;
            }

            uint8_t randomAttackMode         = rand() % 6 + 1; // 1-6 ������ʽ
            m_pdata->actionData.currentState = ActionState::ATTACKING;

            switch (randomAttackMode) {
            case 1:
                //����ģʽ1 - ����λ�÷���������ͨ�ӵ���Ѫ��Խ�ͣ���������ʱ��Խ��������ʱ��Ϊ3�룬����Ϊ6�룬ÿ�뷢��5��
                action_timer   = (uint16_t)(MassiveBasicBulletFireTime
                                          * float(
                                              (float)(m_pdata->healthData.maxHealth - m_pdata->healthData.currentHealth)
                                                  / (float)m_pdata->healthData.maxHealth
                                              + 1
                                          )); // Ѫ��Խ�ͣ�������������ʱ��Խ��������ʱ��Ϊ3000ms, ����Ϊ6000ms
                action_MaxTime = action_timer;
                action_count   = 0;

                positionChange = false;

                m_pdata->actionData.attackMode = AttackMode::MODE_1;
                break;
            case 2:
                //����ģʽ2 - ����λ�÷���5�����򵯣�����ʱ��3��
                action_timer   = FiveFireballBulletFireTime; // ��軹�����������ʱ��3000ms
                action_MaxTime = action_timer;
                action_count   = 0;
                positionChange = false;

                m_pdata->actionData.attackMode = AttackMode::MODE_2;
                break;

            case 3:
                //����ģʽ3 - �м䷢��һ�Ż��򵯣�����Ե���෢����������ͨ�ӵ�
                //1000ms
                action_timer   = CenterFireballAttackTime; // ��軹�����������ʱ��100ms
                action_MaxTime = action_timer;
                action_count   = 0;

                m_pdata->actionData.attackMode = AttackMode::MODE_3;
                break;
            case 4:
                //����ģʽ4 - ����һ���������͵��ӵ���ֻ���м���ȱ��
                action_timer   = NotchedBulletsAttackTime; // ��軹�����������ʱ��
                action_MaxTime = action_timer;
                action_count   = 0;

                m_pdata->actionData.attackMode = AttackMode::MODE_4;
                break;
            case 5:
                //����ģʽ5 - ���������ƶ������»��򵯲���ʧ
                action_timer   = BlinkRandomTime; // ���ֳ���ʱ��
                action_MaxTime = action_timer;
                action_count   = 0;
                positionChange = false;

                m_pdata->actionData.attackMode = AttackMode::MODE_5;
                break;
            case 6:
                //����ģʽ6 - �������֣���������λ������
                action_timer   = BlinkAlignedTime; // ��������ʱ��
                action_MaxTime = action_timer;
                action_count   = 0;
                positionChange = false;

                m_pdata->actionData.attackMode = AttackMode::MODE_6;
                break;
            default:
                break;
            }
        }
    }
}

void TaowuEnemy::doAction() {
    if (m_pdata->initData.isInited == false) {
        return;
    }

    // Implement enemy action logic
    if (m_pdata->deathData.isDead) {
        return;
    }
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
        // Move logic handled in think()
        break;
    case ActionState::ATTACKING:
        //��������ʱ�䣬�������ڶ���������Ƶ��
        action_count += controlDelayTime;

        //��������ʱ
        if (action_timer >= controlDelayTime)
            action_timer -= controlDelayTime;
        else
            action_timer = 0;

        switch (m_pdata->actionData.attackMode) {
        //ִ�й�������
        case AttackMode::MODE_1:
            fireContinuousMassiveBasicBullets();
            break;
        case AttackMode::MODE_2:
            fireFiveFireballBulletsAtRandom();
            break;
        case AttackMode::MODE_3:
            fireCenterFireballAndSideBasicBullets();
            break;
        case AttackMode::MODE_4:
            fireSingleRowNotchedBasicBullets();
            break;
        case AttackMode::MODE_5:
            blinkToRandomPosition();
            break;
        case AttackMode::MODE_6:
            blinkToPlayerAlignedPosition();
            break;
        default:
            break;
        }

        if (action_timer == 0) {
            // ����ģʽ6������CD�������� attackMode ֮ǰ���飩
            bool clearCD = (m_pdata->actionData.attackMode == AttackMode::MODE_6);

            m_pdata->actionData.currentState = ActionState::IDLE;
            m_pdata->actionData.attackMode   = AttackMode::NONE;
            action_count                     = 0;

            if (clearCD) {
                m_pdata->attackData.shootCooldownTimer = 0; // MODE_6 ������ȴʱ��
            } else {
                m_pdata->attackData.shootCooldownTimer =
                    m_pdata->attackData.shootCooldownResetTime; // ������������ȴʱ��
            }
        }
        break;
    }
}

void TaowuEnemy::drawRole() {
    if (m_pdata->img != nullptr && m_pdata->isActive && !m_pdata->deathData.isDead) {
        OLED_DrawImage(
            m_pdata->spatialData.currentPosX, m_pdata->spatialData.currentPosY, m_pdata->img, OLED_COLOR_NORMAL
        );
    }

    if (m_pdata->deathData.isDead) {
        // Draw death animation or effect
        // ������������������һ���򵥵�ԲȦ��ʾ��ʧЧ����
        uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
        uint8_t radius =
            (TaowuEnemyDeadTime - m_pdata->deathData.deathTimer) * 30 / TaowuEnemyDeadTime; // ��0����������ֵ5
        radius = etl::max(radius, uint8_t(1));                                              // ��С�뾶����

        OLED_DrawCircle(centerX, centerY, radius, OLED_COLOR_NORMAL);
    }
}

void TaowuEnemy::die() {
    // Implement enemy death logic
    if (m_pdata->deathData.deathTimer > 0) {
        m_pdata->deathData.deathTimer -= controlDelayTime;
        m_pdata->deathData.deathTimer = etl::max(m_pdata->deathData.deathTimer, uint16_t(0));
        return;
    }

    m_pdata->isActive = false;
}

//��������
void TaowuEnemy::fireContinuousMassiveBasicBullets() {
    if (action_count < 1000 / BulletsPerSecond) // ÿ125ms����һ�� (1000/8)
        return;
    action_count = 0;

    if (!positionChange) {
        //�ı�λ��
        m_pdata->spatialData.currentPosX = 63;
        m_pdata->spatialData.currentPosY = 1;
        m_pdata->spatialData.refPosX     = 63;
        m_pdata->spatialData.refPosY     = 1;
        positionChange                   = true;
    }

    //����������ͨ�ӵ�
    uint8_t m_x = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t m_y = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

    uint8_t offsetY = (rand() % 61) - 30; // -30 �� +30 ������ƫ��

    //��BOSS���������ģ�����������ȴʱ��
    m_pdata->attackData.shootCooldownTimer = 0; //������ȴʱ�䣬���ٷ����ӵ�
    shoot(m_x, m_y + offsetY, BulletType::BASIC);
}

// ÿ375ms����һ�� (3000/8)
void TaowuEnemy::fireFiveFireballBulletsAtRandom() {
    if (action_count < FiveFireballBulletFireTime / FireballCount) // ÿ375ms����3������
        return;
    action_count = 0;

    if (!positionChange) {
        //�ı�λ��
        m_pdata->spatialData.currentPosX = 63;
        m_pdata->spatialData.currentPosY = 1;
        m_pdata->spatialData.refPosX     = 63;
        m_pdata->spatialData.refPosY     = 1;
        positionChange                   = true;
    }

    //����������
    uint8_t m_x = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t m_y = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

    uint8_t offsetY = (rand() % 61) - 30; // -30 �� +30 ������ƫ��

    m_pdata->attackData.shootCooldownTimer = 0; //������ȴʱ�䣬���ٷ����ӵ�
    shoot(m_x, m_y + offsetY, BulletType::FIRE_BALL);
}

void TaowuEnemy::fireCenterFireballAndSideBasicBullets() {
    if (action_count < action_MaxTime - 10) // action_MaxTime=100ms��90ms������
        return;
    action_count = 0;
    //�����м����򵯣�������ͨ�ӵ�
    uint8_t m_x = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t m_y = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
    //�м�������
    m_pdata->attackData.shootCooldownTimer = 0; //������ȴʱ�䣬���ٷ����ӵ�
    shoot(m_x, m_y, BulletType::FIRE_BALL);
    //������ͨ�ӵ�
    m_pdata->attackData.shootCooldownTimer = 0; //������ȴʱ�䣬���ٷ����ӵ�
    shoot(m_x, m_y + 26, BulletType::BASIC);
    m_pdata->attackData.shootCooldownTimer = 0; //������ȴʱ�䣬���ٷ����ӵ�
    shoot(m_x + 20, m_y + 30, BulletType::BASIC);
    //�Ҳ���ͨ�ӵ�
    m_pdata->attackData.shootCooldownTimer = 0; //������ȴʱ�䣬���ٷ����ӵ�
    shoot(m_x, m_y - 26, BulletType::BASIC);
    m_pdata->attackData.shootCooldownTimer = 0; //������ȴʱ�䣬���ٷ����ӵ�
    shoot(m_x + 20, m_y - 30, BulletType::BASIC);
}

void TaowuEnemy::fireSingleRowNotchedBasicBullets() {
    if (action_count < action_MaxTime - 10) // action_MaxTime=500ms��490ms������
        return;
    action_count = 0;
    //����һ���������͵��ӵ���ֻ���м���ȱ��
    uint8_t m_x = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t m_y = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

    //�����ӵ�������6�����أ������м�λ��
    for (int8_t offsetY = -30; offsetY <= 30; offsetY += 6) {
        if (offsetY >= -12 && offsetY <= 12) {
            // �м�ȱ�ڣ���������
            continue;
        }
        m_pdata->attackData.shootCooldownTimer = 0; //������ȴʱ�䣬���ٷ����ӵ�
        shoot(m_x, m_y + offsetY, BulletType::BASIC);
    }
}

void TaowuEnemy::blinkToRandomPosition() {
    // ʹ�� action_MaxTime/3 ��Ϊ�׶μ��� (BlinkRandomTime=1000ms, ÿ�׶�333.3ms)
    uint16_t phaseInterval = action_MaxTime / 3 - 20; // ��ǰ20msִ�н׶��л�
    if (action_count < phaseInterval) return;
    action_count = 0;

    // ʹ�� action_timer �жϵ�ǰ�׶�
    // action_timer �� action_MaxTime ��ʼ�ݼ�
    // �׶�1: action_timer > action_MaxTime * 2/3  (�տ�ʼ)
    // �׶�2: action_timer �� action_MaxTime * 1/3 �� 2/3 ֮��
    // �׶�3: action_timer < action_MaxTime * 1/3  (������)

    uint16_t phase2Threshold = action_MaxTime * 2 / 3; // Լ666ms
    uint16_t phase3Threshold = action_MaxTime / 3;     // Լ333ms

    if (!positionChange) {
        // �׶�1: ���ֵ�Զ��λ��
        m_pdata->spatialData.currentPosX = 140;
        m_pdata->spatialData.currentPosY = 1;
        m_pdata->spatialData.refPosX     = m_pdata->spatialData.currentPosX;
        m_pdata->spatialData.refPosY     = m_pdata->spatialData.currentPosY;
        positionChange                   = true;
    } else if (action_timer >= phase3Threshold) {
        // �׶�2: ��Զ��λ�÷���3�Ż��򵯣�����Yλ�ã�
        // ע�⣺ÿ�η���ǰ��Ҫ������ȴʱ�䣬��Ϊshoot()�ڲ���������ȴ

        for (int i = 0; i < 3; i++) {
            m_pdata->attackData.shootCooldownTimer = 0;                // ������ÿ��shootǰ����
            uint8_t m_y                            = rand() % 54 + 6;  // 6-59 ����Yλ��
            uint8_t m_x                            = 80 + rand() % 42; // 80-121 ����Xλ��
            shoot(m_x, m_y, BulletType::FIRE_BALL);
        }
    } else {
        // �׶�3: �ָ�ԭλ��
        m_pdata->spatialData.currentPosX = 63;
        m_pdata->spatialData.currentPosY = 1;
        m_pdata->spatialData.refPosX     = 63;
        m_pdata->spatialData.refPosY     = 1;
    }
}

void TaowuEnemy::blinkToPlayerAlignedPosition() {
    if (action_count < action_MaxTime - 10) // ����һ��
        return;
    action_count = 0;

    IRole *player = g_entityManager.getPlayerRole();
    if (player == nullptr) return;

    if (!positionChange) {
        //�ı�λ�ã���������λ��
        uint8_t playerY = player->getData()->spatialData.currentPosY + player->getData()->spatialData.sizeY / 2;
        int8_t  targetY = playerY - m_pdata->spatialData.sizeY / 2;

        //Xλ������
        m_pdata->spatialData.currentPosX = 30 + (rand() % 51); // 30-80 ����λ��

        //ȷ��BOSSλ������Ļ��
        if (targetY < -31) targetY = -31;
        if (targetY > 95) targetY = 95;

        m_pdata->spatialData.currentPosY = targetY;
        m_pdata->spatialData.refPosX     = m_pdata->spatialData.currentPosX;
        m_pdata->spatialData.refPosY     = m_pdata->spatialData.currentPosY;
        positionChange                   = true;
    }
}

/*******************************************************************/

/*******************************************************************/
/**
 * @brief XiangliuEnemy class
 * @note  ���ģ����� �� Ӣ�ģ�Xiangliu,�񻰵��ʣ���ͷ���ι��ޣ����ں�ˮ֮�У���������������֮����ľ�Կݣ������ɺԡ�
 * @note  ��ͷ���ι��ޣ��������綾������֮����ľ�Կݣ������ɺԣ�����������������
 * @note  BOSS�����͵��ˣ����;޴���64x64 ���أ�����Ѫ�����߹������������ƶ���������ʽ�����Ҿ�����в�ԡ�
 * @note  ������ʽ1������������ͨ�ӵ�
 * @note  ������ʽ2��������������
 * @note  ������ʽ3���������Ż�����
 * @note  ������ʽ4������3ֻChiMeiEnemy��Ϊ�ٻ���Эͬ��ս
 * @note  ������ʽ5������2ֻFeilianEnemy��Ϊ�ٻ���Эͬ��ս
 * @note  ������ʽ6������1ֻGudiaoEnemy��Ϊ�ٻ���Эͬ��ս
 */

XiangliuEnemy::XiangliuEnemy(
    uint8_t startX, uint8_t startY, uint8_t initPosX, uint8_t initPosY, uint8_t level, uint16_t dropExp
)
: IRole() {
    //ͼƬ��Ϣ
    m_pdata->img = &XiangliuImg;

    //������Ϣ
    m_pdata->identity          = RoleIdentity::ENEMY;
    m_pdata->isActive          = true;
    m_pdata->initData.isInited = false;

    //�ȼ���Ϣ
    m_pdata->level = level;

    //Ѫ����Ϣ
    //Ѫ���ϵͣ����������ߣ��ƶ��ٶȿ�
    m_pdata->healthData.currentHealth = 30 + level * 900;
    m_pdata->healthData.maxHealth     = 30 + level * 900;

    //��Ѫ��Ϣ
    m_pdata->healthData.healValue       = 30;
    m_pdata->healthData.healTimeCounter = 0;
    m_pdata->healthData.healResetTime   = 15000;
    m_pdata->healthData.healSpeed       = 5;

    //�ռ��ƶ���Ϣ
    m_pdata->spatialData.canCrossBorder            = true;
    m_pdata->spatialData.currentPosX               = startX; // Starting X position
    m_pdata->spatialData.currentPosY               = startY; // Starting Y position
    m_pdata->spatialData.refPosX                   = startX;
    m_pdata->spatialData.refPosY                   = startY;
    m_pdata->spatialData.sizeX                     = m_pdata->img->w;
    m_pdata->spatialData.sizeY                     = m_pdata->img->h;
    m_pdata->spatialData.moveSpeed                 = 2; // Set movement speed
    m_pdata->spatialData.consecutiveCollisionCount = 0;

    //��ʼ��λ��
    m_pdata->initData.posX = initPosX;
    m_pdata->initData.posY = initPosY;

    //������Ϣ
    m_pdata->attackData.attackPower            = 3 + level * 5;
    m_pdata->attackData.shootCooldownSpeed     = 5;
    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 8000; //8000 ms
    m_pdata->attackData.bulletSpeed            = 1;
    //�����ٶ� 15000 ms

    m_pdata->attackData.bulletRange            = 10;   //ֻ�Ի�������Ч
    m_pdata->attackData.bulletDamageMultiplier = 1.5f; //ֻ������������Ч

    m_pdata->attackData.collisionPower = 12 + level * 4;

    //������Ϣ
    m_pdata->heatData.maxHeat          = 250;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 0;
    m_pdata->heatData.heatCoolDownRate = 10; //ÿ����ȴ10��������ÿ����ȴʱ��������200ms

    //����״̬��Ϣ
    m_pdata->deathData.deathTimer           = XiangliuEnemyDeadTime;
    m_pdata->deathData.isDead               = false;
    m_pdata->deathData.dropExperiencePoints = dropExp;

    // Initialize other enemy-specific data here
}

void XiangliuEnemy::shoot(uint8_t x, uint8_t y, BulletType type) {
    // Implement enemy shooting logic
    // Create bullet based on type
    switch (type) {
    case BulletType::BASIC:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot > m_pdata->heatData.maxHeat)
                return;                                             // ���������������޷�����
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // ��ȴ�У��޷�����

            IBullet *newBullet = createBullet(x, y, BulletType::BASIC);
            if (newBullet != nullptr) {
                if (g_entityManager.addBullet(newBullet)) {
                    // Successfully added bullet
                    m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
                    m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot;
                } else {
                    delete newBullet; // Clean up if not added
                }
            }
        }
        break;
    case BulletType::FIRE_BALL:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 2 > m_pdata->heatData.maxHeat)
                return;                                             // ���������������޷�����
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // ��ȴ�У��޷�����

            IBullet *newBullet = createBullet(x, y, BulletType::FIRE_BALL);
            if (newBullet != nullptr) {
                if (g_entityManager.addBullet(newBullet)) {
                    // Successfully added bullet
                    m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
                    m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot;
                } else {
                    delete newBullet; // Clean up if not added
                }
            }
        }
        break;
    case BulletType::LIGHTNING_LINE:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 1.5 > m_pdata->heatData.maxHeat)
                return;                                             // ���������������޷�����
            if (m_pdata->attackData.shootCooldownTimer > 0) return; // ��ȴ�У��޷�����

            IBullet *newBullet = createBullet(x, y, BulletType::LIGHTNING_LINE);
            if (newBullet != nullptr) {
                if (g_entityManager.addBullet(newBullet)) {
                    // Successfully added bullet
                    m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
                    m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot;
                } else {
                    delete newBullet; // Clean up if not added
                }
            }
        }
    }
}

void XiangliuEnemy::init() {
    m_pdata->initData.init_count += controlDelayTime;
    // Initialize enemy role specifics

    if (m_pdata->spatialData.currentPosX > m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 60) { // ÿ60ms�ƶ�һ��
            m_pdata->spatialData.currentPosX -= 1;
            m_pdata->initData.init_count = 0;
        }
    } else if (m_pdata->spatialData.currentPosX < m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 60) { // ÿ60ms�ƶ�һ��
            m_pdata->spatialData.currentPosX += 1;
            m_pdata->initData.init_count = 0;
        }
    } else {
        m_pdata->initData.isInited   = true;
        m_pdata->spatialData.refPosX = m_pdata->spatialData.currentPosX;
        m_pdata->spatialData.refPosY = m_pdata->spatialData.currentPosY;
        m_pdata->initData.init_count = 0;
    }
}

void XiangliuEnemy::think() {
    // Implement enemy AI logic
    think_count += controlDelayTime;
    if (think_count < 100) // ÿ100ms����һ���ж�
        return;

    think_count = 0;

    uint8_t randomAction = rand() % 6;
    // Random action: 0 - move left, 1 - move right, 2 - move down, 3 - move up, 4 - stay still, 5 - shoot
    if (m_pdata->actionData.currentState == ActionState::IDLE) {
        //�ƶ�
        if (randomAction == 0) {
            m_pdata->actionData.moveMode     = MoveMode::LEFT;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 1) {
            m_pdata->actionData.moveMode     = MoveMode::RIGHT;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 2) {
            m_pdata->actionData.moveMode     = MoveMode::DOWN;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 3) {
            m_pdata->actionData.moveMode     = MoveMode::UP;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 4) {
            // Stay still
            m_pdata->actionData.moveMode     = MoveMode::NONE;
            m_pdata->actionData.currentState = ActionState::MOVING;
        }

        //����
        else if (randomAction == 5) {
            if (m_pdata->attackData.shootCooldownTimer > 0) {
                // ��������ȴ�У��򲻽��й��������ֿ���״̬
                m_pdata->actionData.moveMode     = MoveMode::NONE;
                m_pdata->actionData.currentState = ActionState::IDLE;
                return;
            }

            uint8_t randomAttackMode = rand() % 6 + 1; // 1-6 ������ʽ
            if (randomAttackMode > 3 && g_entityManager.m_roles.size() >= 4)
                randomAttackMode -= 3; // �������ϵ��˹��࣬�������ٻ��﹥����ʽ�ĸ���

            m_pdata->actionData.currentState = ActionState::ATTACKING;

            switch (randomAttackMode) {
            case 1:
                action_count   = 0;
                action_timer   = 300; // ����������������ʱ��300ms
                action_MaxTime = action_timer;

                m_pdata->actionData.attackMode = AttackMode::MODE_1;
                break;
            case 2:
                action_count   = 0;
                action_timer   = 300; // ����������������ʱ��300ms
                action_MaxTime = action_timer;

                m_pdata->actionData.attackMode = AttackMode::MODE_2;
                break;

            case 3:
                action_count   = 0;
                action_timer   = 300; // ����������������ʱ��300ms
                action_MaxTime = action_timer;

                m_pdata->actionData.attackMode = AttackMode::MODE_3;
                break;
            case 4:
                action_count   = 0;
                action_timer   = 300; // ����������������ʱ��300ms
                action_MaxTime = action_timer;

                m_pdata->actionData.attackMode = AttackMode::MODE_4;
                break;
            case 5:
                action_count   = 0;
                action_timer   = 300; // ����������������ʱ��300ms
                action_MaxTime = action_timer;

                m_pdata->actionData.attackMode = AttackMode::MODE_5;
                break;
            case 6:
                action_count   = 0;
                action_timer   = 300; // ����������������ʱ��300ms
                action_MaxTime = action_timer;

                m_pdata->actionData.attackMode = AttackMode::MODE_6;
                break;
            default:
                break;
            }
        }
    }
}

void XiangliuEnemy::doAction() {
    if (m_pdata->initData.isInited == false) {
        return;
    }

    // Implement enemy action logic
    if (m_pdata->deathData.isDead) {
        return;
    }
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
        // Move logic handled in think()
        break;
    case ActionState::ATTACKING:
        //��������ʱ�䣬�������ڶ���������Ƶ��
        action_count += controlDelayTime;

        //��������ʱ
        if (action_timer >= controlDelayTime)
            action_timer -= controlDelayTime;
        else
            action_timer = 0;

        switch (m_pdata->actionData.attackMode) {
        //ִ�й�������
        case AttackMode::MODE_1:
            fireNineRowsBasicBullets();
            break;
        case AttackMode::MODE_2:
            fireThreeRowsLightningBullets();
            break;
        case AttackMode::MODE_3:
            fireThreeRowsFireballBullets();
            break;
        case AttackMode::MODE_4:
            summonThreeChiMeiMinions();
            break;
        case AttackMode::MODE_5:
            summonTwoFeilianMinions();
            break;
        case AttackMode::MODE_6:
            summonOneGudiaoMinion();
            break;
        default:
            break;
        }

        if (action_timer == 0) {
            m_pdata->actionData.currentState       = ActionState::IDLE;
            m_pdata->actionData.attackMode         = AttackMode::NONE;
            action_count                           = 0;
            m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime; // ������������ȴʱ��
        }
        break;
    }
}

void XiangliuEnemy::drawRole() {
    if (m_pdata->img != nullptr && m_pdata->isActive && !m_pdata->deathData.isDead) {
        OLED_DrawImage(
            m_pdata->spatialData.currentPosX, m_pdata->spatialData.currentPosY, m_pdata->img, OLED_COLOR_NORMAL
        );
    }

    if (m_pdata->deathData.isDead) {
        // Draw death animation or effect
        // ������������������һ���򵥵�ԲȦ��ʾ��ʧЧ����
        uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
        uint8_t radius =
            (XiangliuEnemyDeadTime - m_pdata->deathData.deathTimer) * 30 / XiangliuEnemyDeadTime; // ��0����������ֵ5
        radius = etl::max(radius, uint8_t(1));                                                    // ��С�뾶����

        OLED_DrawCircle(centerX, centerY, radius, OLED_COLOR_NORMAL);
    }
}

void XiangliuEnemy::die() {
    // Implement enemy death logic
    if (m_pdata->deathData.deathTimer > 0) {
        m_pdata->deathData.deathTimer -= controlDelayTime;
        m_pdata->deathData.deathTimer = etl::max(m_pdata->deathData.deathTimer, uint16_t(0));
        return;
    }

    m_pdata->isActive = false;
}

//��������
void XiangliuEnemy::fireNineRowsBasicBullets() {
    //����������ͨ�ӵ�
    if (action_count < action_MaxTime - 10) // ����һ��
        return;
    action_count = 0;

    uint8_t m_x = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t m_y = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
    //�����ӵ�������2�����أ��м����п�϶
    int8_t offsetYList[9] = {-31, -29, -27, -2, 0, 2, 27, 29, 31};
    for (uint8_t i = 0; i < 9; i++) {
        m_pdata->attackData.shootCooldownTimer = 0; //������ȴʱ�䣬���ٷ����ӵ�
        shoot(m_x, m_y + offsetYList[i], BulletType::BASIC);
        m_pdata->attackData.shootCooldownTimer = 0; //������ȴʱ�䣬���ٷ����ӵ�
        shoot(m_x + 18, m_y + offsetYList[i], BulletType::BASIC);
    }
}

void XiangliuEnemy::fireThreeRowsLightningBullets() {
    if (action_count < action_MaxTime - 10) // ����һ��
        return;
    action_count = 0;

    //����λ��
    m_pdata->spatialData.currentPosX = 64 + 14;
    m_pdata->spatialData.refPosX     = 64 + 14;
    m_pdata->spatialData.currentPosY = 1;
    m_pdata->spatialData.refPosY     = 1;

    //������������

    uint8_t m_y = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
    //�����ӵ�������21������
    int8_t offsetYList[3] = {-27, 0, 27};
    for (uint8_t i = 0; i < 3; i++) {
        m_pdata->attackData.shootCooldownTimer = 0; //������ȴʱ�䣬���ٷ����ӵ�
        shoot(1, m_y + offsetYList[i], BulletType::LIGHTNING_LINE);
    }
}

void XiangliuEnemy::fireThreeRowsFireballBullets() {
    if (action_count < action_MaxTime - 10) // ����һ��
        return;
    action_count = 0;
    //�������Ż�����
    uint8_t m_x = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t m_y = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
    //�����ӵ�������21������
    int8_t offsetYList[3] = {-21, 0, 21};
    for (uint8_t i = 0; i < 3; i++) {
        m_pdata->attackData.shootCooldownTimer = 0; //������ȴʱ�䣬���ٷ����ӵ�
        shoot(m_x, m_y + offsetYList[i], BulletType::FIRE_BALL);
    }
}

void XiangliuEnemy::summonThreeChiMeiMinions() {
    if (action_count < action_MaxTime - 10) // �ٻ�һ��
        return;
    action_count = 0;

    uint8_t posX[3] = {20, 40, 60};
    uint8_t posY[3] = {1, 32, 50};

    for (uint8_t i = 0; i < 3; i++) {
        IRole *minion = new ChiMeiEnemy(posX[i], posY[i], posX[i], posY[i], rand() % 3 + 1);
        if (minion != nullptr) {
            if (!g_entityManager.addRole(minion)) delete minion; // Clean up if not added
        }
    }
}

void XiangliuEnemy::summonTwoFeilianMinions() {
    if (action_count < action_MaxTime - 10) // �ٻ�һ��
        return;
    action_count = 0;

    uint8_t posX[2] = {50, 50};
    uint8_t posY[2] = {0, 50};
    for (uint8_t i = 0; i < 2; i++) {
        IRole *minion = new FeilianEnemy(posX[i], posY[i], posX[i], posY[i], rand() % 3 + 1);
        if (minion != nullptr) {
            if (!g_entityManager.addRole(minion)) delete minion; // Clean up if not added
        }
    }
}

void XiangliuEnemy::summonOneGudiaoMinion() {
    if (action_count < action_MaxTime - 10) // �ٻ�һ��
        return;
    action_count = 0;

    uint8_t posX = 50;
    uint8_t posY = 25;

    IRole *minion = new GudiaoEnemy(posX, posY, posX, posY, rand() % 3 + 1);
    if (minion != nullptr) {
        if (!g_entityManager.addRole(minion)) delete minion; // Clean up if not added
    }
}

/*******************************************************************/

/*******************************************************************/
/**
 * @brief HundunEnemy class - ���� BOSS������֮�ף�
 * @note  ���ģ����� �� Ӣ�ģ�Hundun
 * @note  �񻰵��ʣ�����֮һ����ɽ�������������������ң����絤������������������Ŀ��
 * @note  ��ׯ�ӡ���"��������������"�ĵ��ʣ��������ҡ�������δ�ֻ���ԭʼ״̬��
 * 
 * @note  BOSS�����͵��ˣ����;޴���68x64 ���أ�������Ѫ��������֮�ף����еȹ�������
 * @note  �����ƶ�����˲�ƣ�������ʽ�Ը��źͻ���Ϊ����
 * 
 * @note  === ������ʽ ===
 * @note  MODE_1: ����ӿ�� - ������˸�ƶ���ͬʱ��4������������ͨ�ӵ�
 *               ����ʱ�� ChaosSurgeTime=3000ms��ÿ ChaosSurgeInterval=500ms ��˸������һ��
 * @note  MODE_2: ���Ϸ�ӡ - ����Ļ������7����˸���������ڵ���Ұ
 *               ����ʱ�� SealAperturesTime=2000ms����Ӧ"��������������"����
 * @note  MODE_3: ����ǣ�� - ������������λ�û���������ͬʱ����׷�ٻ�����
 *               ����ʱ�� VoidPullTime=2500ms��ÿ VoidPullInterval=300ms ����һ��
 * @note  MODE_4: �������� - ����ʽ�����ӵ���Yλ�ð����Ҳ�ɨ��
 *               ����ʱ�� ChaoticBarrageTime=2000ms��ÿ ChaoticBarrageInterval=150ms ����һ��
 * @note  MODE_5: ʱ����϶ - ���ٷ���������ȱ�ڵĵ�Ļǽ��ȱ��λ��ÿ�ֱ仯
 *               ����ʱ�� TemporalRiftTime=2500ms��ÿ TemporalRiftInterval=400ms ����һ��
 * @note  MODE_6: ���ڻ��� - ȫ����Ļ�������м��а�ȫȱ�ڣ�Ѫ��Խ��ȱ��ԽС
 *               ����ʱ�� ReturnToChaosWarning=500ms������ʱ�� ReturnToChaosTime=100ms
 */

//��ֵ�趨�ο�������֮�ף���ǿBOSS��
//Ѫ����200 + level * 1000������Ѫ����
//��������8 + level * 4���еȹ�������
//�ƶ��ٶȣ�1�������ƶ�����������������
//������ȴʱ�䣺6000ms
//��ײ�˺���15 + level * 5������ײ�˺���

HundunEnemy::HundunEnemy(
    uint8_t startX, uint8_t startY, uint8_t initPosX, uint8_t initPosY, uint8_t level, uint16_t dropExp
)
: IRole() {
    //ͼƬ��Ϣ
    m_pdata->img = &HundunImg;

    //������Ϣ
    m_pdata->identity          = RoleIdentity::ENEMY;
    m_pdata->isActive          = true;
    m_pdata->initData.isInited = false;

    //�ȼ���Ϣ
    m_pdata->level = level;

    //Ѫ����Ϣ
    //����֮�ף�Ѫ������
    m_pdata->healthData.currentHealth = 200 + level * 1000;
    m_pdata->healthData.maxHealth     = 200 + level * 1000;

    //��Ѫ��Ϣ
    m_pdata->healthData.healValue       = 50; // �ϸߵĻ�Ѫ��
    m_pdata->healthData.healTimeCounter = 0;
    m_pdata->healthData.healResetTime   = 12000; // 12����Ѫ����
    m_pdata->healthData.healSpeed       = 5;

    //�ռ��ƶ���Ϣ
    m_pdata->spatialData.canCrossBorder            = true;
    m_pdata->spatialData.currentPosX               = startX;
    m_pdata->spatialData.currentPosY               = startY;
    m_pdata->spatialData.refPosX                   = startX;
    m_pdata->spatialData.refPosY                   = startY;
    m_pdata->spatialData.sizeX                     = m_pdata->img->w; // 68
    m_pdata->spatialData.sizeY                     = m_pdata->img->h; // 64
    m_pdata->spatialData.moveSpeed                 = 1;               // �����ƶ�����������
    m_pdata->spatialData.consecutiveCollisionCount = 0;

    //��ʼ��λ��
    m_pdata->initData.posX = initPosX;
    m_pdata->initData.posY = initPosY;

    //������Ϣ
    m_pdata->attackData.attackPower            = 8 + level * 4; // �еȹ�����
    m_pdata->attackData.shootCooldownSpeed     = 5;
    m_pdata->attackData.shootCooldownTimer     = 0;
    m_pdata->attackData.shootCooldownResetTime = 6000; // 6000ms ������ȴ
    m_pdata->attackData.bulletSpeed            = 1;

    m_pdata->attackData.bulletRange            = 12;   // ���򵯷�Χ
    m_pdata->attackData.bulletDamageMultiplier = 1.8f; // ���������˺�����

    m_pdata->attackData.collisionPower = 15 + level * 5; // ����ײ�˺�

    //������Ϣ��BOSS���������ƣ�
    m_pdata->heatData.maxHeat          = 500;
    m_pdata->heatData.currentHeat      = 0;
    m_pdata->heatData.heatPerShot      = 0; // BOSS����������
    m_pdata->heatData.heatCoolDownRate = 10;

    //����״̬��Ϣ
    m_pdata->deathData.deathTimer           = HundunEnemyDeadTime;
    m_pdata->deathData.isDead               = false;
    m_pdata->deathData.dropExperiencePoints = dropExp;

    // ��ʼ������ģʽ״̬����
    positionChange     = false;
    aperturesGenerated = false;
    warningDisplayed   = false;
    spiralPhase        = 0;
    riftWaveCount      = 0;
}

void HundunEnemy::shoot(uint8_t x, uint8_t y, BulletType type) {
    // ������BOSS����һ�µ������߼�
    switch (type) {
    case BulletType::BASIC:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot > m_pdata->heatData.maxHeat) return;
            if (m_pdata->attackData.shootCooldownTimer > 0) return;

            IBullet *newBullet = createBullet(x, y, BulletType::BASIC);
            if (newBullet != nullptr) {
                if (g_entityManager.addBullet(newBullet)) {
                    m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
                    m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot;
                } else {
                    delete newBullet;
                }
            }
        }
        break;
    case BulletType::FIRE_BALL:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 2 > m_pdata->heatData.maxHeat) return;
            if (m_pdata->attackData.shootCooldownTimer > 0) return;

            IBullet *newBullet = createBullet(x, y, BulletType::FIRE_BALL);
            if (newBullet != nullptr) {
                if (g_entityManager.addBullet(newBullet)) {
                    m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
                    m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot;
                } else {
                    delete newBullet;
                }
            }
        }
        break;
    case BulletType::LIGHTNING_LINE:
        {
            if (m_pdata->heatData.currentHeat + m_pdata->heatData.heatPerShot * 1.5 > m_pdata->heatData.maxHeat) return;
            if (m_pdata->attackData.shootCooldownTimer > 0) return;

            IBullet *newBullet = createBullet(x, y, BulletType::LIGHTNING_LINE);
            if (newBullet != nullptr) {
                if (g_entityManager.addBullet(newBullet)) {
                    m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;
                    m_pdata->heatData.currentHeat += m_pdata->heatData.heatPerShot;
                } else {
                    delete newBullet;
                }
            }
        }
        break;
    }
}

void HundunEnemy::init() {
    m_pdata->initData.init_count += controlDelayTime;

    // ����Ļ�⻺���ƶ�����ʼλ��
    if (m_pdata->spatialData.currentPosX > m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 60) { // ÿ60ms�ƶ�һ��
            m_pdata->spatialData.currentPosX -= 1;
            m_pdata->initData.init_count = 0;
        }
    } else if (m_pdata->spatialData.currentPosX < m_pdata->initData.posX) {
        if (m_pdata->initData.init_count >= 60) {
            m_pdata->spatialData.currentPosX += 1;
            m_pdata->initData.init_count = 0;
        }
    } else {
        m_pdata->initData.isInited   = true;
        m_pdata->spatialData.refPosX = m_pdata->spatialData.currentPosX;
        m_pdata->spatialData.refPosY = m_pdata->spatialData.currentPosY;
        m_pdata->initData.init_count = 0;
    }
}

void HundunEnemy::think() {
    think_count += controlDelayTime;
    if (think_count < 100) // ÿ100ms����һ���ж�
        return;

    think_count = 0;

    uint8_t randomAction = rand() % 6;
    // �����ж�: 0-3 �ƶ�, 4 ��ֹ, 5 ����

    if (m_pdata->actionData.currentState == ActionState::IDLE) {
        // �ƶ�����
        if (randomAction == 0) {
            m_pdata->actionData.moveMode     = MoveMode::LEFT;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 1) {
            m_pdata->actionData.moveMode     = MoveMode::RIGHT;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 2) {
            m_pdata->actionData.moveMode     = MoveMode::DOWN;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 3) {
            m_pdata->actionData.moveMode     = MoveMode::UP;
            m_pdata->actionData.currentState = ActionState::MOVING;
        } else if (randomAction == 4) {
            m_pdata->actionData.moveMode     = MoveMode::NONE;
            m_pdata->actionData.currentState = ActionState::MOVING;
        }

        // ��������
        else if (randomAction == 5) {
            if (m_pdata->attackData.shootCooldownTimer > 0) {
                // ��ȴ�У����ֿ���
                m_pdata->actionData.moveMode     = MoveMode::NONE;
                m_pdata->actionData.currentState = ActionState::IDLE;
                return;
            }

            uint8_t randomAttackMode         = rand() % 6 + 1; // 1-6 ������ʽ
            m_pdata->actionData.currentState = ActionState::ATTACKING;

            switch (randomAttackMode) {
            case 1:
                // MODE_1: ����ӿ�� - ������˸�ƶ�����8���������ӵ�
                action_timer   = ChaosSurgeTime; // 3000ms
                action_MaxTime = action_timer;
                action_count   = 0;
                positionChange = false;

                m_pdata->actionData.attackMode = AttackMode::MODE_1;
                break;

            case 2:
                // MODE_2: ���Ϸ�ӡ - ����7����������
                action_timer       = SealAperturesTime; // 2000ms
                action_MaxTime     = action_timer;
                action_count       = 0;
                aperturesGenerated = false;

                m_pdata->actionData.attackMode = AttackMode::MODE_2;
                break;

            case 3:
                // MODE_3: ����ǣ�� - �������Ҳ���������
                action_timer   = VoidPullTime; // 2500ms
                action_MaxTime = action_timer;
                action_count   = 0;

                m_pdata->actionData.attackMode = AttackMode::MODE_3;
                break;

            case 4:
                // MODE_4: �������� - ����ʽ�����ӵ�
                action_timer   = ChaoticBarrageTime; // 2000ms
                action_MaxTime = action_timer;
                action_count   = 0;
                positionChange = false;
                spiralPhase    = 0; // ����������λ

                m_pdata->actionData.attackMode = AttackMode::MODE_4;
                break;

            case 5:
                // MODE_5: ʱ����϶ - ����������ȱ�ڵĵ�Ļǽ
                action_timer   = TemporalRiftTime; // 2500ms
                action_MaxTime = action_timer;
                action_count   = 0;
                positionChange = false;
                riftWaveCount  = 0; // ���ò��μ���

                m_pdata->actionData.attackMode = AttackMode::MODE_5;
                break;

            case 6:
                // MODE_6: ���ڻ��� - ȫ����Ļ����
                action_timer     = ReturnToChaosWarning + ReturnToChaosTime; // 600ms
                action_MaxTime   = action_timer;
                action_count     = 0;
                warningDisplayed = false;

                m_pdata->actionData.attackMode = AttackMode::MODE_6;
                break;

            default:
                break;
            }
        }
    }
}

void HundunEnemy::doAction() {
    if (m_pdata->initData.isInited == false) {
        return;
    }

    if (m_pdata->deathData.isDead) {
        return;
    }

    switch (m_pdata->actionData.currentState) {
    case ActionState::IDLE:
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

    case ActionState::ATTACKING:
        // ������ʱ
        action_count += controlDelayTime;

        // ��������ʱ
        if (action_timer >= controlDelayTime)
            action_timer -= controlDelayTime;
        else
            action_timer = 0;

        // ִ�ж�Ӧ��������
        switch (m_pdata->actionData.attackMode) {
        case AttackMode::MODE_1:
            chaosSurge();
            break;
        case AttackMode::MODE_2:
            sealSevenApertures();
            break;
        case AttackMode::MODE_3:
            voidPull();
            break;
        case AttackMode::MODE_4:
            chaoticBarrage();
            break;
        case AttackMode::MODE_5:
            temporalRift();
            break;
        case AttackMode::MODE_6:
            returnToChaos();
            break;
        default:
            break;
        }

        // ��������
        if (action_timer == 0) {
            m_pdata->actionData.currentState       = ActionState::IDLE;
            m_pdata->actionData.attackMode         = AttackMode::NONE;
            action_count                           = 0;
            m_pdata->attackData.shootCooldownTimer = m_pdata->attackData.shootCooldownResetTime;

            // ����״̬����
            positionChange     = false;
            aperturesGenerated = false;
            warningDisplayed   = false;
            spiralPhase        = 0;
            riftWaveCount      = 0;
        }
        break;
    }
}

void HundunEnemy::drawRole() {
    if (m_pdata->img != nullptr && m_pdata->isActive && !m_pdata->deathData.isDead) {
        // ��������
        OLED_DrawImage(
            m_pdata->spatialData.currentPosX, m_pdata->spatialData.currentPosY, m_pdata->img, OLED_COLOR_NORMAL
        );

        // MODE_2: �������Ϸ�ӡ��������
        if (m_pdata->actionData.attackMode == AttackMode::MODE_2 && aperturesGenerated) {
            // ��˸Ч��������ʱ���л���ʾ
            bool showApertures = ((action_timer / 100) % 2 == 0);
            if (showApertures) {
                for (uint8_t i = 0; i < ApertureCount; i++) {
                    uint8_t ax = aperturePositions[i][0];
                    uint8_t ay = aperturePositions[i][1];
                    // ���Ƹ���������8x8���ص����䷽�飩
                    OLED_DrawFilledRectangle(ax, ay, 8, 8, OLED_COLOR_NORMAL);
                }
            }
        }

        // MODE_6: ���ƾ���Ч��
        if (m_pdata->actionData.attackMode == AttackMode::MODE_6 && !warningDisplayed) {
            // �����׶Σ���Ļ��Ե��˸
            if ((action_timer / 50) % 2 == 0) {
                OLED_DrawRectangle(0, 0, 127, 63, OLED_COLOR_NORMAL);
                OLED_DrawRectangle(1, 1, 125, 61, OLED_COLOR_NORMAL);
            }
        }
    }

    // ��������
    if (m_pdata->deathData.isDead) {
        uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;
        uint8_t radius  = (HundunEnemyDeadTime - m_pdata->deathData.deathTimer) * 35 / HundunEnemyDeadTime;
        radius          = etl::max(radius, uint8_t(1));

        // ��������Ч����������ɢԲ��
        OLED_DrawCircle(centerX, centerY, radius, OLED_COLOR_NORMAL);
        if (radius > 5) {
            OLED_DrawCircle(centerX, centerY, radius - 5, OLED_COLOR_NORMAL);
        }
    }
}

void HundunEnemy::die() {
    if (m_pdata->deathData.deathTimer > 0) {
        m_pdata->deathData.deathTimer -= controlDelayTime;
        m_pdata->deathData.deathTimer = etl::max(m_pdata->deathData.deathTimer, uint16_t(0));
        return;
    }

    m_pdata->isActive = false;
}

//=========================== ��������ʵ�� ===========================

/**
 * @brief MODE_1: ����ӿ��
 * @note  ������˸�ƶ���ͬʱ��4������������ͨ�ӵ�
 *        ��Ӧ����"�޶���"�����ԣ���Ļ������
 */
void HundunEnemy::chaosSurge() {
    if (action_count < ChaosSurgeInterval) // ÿ500msִ��һ��
        return;
    action_count = 0;

    // �������ֵ���λ��
    uint8_t newX = 30 + rand() % 71;  // 30-100 ����Xλ��
    int8_t  newY = -20 + rand() % 70; // -20-49 ����Yλ�ã��ɲ��ֳ�����Ļ��

    // ����Yλ�÷�Χ
    if (newY < -30) newY = -30;
    if (newY > 60) newY = 60;

    m_pdata->spatialData.currentPosX = newX;
    m_pdata->spatialData.currentPosY = newY;
    m_pdata->spatialData.refPosX     = newX;
    m_pdata->spatialData.refPosY     = newY;

    // ��������λ��
    uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

    // ��4������������ͨ�ӵ����������ң������ٵ�Ļ��
    // �ӵ�����λ����BOSS��Ե
    int8_t directions[4][2] = {
        {-1, 0 }, // ��
        {1,  0 }, // ��
        {0,  -1}, // ��
        {0,  1 }  // ��
    };

    for (uint8_t i = 0; i < 4; i++) {
        m_pdata->attackData.shootCooldownTimer = 0; // ������ȴ
        uint8_t bulletX                        = centerX + directions[i][0] * 20;
        uint8_t bulletY                        = centerY + directions[i][1] * 20;
        shoot(bulletX, bulletY, BulletType::BASIC);
    }
}

/**
 * @brief MODE_2: ���Ϸ�ӡ
 * @note  ����Ļ������7����˸���������ڵ���Ұ
 *        ��Ӧ"��������������"�ĵ���
 */
void HundunEnemy::sealSevenApertures() {
    // ֻ�ڼ��ܿ�ʼʱ����һ�θ�������λ��
    if (!aperturesGenerated) {
        for (uint8_t i = 0; i < ApertureCount; i++) {
            // �������ɸ�������λ�ã������ص���Ҫ��Ϸ������
            aperturePositions[i][0] = rand() % 100 + 10; // 10-109 Xλ��
            aperturePositions[i][1] = rand() % 48 + 8;   // 8-55 Yλ��
        }
        aperturesGenerated = true;
    }

    // ���������Ļ����� drawRole() ������
    // �˴������Ӷ����߼�����ÿ��һ��ʱ����������λ�ã�
}

/**
 * @brief MODE_3: ����ǣ��
 * @note  ������������λ�û���������ͬʱ����׷�ٻ�����
 *        ��Ӧ������������������
 */
void HundunEnemy::voidPull() {
    if (action_count < VoidPullInterval) // ÿ300msִ��һ��
        return;
    action_count = 0;

    IRole *player = g_entityManager.getPlayerRole();
    if (player == nullptr) return;

    // ���㷽������
    int16_t bossX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    int16_t bossY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

    int16_t playerX = player->getData()->spatialData.currentPosX + player->getData()->spatialData.sizeX / 2;
    int16_t playerY = player->getData()->spatialData.currentPosY + player->getData()->spatialData.sizeY / 2;

    int16_t deltaX = bossX - playerX;
    int16_t deltaY = bossY - playerY;

    int8_t dirX = 0, dirY = 0;
    if (deltaX < 0) dirX = -1;
    if (deltaX > 0) dirX = 1;
    if (deltaY < 0) dirY = -1;
    if (deltaY > 0) dirY = 1;

    // �������ң�ǿ���ƶ���
    for (uint8_t i = 0; i < VoidPullDistance / 2; i++) {
        player->move(dirX, dirY, true);
    }

    // ͬʱ�������򵯣������ҷ�����
    m_pdata->attackData.shootCooldownTimer = 0;
    shoot(bossX, bossY, BulletType::FIRE_BALL);
}

/**
 * @brief MODE_4: ��������
 * @note  ����ʽ�����ӵ����ӵ������Ҳ��δ��ϵ���ɨ��
 *        ���ֻ�����"��ת����"���ԣ���Taowu��������Ļ����
 */
void HundunEnemy::chaoticBarrage() {
    if (action_count < ChaoticBarrageInterval) // ÿ150ms����һ��
        return;
    action_count = 0;

    // ��һ��ִ��ʱ�ƶ����м�λ��
    if (!positionChange) {
        m_pdata->spatialData.currentPosX = 60;
        m_pdata->spatialData.currentPosY = 0;
        m_pdata->spatialData.refPosX     = 60;
        m_pdata->spatialData.refPosY     = 0;
        positionChange                   = true;
        spiralPhase                      = 0; // ����������λ
    }

    uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
    uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

    // ����ɨ�䣺Yλ�ð���λ���ϵ�������ɨ
    // ʹ�����Ǻ������ƣ���λ0-8��ӦYƫ�ƴ�-25��+25�ٻص�-25
    int8_t  yOffset = 0;
    uint8_t phase   = spiralPhase % 16; // 16����λΪһ������
    if (phase < 8) {
        yOffset = -25 + phase * 6; // 0->-25, 1->-19, ..., 7->17
    } else {
        yOffset = 25 - (phase - 8) * 6; // 8->25, 9->19, ..., 15->-17
    }

    m_pdata->attackData.shootCooldownTimer = 0;
    shoot(centerX, centerY + yOffset, BulletType::BASIC);

    spiralPhase++; // ������λ
}

/**
 * @brief MODE_5: ʱ����϶
 * @note  ���ٷ���һ�ŵ�Ļǽ������һ������λ�õ�Сȱ��
 *        ȱ��λ��ÿ�ֱ仯�����������ٷ�ӦѰ�Ұ�ȫλ��
 *        ��Ӧ����"����δ��"ʱ�ս���������
 */
void HundunEnemy::temporalRift() {
    if (action_count < TemporalRiftInterval) // ÿ400ms����һ��
        return;
    action_count = 0;

    // ��һ��ִ��ʱ�ƶ����Ҳ�
    if (!positionChange) {
        m_pdata->spatialData.currentPosX = 60;
        m_pdata->spatialData.currentPosY = 0;
        m_pdata->spatialData.refPosX     = 60;
        m_pdata->spatialData.refPosY     = 0;
        positionChange                   = true;
    }

    uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;

    // ��������ȱ��λ�ã�8-52��Χ����֤ȱ���ڿɼ�������
    uint8_t gapCenter = 8 + rand() % 44; // ȱ������Yλ��
    uint8_t gapSize   = RiftGapSize;     // ȱ�ڴ�С��12���أ�

    // ����һ���ӵ�������ȱ������
    for (uint8_t y = 2; y < 62; y += 6) {
        // �ж��Ƿ���ȱ�ڷ�Χ��
        if (y >= gapCenter - gapSize / 2 && y <= gapCenter + gapSize / 2) {
            continue; // ����ȱ��
        }
        m_pdata->attackData.shootCooldownTimer = 0;
        shoot(centerX, y, BulletType::BASIC);
    }

    // ÿ2�ֶ��ⷢ��һ��������������в
    riftWaveCount++;
    if (riftWaveCount % 2 == 0) {
        m_pdata->attackData.shootCooldownTimer = 0;
        shoot(centerX, gapCenter, BulletType::FIRE_BALL); // ������ȱ��λ�ã���ʹ�����ƶ�
    }
}

/**
 * @brief MODE_6: ���ڻ���
 * @note  ȫ����Ļ�������м��а�ȫȱ�ڣ�Ѫ��Խ��ȱ��ԽС
 *        �ռ����ܣ�������������һ��
 */
void HundunEnemy::returnToChaos() {
    // �׶�1: �����׶� (ǰ500ms)
    if (action_timer > ReturnToChaosTime) {
        // ����Ч���� drawRole() �л���
        return;
    }

    // �׶�2: ���䵯Ļ (����100msִֻ��һ��)
    if (!warningDisplayed) {
        warningDisplayed = true;

        // �ƶ����м�λ��
        m_pdata->spatialData.currentPosX = 60;
        m_pdata->spatialData.currentPosY = 0;
        m_pdata->spatialData.refPosX     = 60;
        m_pdata->spatialData.refPosY     = 0;

        uint8_t centerX = m_pdata->spatialData.currentPosX + m_pdata->spatialData.sizeX / 2;
        uint8_t centerY = m_pdata->spatialData.currentPosY + m_pdata->spatialData.sizeY / 2;

        // ���㰲ȫȱ�ڴ�С��Ѫ��Խ�ͣ�ȱ��ԽС
        // Ѫ��100%ʱȱ��20���أ�Ѫ��0%ʱȱ��8����
        float  healthRatio = (float)m_pdata->healthData.currentHealth / (float)m_pdata->healthData.maxHealth;
        int8_t gapSize     = 8 + (int8_t)(healthRatio * 12); // 8-20����

        // ����ȫ����Ļ���м���ȱ��
        for (int8_t offsetY = -30; offsetY <= 30; offsetY += 5) {
            // �����м�ȱ������
            if (offsetY >= -gapSize / 2 && offsetY <= gapSize / 2) {
                continue;
            }
            m_pdata->attackData.shootCooldownTimer = 0;
            shoot(centerX, centerY + offsetY, BulletType::BASIC);
            // �ڶ����ӵ������ӵ�Ļ�ܶ�
            m_pdata->attackData.shootCooldownTimer = 0;
            shoot(centerX + 15, centerY + offsetY + 2, BulletType::BASIC);
        }

        // ���ⷢ�����Ż�����������в��
        m_pdata->attackData.shootCooldownTimer = 0;
        shoot(centerX, centerY - gapSize / 2 - 5, BulletType::FIRE_BALL);
        m_pdata->attackData.shootCooldownTimer = 0;
        shoot(centerX, centerY + gapSize / 2 + 5, BulletType::FIRE_BALL);
    }
}

/*******************************************************************/
