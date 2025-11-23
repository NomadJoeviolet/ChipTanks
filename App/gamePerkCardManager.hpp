#ifndef GAMEPERKCARDMANAGER_HPP
#define GAMEPERKCARDMANAGER_HPP
#include "etl/vector.h"

#include "oled.h"
#include "prekCard.hpp"
#include "gameEntityManager.hpp"
#include "leadingRole.hpp"

// 管理游戏中的Perk卡片系统
// 引用GameEntityManager实例
// 归gameProgressManager管理

extern GameEntityManager g_entityManager;

class GamePerkCardManager {
public:
    GamePerkCardManager()  = default;
    ~GamePerkCardManager() = default;

    GamePerkCardManager(const GamePerkCardManager &)            = delete;
    GamePerkCardManager &operator=(const GamePerkCardManager &) = delete;

private:
    static const uint8_t MAX_CARD_COUNT       = 22; // 总卡片数（2*10 +1*2=22）
    static const uint8_t SELECTION_SLOT_COUNT = 3;  // 选卡槽数量（3张）

    etl::vector<PerkCard, MAX_CARD_COUNT>       m_cardWarehouse;       // 卡片仓库（存所有卡片）
    etl::vector<PerkCard, SELECTION_SLOT_COUNT> m_selectionSlots;      // 选卡槽（临时存3张待选）


public:
    bool isInited = false;               // 是否已初始化
    bool                                        m_isSelecting = false; // 选卡状态（标记是否处于“选卡中”，避免重复触发）
    uint8_t                                    m_selectedIndex = 0;   // 选中卡片索引（选卡槽内索引）
    uint8_t                                   m_selectedSize = 0;    // 选中卡片数量（选卡槽内数量）

public:
    // 初始化卡片仓库
    void initWarehouse() {
        m_cardWarehouse.clear();
        m_selectionSlots.clear();
        m_isSelecting = false;
        isInited = true;

        // 遍历配置表，按数量创建卡片并加入仓库
        for (const auto &config : PERK_CARD_CONFIGS) {
            for (uint8_t i = 0; i < config.count; ++i) {
                PerkCard newCard(config.type, config.name, config.param);
                m_cardWarehouse.push_back(newCard);
            }
        }
    }

    bool triggerPerkSelection() {
        if (m_isSelecting || m_cardWarehouse.empty()) {
            return false; // 已在选卡中或无卡片可选，拒绝触发
        }

        m_selectionSlots.clear();
        etl::vector<uint8_t, MAX_CARD_COUNT> availableIndices; // 仓库中可用卡片的索引
        availableIndices.clear();

        for (size_t i = 0; i < m_cardWarehouse.size(); ++i) {
            availableIndices.push_back(static_cast<uint8_t>(i));
        }

        // 可用卡片不足3张时，取全部（避免选卡槽为空）
        uint8_t selectCount = etl::min(SELECTION_SLOT_COUNT, (uint8_t)availableIndices.size());
        if (selectCount == 0) {
            return false; // 无可用卡片
        }
        m_selectedSize = selectCount;
        // 随机抽取selectCount张卡片，放入选卡槽（从仓库移除，避免重复抽）
        for (uint8_t i = 0; i < selectCount; ++i) {
            // 嵌入式随机算法：用系统滴答定时器做模运算（无需rand()）
            uint8_t randIdx      = rand() % availableIndices.size();
            uint8_t warehouseIdx = availableIndices[randIdx];

            // 从仓库移动到选卡槽（避免拷贝，提升效率）
            m_selectionSlots.push_back(m_cardWarehouse[warehouseIdx]);
            m_cardWarehouse.erase(m_cardWarehouse.begin() + warehouseIdx);

            // 从可用索引中移除，避免重复抽取
            availableIndices.erase(availableIndices.begin() + randIdx);
        }
        // 标记为选卡中，UI提示
        m_isSelecting = true;

        return true;
    }

    bool selectCard(uint8_t slotIndex) {
        // 前置校验：1. 在选卡中；2. 槽索引有效
        if (!m_isSelecting || slotIndex >= m_selectionSlots.size()) {
            return false;
        }

        // 处理选中卡片（slotIndex）
        PerkCard selectedCard = m_selectionSlots[slotIndex];
        LeadingRole* player   = (LeadingRole*)g_entityManager.getPlayerRole();
        
        if(player == nullptr) {
            return false; // 未找到玩家角色，无法应用卡片效果
        }

        // 应用卡片效果
        switch (selectedCard.type) {
            case PerkCardType::HEAL_SPEED_UP:
                player->getData()->healthData.healSpeed += selectedCard.param;
                break;
            case PerkCardType::HEAL_AMOUNT_UP:
                player->getData()->healthData.healValue += selectedCard.param;
                break;
            case PerkCardType::HEALTH_UP:
                player->getData()->healthData.maxHealth += selectedCard.param;
                player->getData()->healthData.currentHealth = player->getData()->healthData.maxHealth;
                break;
            case PerkCardType::ATTACK_UP:
                player->getData()->attackData.attackPower += selectedCard.param;
                break;
            case PerkCardType::ATTACK_SPEED_UP:
                player->getData()->attackData.shootCooldownSpeed += selectedCard.param;
                break;
            case PerkCardType::HEAT_CAPACITY_UP:
                player->getData()->heatData.maxHeat += selectedCard.param;
                break;
            case PerkCardType::HEAT_COOL_DOWN_UP:
                player->getData()->heatData.heatCoolDownRate += selectedCard.param;
                break;
            case PerkCardType::UNLOCK_FIREBALL:
                player->bulletTypeOwned.fireBallBulletOwed = 1;
                break;
            case PerkCardType::UNLOCK_LIGHTNING:
                player->bulletTypeOwned.lightningLineBulletOwed = 1;
                break;
            case PerkCardType::FIREBALL_RANGE_UP:
                player->getData()->attackData.bulletRange += selectedCard.param;
                break;
            case PerkCardType::LIGHTNING_MULTIPLIER_UP:
                player->getData()->attackData.bulletDamageMultiplier += (static_cast<float>(selectedCard.param) / 10.0f);
                break;
            case PerkCardType::MOVE_SPEED_UP:
                player->getData()->spatialData.moveSpeed += selectedCard.param;
                break;
            default:
                break;
        }

        // 移除选中卡片，清理未选中卡片，重置选卡状态
        m_selectionSlots.erase(m_selectionSlots.begin() + slotIndex);
        returnUnselectedCards();

        m_isSelecting = false;
        return true;
    }  

    // 辅助接口：未选中的卡片回库
    void returnUnselectedCards() {
        for (uint8_t i = 0; i < m_selectionSlots.size(); ++i) {
            // 跳过选中的卡片（slotIndex已处理）
            m_cardWarehouse.push_back(m_selectionSlots[i]);
        }
        m_selectionSlots.clear();
    }



    void drawSelectionUI() {
        if (!m_isSelecting) {
            return; // 非选卡状态，无需绘制
        }
        // 绘制选卡槽UI（简化示例，实际根据UI框架实现）
        for (uint8_t i = 0; i < m_selectionSlots.size(); ++i) {
            OLED_PrintString( 10 , 10+(i*20) , m_selectionSlots[i].name , &font8x6 , OLED_COLOR_NORMAL );
        }

        OLED_DrawCircle( 5 , 12 + (m_selectedIndex * 20) , 2 , OLED_COLOR_NORMAL ); // 绘制选中指示器
    }

};

#endif // GAMEPERKCARDMANAGER_HPP
