#ifndef GAMEPERKCARDMANAGER_HPP
#define GAMEPERKCARDMANAGER_HPP
#include "prekCard.hpp"
#include "etl/vector.h"

class GamePerkCardManager {
public:
    GamePerkCardManager()  = default;
    ~GamePerkCardManager() = default;

    GamePerkCardManager(const GamePerkCardManager &)            = delete;
    GamePerkCardManager &operator=(const GamePerkCardManager &) = delete;

private:
    static const uint8_t MAX_CARD_COUNT = 22; // 总卡片数（2*10 +1*2=22）
    static const uint8_t SELECTION_SLOT_COUNT = 3; // 选卡槽数量（3张）

    etl::vector<PerkCard , MAX_CARD_COUNT> m_cardWarehouse; // 卡片仓库（存所有卡片）
    etl::vector<PerkCard , SELECTION_SLOT_COUNT> m_selectionSlots; // 选卡槽（临时存3张待选）
    bool m_isSelecting = false; // 选卡状态（标记是否处于“选卡中”，避免重复触发）

public:

    // 初始化卡片仓库
    void initWarehouse() {

    m_isSelecting = false;

    // 遍历配置表，按数量创建卡片并加入仓库
    
}
};

#endif // GAMEPERKCARDMANAGER_HPP
