#ifndef PREKCARD_HPP
#define PREKCARD_HPP

#include "stdint.h"

enum class PerkCardType {
    RESPONSE_SPEED_UP,    // 1. 回复速度+2
    RESPONSE_AMOUNT_UP,   // 2. 回复量+5
    HEALTH_UP,            // 3. 生命值+100
    ATTACK_UP,            // 4. 攻击力+5
    ATTACK_SPEED_UP,      // 5. 攻击速度+3
    HEAT_CAPACITY_UP,     // 6. 热量上限+25
    HEAT_COOL_DOWN_UP,    // 7. 热量冷却量+2
    UNLOCK_FIREBALL,      // 8. 解锁火球弹（原第8条，修正编号避免重复）
    UNLOCK_LIGHTNING,     // 9. 解锁闪电弹（原第9条）
    FIREBALL_RANGE_UP,    // 10. 火球范围+5
    LIGHTNING_MULTIPLIER_UP, // 11. 闪电倍率×1.5
    SPEED_BOOST,          // 12. 神速：移动速度+1
    MAX_CARD_TYPES        // 占位符：标记卡型总数（12种）
};

class PerkCard{
public:
    PerkCardType type;
    char  name[30] ;
    uint8_t     param ;
public :
    PerkCard(PerkCardType _type , const char* _name , uint8_t _param )
    {
        type = _type;
        strncpy(name, _name, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
        param = _param;
    }
};

// 卡片配置表：统一管理“类型+名称+描述+参数+数量+是否消耗”（你的设计完全对齐）
constexpr struct PerkCardConfig {
    PerkCardType type;
    const char* name;
    const char* desc;
    uint8_t param;       // 整数参数直接填，倍率参数×10填（如1.5→15）
    uint8_t count;       // 卡片数量（你的设计：大部分2张，解锁类1张）
} PERK_CARD_CONFIGS[] = {
    {PerkCardType::RESPONSE_SPEED_UP,    "Response Speed +2",    "回复速度永久+2",                2,  2},
    {PerkCardType::RESPONSE_AMOUNT_UP,   "Response Amount +5",   "每次回复量+5",                  5,  2},
    {PerkCardType::HEALTH_UP,            "Health +100",          "生命值永久+100（不超上限）",    100,2,},
    {PerkCardType::ATTACK_UP,            "Attack +5",            "攻击力永久+5",                  5,  2},
    {PerkCardType::ATTACK_SPEED_UP,      "Attack Speed +3",      "攻击速度永久+3",                3,  2},
    {PerkCardType::HEAT_CAPACITY_UP,     "Heat Capacity +25",    "热量上限+25",                  25, 2},
    {PerkCardType::HEAT_COOL_DOWN_UP,    "Heat Cool Down +2",    "每秒热量冷却+2",                2,  2},
    {PerkCardType::UNLOCK_FIREBALL,      "Unlock Fireball",      "解锁火球弹（范围伤害）",        0,  1},
    {PerkCardType::UNLOCK_LIGHTNING,     "Unlock Lightning",     "解锁闪电弹（直线伤害）",        0,  1},
    {PerkCardType::FIREBALL_RANGE_UP,    "Fireball Range +5",    "火球弹范围+5",                  5,  2},
    {PerkCardType::LIGHTNING_MULTIPLIER_UP, "Lightning ×1.5", "闪电弹伤害×1.5",             15, 2}, // 15=1.5×10
    {PerkCardType::SPEED_BOOST,          "Speed Boost +1",       "神速：移动速度永久+1",          1,  1},
};


#endif // PREKCARD_HPP
