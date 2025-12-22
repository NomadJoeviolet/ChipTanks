## 设计思路简介

### 游戏类型设计

**嵌入式平台上的“山海经”题材 Rougelite 弹幕射击游戏**（STG）

**横版/竖版射击 (Shoot 'em up / STG)**：
游戏的核心玩法是控制主角在 OLED 屏幕上移动，发射子弹消灭源源不断的敌人。

**Roguelite (轻度肉鸽)**：
**随机成长**：包含  `PerkCard`  系统和  `gamePerkCardManager`。玩家升级后会触发 Perk 选择（如“攻速提升”、“解锁火球”、“解锁闪电”），每次游玩的构建（Build）都是不同的。
**永久死亡/单局进程**：通常此类嵌入式游戏为单局挑战模式。

**热量系统 (Overheat System)**：不同于传统的无限射击，游戏引入了“热量”机制（`heatData`）。普通子弹、火球、闪电消耗不同的热量，过热可能导致无法射击。这要求玩家在“爆发输出”和“控温”之间做决策，增加了战术深度。

### 游戏背景设计

基于中国古代神话典籍《山海经》。所有的敌人都是古代神话中的妖兽，所有敌人的设计元素均来自于《山海经》。

通过将古老的神话生物与“坦克/射击”、“OLED 显示”、“热量过载”等科技元素结合形成反差。

- **敌人**：
- **小怪**：飞廉（风神）、蛊雕（食人雕）、魑魅（山林鬼怪）
- **BOSS**：上古四凶（饕餮、梼杌）和凶神（相柳）

## 游戏具体设计

### 机制设计

所有角色遵循以下基础物理与战斗公式：

#### 1. 基础战斗机制

- **射击冷却 (Cooldown):** `射击间隔(ms) = resetTime / Speed`。
- **热量机制 (Heat):**
  - **冷却速率:**  基类 `IRole::update()` 每 **500ms** 结算一次，减少 `heatCoolDownRate * 5` 点热量（等效每秒 `heatCoolDownRate * 10`）。
  - **热量消耗倍率（与 `heatPerShot` 基值相乘）:**
    - 普通子弹: **1.0x**
    - 火球弹: **2.0x**
    - 闪电链: **2.0x** （当前实现）
- **伤害计算:**
  - **普通子弹:** `Damage = attackPower`
  - **火球弹:** `Damage = attackPower + 10` (直接伤害 + 范围伤害)（造成两次）
  - **闪电链:** `Damage = (multiplier * attackPower) + 30` (穿透伤害
- **恢复计算**：`恢复间隔(ms) = resetTime / Speed , 恢复量 = healValue`

子弹设计逻辑，火球弹是大范围清杂子弹，闪电链是高伤害穿透子弹

#### 2.升级机制 (Level-Up Mechanism)

升级是玩家成长的基础驱动力。

**经验获取 (XP Gain)**

- **来源**: 击杀敌人。
- **数值**: 每个敌人都有定义的  `dropExp`（掉落经验值）。
  - 例如  `FeilianEnemy`  构造函数中传入  `dropExp`
- **过程**: 当敌人死亡（[die()](vscode-file://vscode-app/c:/IncludeAll/Tools/VSCode/Microsoft%20VS%20Code/resources/app/out/vs/code/electron-browser/workbench/workbench.html)）时，其  `dropExp`  会被加到玩家（`LeadingRole`）的当前经验池中

**升级判定**

- **阈值**: 玩家拥有  `currentExp`  和  `maxExp`（升级所需经验）
- **逻辑**: 当  `currentExp >= maxExp`  时，触发升级
- **成长曲线**: 升级后，下一级所需的  `maxExp`  通常会增加（例如：`maxExp = maxExp * 1.2`  或固定增量），增加升级难度

**升级后的基础提升 (Base Stat Growth)**
每次升级，玩家角色会自动获得一些基础属性的提升，无需选择：

- **生命值上限 (Max Health)**: 提升生存能力（例如  `maxHealth += 10`）。
- **回满血**: 升级通常伴随着状态恢复
- **攻击力/热量上限**: 可能伴随微量的线性成长（取决于  `LeadingRole`  的具体实现）
  **触发特殊事件**
- **Perk 选择**: 每隔特定等级（**每 2 级**），触发一次 PerkCard 选择
- **形态变化**: ，会改变角色的基础攻速

#### 3.PerkCard 机制 (天赋卡片系统)

PerkCard 是游戏的**Roguelike**核心元素，允许玩家在单局游戏中构建不同的战斗流派。
**卡片定义**
卡片包含以下属性：

- **Type (类型)**: 枚举值，决定卡片的功能。
- **Name (名称)**: 显示在 OLED 屏幕上的文本（如 "Attack +5"）。
- **Param (参数)**: 数值强度（如加 5 点攻击，参数就是 5）。
- **卡片类型与效果详解**

| 卡片类型                 | 效果描述         | 关联代码变量                        |
| :----------------------- | :--------------- | :---------------------------------- |
| **HEAL_SPEED_UP**        | 提升回血速度     | `healthData.healSpeed`              |
| **HEAL_AMOUNT_UP**       | 提升单次回复量   | `healthData.healValue`              |
| **HEALTH_UP**            | 提升生命上限     | `healthData.maxHealth`              |
| **ATTACK_UP**            | 提升基础攻击力   | `attackData.attackPower`            |
| **ATTACK_SPEED_UP**      | 提升射击频率     | `attackData.shootCooldownSpeed`     |
| **HEAT_CAPACITY_UP**     | 提升热量上限     | `heatData.maxHeat`                  |
| **HEAT_COOL_DOWN_UP**    | 提升散热速度     | `heatData.heatCoolDownRate`         |
| **UNLOCK_FIREBALL**      | **解锁新武器**   | 玩家解锁新的子弹                    |
| **UNLOCK_LIGHTNING**     | **解锁新武器**   | 玩家解锁新的子弹                    |
| **FIREBALL_RANGE_UP**    | 提升火球爆炸范围 | `attackData.bulletRange`            |
| **LIGHTNING_MULTIPLIER** | 提升闪电伤害倍率 | `attackData.bulletDamageMultiplier` |
| **MOVE_SPEED_UP**        | 提升移动速度     | `spatialData.moveSpeed`             |

##### 流派构建

通过升级和 Perk 的组合，玩家可以形成不同的策略：

1. **加特林流 (高攻速 + 高散热)**:
   - 重点选择  `ATTACK_SPEED_UP`  和  `HEAT_COOL_DOWN_UP`。
   - 结果：发射普通子弹像泼水一样，利用高频率触发伤害。
2. **重炮流 (火球 + 爆炸范围 + 高攻击)**:
   - 重点选择  `UNLOCK_FIREBALL`, `ATTACK_UP`, `FIREBALL_RANGE_UP`。
   - 结果：攻速慢，但一发火球能清空一片区域。
3. **雷神流 (闪电 + 倍率 + 高攻击 )**:
   - 重点选择  `UNLOCK_LIGHTNING`, `LIGHTNING_MULTIPLIER_UP`。
   - 结果：利用闪电的穿透特性和高倍率伤害，瞬间秒杀直线上的敌人。
4. **坦克流 (回血 + 高血量)**:
   - 重点选择  `HEALTH_UP`, `HEAL_SPEED_UP`。
   - 结果：依靠中的自动回血逻辑（`healTimeCounter`），硬抗敌人

### 敌人设计风格

#### 定位

**Fodder (炮灰/骚扰)**：飞廉（高机动低伤）、魑魅（自爆卡车），主要用于干扰走位和消耗玩家精力。

**Elite (精英/高威胁)**：蛊雕，拥有亡语机制（死后发火球），强迫玩家进行特定走位。

**Boss (首领)**：拥有多阶段或多技能循环。
_饕餮_：近战系 BOSS，强调位移控制（拉人、冲撞）。
_梼杌_：敏捷系 BOSS，强调反应速度（闪现、狂暴弹幕）。
_相柳_：法师/召唤系 BOSS，强调全屏躲避和处理小怪（AOE、召唤）。

#### 攻击机制与数值

##### 1. 飞廉 (FeilianEnemy)

- **定位**：小型高速骚扰者
- **原型**：风神，鹿身翼角。
- **数值设定**：
  - **体型**：12x12 (小)
  - **移动**：高速，轨迹飘忽不定（模拟风的特性）。
  - **攻击力**：低。
  - **死亡动画**：250ms。
- **攻击方式**：
  - **普通射击**：只发射普通子弹。
- **战术意义**：单体威胁小，依靠数量和难以预测的移动轨迹干扰玩家，消耗玩家的注意力。

##### 2. 蛊雕 (GudiaoEnemy)

- **定位**：中型伏击者
- **原型**：食人猛禽，似雕有角，音如婴儿。
- **数值设定**：
  - **体型**：15x15 (中)
  - **移动**：中速，飞行轨迹较直接。
  - **攻击力**：较高。
  - **死亡动画**：250ms。
- **攻击方式**：
  - **双侧射击**：每次从身体中心的两侧发射普通子弹（高伤害）。
  - **亡语（Death Rattle）**：死亡后会发射一颗火球弹，对击杀者造成最后威胁。
- **战术意义**：需要优先处理的高伤害单位，且击杀时需注意躲避亡语子弹。

##### 3. 魑魅 (ChiMeiEnemy)

- **定位**：自杀式冲锋怪
- **原型**：山林鬼怪，迷惑人心。
- **数值设定**：
  - **体型**：8x8 (极小)
  - **移动**：高速。
  - **死亡动画**：250ms。
- **攻击方式**：
  - **自杀冲撞**：不发射子弹，通过高速移动直接撞击玩家造成伤害。
- **战术意义**：迫使玩家保持移动，不能停留在原地。

##### 4. 饕餮 (TaotieEnemy)

- **定位**：近战坦克型 BOSS
- **原型**：四凶之一，贪婪，吞噬万物。
- **数值设定**：
  - **体型**：64x64 (巨大)
  - **HP**：高血量。
  - **攻击力**：高。
  - **移动**：低速。
  - **死亡动画**：500ms。
- **攻击方式 (5 种模式)**：
  1. **吞噬 (Devour)**：将玩家向自己拉近 (`pullDistance = 30`)，然后进行攻击。
  2. **弹幕**：发射三排普通子弹。
  3. **冲撞 (Ram)**：向前冲锋 (`chargeDistance = 30`) 进行撞击。
  4. **碾压 (Crush)**：从玩家左侧出现 (`crushChargeDistance = 100`)，向后碾压，封锁走位。
  5. **组合技**：将玩家拉近，同时向前冲锋 (`pullAndChargeDistance = 50`)。

##### 5. 梼杌 (TaowuEnemy)

- **定位**：高机动弹幕型 BOSS
- **原型**：四凶之一，顽固凶暴。
- **数值设定**：
  - **体型**：64x64 (巨大)
  - **HP**：低血量（相对其他 BOSS）。
  - **攻击力**：高。
  - **移动**：高速，擅长闪现 (Blink)。
  - **死亡动画**：500ms。
- **攻击方式 (6 种模式)**：
  1. **狂暴弹幕**：闪现至中间，随机发射大量普通子弹。**狂暴机制**：血量越低，持续时间越长（3 秒 -> 6 秒），频率 5 发/秒。
  2. **随机火球**：随机位置发射 5 个火球弹。
  3. **混合射击**：中间发射 1 颗火球，两侧边缘各发射 2 颗普通子弹。
  4. **缺口阵型**：发射一排子弹，仅中间留有缺口（迫使玩家钻缝）。
  5. **诡雷闪现**：原地留下一颗火球弹，本体消失。
  6. **追击闪现**：消除冷却 CD，直接闪现对齐玩家位置。

##### 6. 相柳 (XiangliuEnemy)

- **定位**：召唤与全屏压制型 BOSS
- **原型**：九头蛇怪，所过之处尽成泽国。
- **数值设定**：
  - **体型**：64x64 (巨大)
  - **HP**：高血量。
  - **攻击力**：高。
  - **移动**：中速。
  - **死亡动画**：500ms。
- **攻击方式 (6 种模式)**：
  1. **九头齐射**：发射  **9 排**  普通子弹（覆盖面极大）。
  2. **雷狱**：发射  **3 排**  闪电链（穿透伤害）。
  3. **火海**：发射  **3 排**  火球弹（范围爆炸）。
  4. **召唤魑魅**：生成 3 只自爆怪 (ChiMei) 协同作战。
  5. **召唤飞廉**：生成 2 只高速怪 (Feilian) 协同作战。
  6. **召唤蛊雕**：生成 1 只高伤怪 (Gudiao) 协同作战。

### 所有角色数值设计

#### 玩家 (LeadingRole) 数值公式

玩家的数值受基础属性、等级成长和 PerkCard（天赋卡）共同影响。

##### 1. 基础属性与成长

- **生命值 (HP):**
  - `MaxHealth` = 初始值 (180) + `Level`  成长 (每级+20) + `Perk: HEALTH_UP`
  - `CurrentHealth`: 升级时回满；受伤害减少；受回血机制增加。
- **攻击力 (Atk):**
  - `AttackPower` = 初始值 (15) + `Level`  成长 (每级+1) + `Perk: ATTACK_UP`
- **移动速度:**
  - `MoveSpeed` = 初始值 (1) + `Perk: MOVE_SPEED_UP`

#### 2. 战斗机制公式

- **射击冷却 (Cooldown):**
  - `冷却时间(ms)` = `shootCooldownResetTime` (4000) / shootCooldownSpeed
  - shootCooldownSpeed = 初始值 (4) + `Level`  成长 (每 5 级+2) + `Perk: ATTACK_SPEED_UP`
- **热量机制 (Heat):**
 - **热量机制 (Heat):**
  - `热量上限` = `maxHeat` (100) + `Level`  成长 (每级+5) + `Perk: HEAT_CAPACITY_UP`
  - `冷却速率` = **每 500ms** 结算一次，减少 `heatCoolDownRate * 5` 点热量；初始 `heatCoolDownRate = 4`，每 2 级 +1，Perk: HEAT_COOL_DOWN_UP 每张 +2。
  - `单发热量` = `heatPerShot` (初始 **15**，每 2 级 -1，最低不低于 1) × 枪械倍率（普通 1.0x / 火球 2.0x / 闪电 2.0x）。
- **回血机制 (Regen):**
  - `触发间隔` = `healResetTime` (15000) / `healSpeed` (3) = 5000ms
  - `单次回复量` = `healValue` (初始 5 + 每级+1 + `Perk: HEAL_AMOUNT_UP`)

#### 3. 玩家伤害输出公式 (createBullet)

- **普通子弹 (Basic):**  
   Damage=attackPowerDamage=attackPower
- **火球弹 (FireBall):**  
   Damage=attackPower+10Damage=attackPower+10
  _(注：火球造成一次直接伤害 + 范围伤害，两者数值相同)_
- **闪电链 (Lightning):**  
   Damage=(attackPower×multiplier)+30Damage=(attackPower×multiplier)+30
  _(注：`multiplier`  初始为 1.5，可通过 Perk 提升)_

#### 敌人 (Enemy) 数值公式

敌人的数值主要由其类型和当前游戏等级 (`Level`) 决定。

#### 1. 基础属性 (通用模板)

- **生命值 (HP):**  
   HP=BaseHP+(Level×HP_Growth)HP=BaseHP+(Level×HP_Growth)
- **攻击力 (Atk):**  
   Atk=BaseAtk+(Level×Atk_Growth)Atk=BaseAtk+(Level×Atk_Growth)
- **经验掉落 (XP):**  
   DropExp=BaseExp+(Level×Exp_Growth)DropExp=BaseExp+(Level×Exp_Growth)
- **热量冷却:** 基类同一逻辑，每 500ms 结算一次，减少 `heatCoolDownRate * 5` 点（敌人默认 `heatCoolDownRate = 10`，见 enemyRole.cpp）。

#### 2. 具体敌人数值 (基于 enemyRole.cpp)

| 敌人类型            | 生命值公式 (HP)           | 攻击力公式 (Atk)       | 碰撞伤害             | 备注                 |
| :------------------ | :------------------------ | :--------------------- | :------------------- | :------------------- |
| **飞廉 (Feilian)**  | 10 + Level×20             | 2 + Level×1            | 4 + Level×1          | 高速骚扰，低伤脆皮   |
| **蛊雕 (Gudiao)**   | 60 + Level×100            | 8 + Level×2            | 4 + Level×1          | 远程伏击，死后发火球 |
| **魑魅 (ChiMei)**   | 1 + Level×1               | 1 + Level×1            | 20（固定）           | 自爆冲撞             |
| **饕餮 (Taotie)**   | 350 + Level×1100          | 2 + Level×4            | 10 + Level×10        | 吞噬/冲撞/碾压       |
| **梼杌 (Taowu)**    | 130 + Level×800           | 10 + Level×5           | 7 + Level×5          | 闪现/弹幕爆发        |
| **相柳 (Xiangliu)** | 30 + Level×900            | 3 + Level×5            | 12 + Level×4         | 全屏 AOE/召唤        |

#### 3. 敌人伤害输出

- **普通射击:**  造成  `Atk`  点伤害。
- **碰撞伤害:**  等于其  `collisionPower`。
- **特殊技能:**  如火球或闪电，遵循与玩家相同的伤害增幅逻辑（例如火球通常比普攻高）。

#### 通用物理与状态公式

适用于所有角色（玩家与敌人）：

- **移动公式:**  
   NewPos=CurrentPos+(Direction×Speed)NewPos=CurrentPos+(Direction×Speed)
- **碰撞回退:** BackStep=BaseStep×ConsecutiveCollisionCountBackStep=BaseStep×ConsecutiveCollisionCount
  _(注：连续碰撞次数越多，弹开距离越远，防止卡死)_
- **受伤公式:** CurrentHealth=max⁡(0,CurrentHealth−Damage)CurrentHealth=max(0,CurrentHealth−Damage)
