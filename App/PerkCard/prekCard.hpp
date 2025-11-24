#ifndef PREKCARD_HPP
#define PREKCARD_HPP

#include "stdint.h"

enum class PerkCardType {
    HEAL_SPEED_UP,           // 1. 恢复速度+2
    HEAL_AMOUNT_UP,          // 2. 恢复量+5
    HEALTH_UP,               // 3. 生命值+100
    ATTACK_UP,               // 4. 攻击力+5
    ATTACK_SPEED_UP,         // 5. 攻击速度+3
    HEAT_CAPACITY_UP,        // 6. 热量上限+25
    HEAT_COOL_DOWN_UP,       // 7. 热量冷却量+2
    UNLOCK_FIREBALL,         // 8. 解锁火球弹（原第8条，修正编号避免重复）
    UNLOCK_LIGHTNING,        // 9. 解锁闪电弹（原第9条）
    FIREBALL_RANGE_UP,       // 10. 火球范围+5
    LIGHTNING_MULTIPLIER_UP, // 11. 闪电倍率×1.5
    MOVE_SPEED_UP,           // 12. 神速：移动速度+1
    MAX_CARD_TYPES           // 占位符：标记卡型总数（12种）
};

class PerkCard {
public:
    PerkCardType type;
    char         name[30];
    uint8_t      param;

public:
    PerkCard(PerkCardType _type, const char _name[30], uint8_t _param) {

        if (_name != nullptr) {
            // 拷贝最多 sizeof(name)-1 字节，然后保证结尾终止
            strncpy(name, _name, sizeof(name) - 1);
            //name[sizeof(name) - 1] = '\0';
        } else {
            name[0] = '\0';
        }
        param = _param;


    }
};

constexpr struct PerkCardConfig {
    PerkCardType type;
    char         name[30];
    uint8_t      param; // 整数参数直接填，倍率参数×10填（如1.5→15）
    uint8_t      count; // 卡片数量（你的设计：大部分2张，解锁类1张）
} PERK_CARD_CONFIGS[12] = {
    {PerkCardType::HEAL_SPEED_UP,           "Heal Speed +2",     2,   2},
    {PerkCardType::HEAL_AMOUNT_UP,          "Heal Amount +5",    5,   2},
    {PerkCardType::HEALTH_UP,               "Health +100",       100, 2},
    {PerkCardType::ATTACK_UP,               "Attack +5",         5,   2},
    {PerkCardType::ATTACK_SPEED_UP,         "Attack Speed +3",   3,   2},
    {PerkCardType::HEAT_CAPACITY_UP,        "Heat Capacity +25", 25,  2},
    {PerkCardType::HEAT_COOL_DOWN_UP,       "Heat Cool Down +2", 2,   2},
    {PerkCardType::UNLOCK_FIREBALL,         "Unlock Fireball",   0,   1},
    {PerkCardType::UNLOCK_LIGHTNING,        "Unlock Lightning",  0,   1},
    {PerkCardType::FIREBALL_RANGE_UP,       "Fireball Range +5", 5,   2},
    {PerkCardType::LIGHTNING_MULTIPLIER_UP, "LightningDamage+1.5",    15,  2}, // 15=1.5×10
    {PerkCardType::MOVE_SPEED_UP,           "Move Speed +1",     1,   1},
};

#endif // PREKCARD_HPP
