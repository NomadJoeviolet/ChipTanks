## 前言
非常适合集成各类库后去雕琢，练习 C&C++混编和设计模式，C++动态内存分配等，很爽的练手项目

仓库：[NomadJoeviolet/ChipTanks: OLED屏幕的Rouglike弹幕射击游戏](https://github.com/NomadJoeviolet/ChipTanks)

项目代码状况
```txt
Top-level groups:
Bullet          Raw=   378 NonEmpty=   304 Code=   266
Data            Raw=   301 NonEmpty=   252 Code=   186
gameEntityManager.hpp Raw=   412 NonEmpty=   362 Code=   300
gamePerkCardManager.hpp Raw=   171 NonEmpty=   145 Code=   128
gameProgressManager.hpp Raw=   410 NonEmpty=   359 Code=   307
Peripheral      Raw=  1624 NonEmpty=  1495 Code=   905
PerkCard        Raw=    67 NonEmpty=    56 Code=    54
reload_new_delete.cpp Raw=    61 NonEmpty=    52 Code=    33
Role            Raw=  3314 NonEmpty=  2865 Code=  2349
threads.cpp     Raw=   259 NonEmpty=   219 Code=   140

Totals: RawLines=6997  NonEmptyLines=6109  CodeLines=4668
```

## 游戏设计
玩法介绍：[ChipTanks-玩法介绍](https://nomadjoeviolet.github.io/p/chiptanks-%E7%8E%A9%E6%B3%95%E4%BB%8B%E7%BB%8D/)

游戏设计：[ChipTanks-游戏设计](https://nomadjoeviolet.github.io/p/chiptanks-%E6%B8%B8%E6%88%8F%E8%AE%BE%E8%AE%A1/)

## 硬件选型
使用的OLED屏的显示驱动为SSD1306
主控板为stm32f103c8t6

## 开发工具链
使用 VSCode+Intellisence+CubeMX+CMake+Ninja+arm-none-eabi+Ozone+Jlink的高自定义开发工具链

代码编辑器采用VSCode，语言服务器使用VSCode的Intellisence来实现智能代码补全和理解功能，用CubeMX生成硬件抽象层代码，CMake和Ninja作为项目构建工具，使用arm-none-eabi编译器，烧录和调试使用Ozone，硬件烧录器使用Jlink

CubeMX使用的版本为`6.15.0`
CMake使用的版本为`4.1.0`
编译器arm-none-eabi使用的版本为`14.3 rel1`

开发方式采用C&C++混编，使用`exter "C" { }`解决C++名词修饰导致C无法链接的问题，`extern "C"`的作用就是告诉C++编译器修改符号表生成方式，将C++符号的生成方式换成了的生成方式。

实践上将FreeRTOS的线程函数入口声明 定义为`As external`

![](ChipTanks-基于C&C++混编的STM32OLED迷你游戏掌机个人项目开发-keyScan线程.png)

然后在`cpp`文件中去实现，实现的函数用`extern "C"`包裹即可，这样`void keyScanThread(void *argument)`经过C++编译器编译后的符号就可以被C链接

```cpp

#ifdef __cplusplus
extern "C" {
#endif

void keyScanThread(void *argument) {
	for(){
		//...
		osDelay(10);
	}
}

#ifdef __cplusplus
}
#endif

```

## 第三方库引入
引入FreeRTOS和dsp库以及etl库

引入FreeRTOS用来处理多线程任务，引入dsp库用来加速部分运算（主要想用来加速 游戏实体 碰撞检测）（实际基本上没用，当做练手了），引入etl库来做到 零开销的使用 C++ 容器 提高开发效率，其确定性内存管理 (Deterministic Memory)在**栈上**或**静态区**分配固定大小的内存，而不是在堆上动态扩容，能完全避免了内存碎片问题，且内存占用在编译期就是已知的，非常适合 RAM 有限的 STM32，同时有容器去实现设计模式等会更加高效。

## 代码实现
### 动态内存分配在嵌入式平台上的解决
因嵌入式平台的内存资源非常有限，同时还要保证FreeRTOS的线程安全，需要对C++的动态内存分配（`new`和`delete`）进行重载，重载为`pvPortMalloc(size)`和`vPortFree(ptr)`，FreeRTOS线程安全的分配方式，从FreeRTOS的堆里面分配


```cpp
#include "FreeRTOS.h"
#include "task.h" // pvPortMalloc and vPortFree are defined here
#include <stddef.h> // Use C header for size_t in embedded environments
void* operator new(size_t size) {
    void* ptr = pvPortMalloc(size);
    return ptr;
}
void operator delete(void* ptr) noexcept {
    if (ptr) {
        vPortFree(ptr);
    }
}
void* operator new[](size_t size) {
    void* ptr = pvPortMalloc(size);
    return ptr;
}
void operator delete[](void* ptr) noexcept {
    if (ptr) {
        vPortFree(ptr);
    }
}

void operator delete(void* ptr, size_t size) noexcept {
    (void)size; // The size parameter is often unused in custom deallocations.
    if (ptr) {
        vPortFree(ptr);
    }
}
void operator delete[](void* ptr, size_t size) noexcept {
    (void)size;
    if (ptr) {
        vPortFree(ptr);
    }
}
```


这样重载后，动态分配的内存不会从 系统堆里面分配，而是从FreeRTOS的 **静态 堆**（内存池） （此处静态指 编译期最大容量确定）里面分配，从而避免可能的堆栈溢出，FreeRTOS的堆栈管理方案采用 安全 且 有高效内存碎片化处理 的heap4 即可

![](ChipTanks-基于C&C++混编的STM32OLED迷你游戏掌机个人项目开发-FreeRTOS堆栈管理.png)


**为什么不使用系统堆？**
1. 因为在嵌入式实时操作系统中，调用C标准库中 malloc() 和 free() 是危险的，C++中的 new 和 delete 也一样：
2.  这些函数在小型嵌入式系统中并不总是可用的，小型嵌入式设备中的RAM不足。
3.  它们的实现可能非常的大，占据了相当大的一块代码空间。
 4. 他们几乎都不是线程安全的。
 5. 它们并不是确定的，每次调用这些函数执行的时间可能都不一样。
 6. 它们有可能产生内存碎片。
 7. 这两个函数会使得链接器配置得复杂。

系统内存分配

![](ChipTanks-基于C&C++混编的STM32OLED迷你游戏掌机个人项目开发-系统RAM内存分配.png)
![](ChipTanks-基于C&C++混编的STM32OLED迷你游戏掌机个人项目开发-FreeRTOS的内存布局.png)

由此可以直接看出重载后动态内存分配可以保证系统栈和线程的安全

### 项目文件结构
#### 整体结构
```txt
ChipTanks/
├── App/                        // 游戏应用逻辑层
│   ├── Bullet/                 // 子弹模块
│   ├── Data/                   // 基础数据定义
│   ├── Peripheral/             // 外设驱动（OLED, KEY）
│   ├── PerkCard/               // 天赋卡片模块
│   ├── Role/                   // 角色模块（玩家与敌人）
│   ├── gameEntityManager.hpp   // 实体管理器
│   ├── gamePerkCardManager.hpp // Perk管理器
│   ├── gameProgressManager.hpp // 进度管理器
│   ├── reload_new_delete.cpp   // 内存管理重载
│   └── threads.cpp             // FreeRTOS任务定义
├── ChipTanksPic/               // 项目图片资源（可能为空或存放设计图）
├── Core/                       // STM32CubeMX 生成的核心代码
│   ├── Inc/                    // 核心头文件 (main.h, FreeRTOSConfig.h 等)
│   └── Src/                    // 核心源文件 (main.c, freertos.c, stm32f1xx_it.c 等)
├── Drivers/                    // 硬件驱动库
│   ├── CMSIS/                  // ARM Cortex 微控制器软件接口标准
│   │   ├── Core/               // 核心外设访问层
│   │   ├── DSP/                // 数字信号处理库 (DSP Lib)
│   │   └── ...
│   └── STM32F1xx_HAL_Driver/   // STM32F1系列 HAL 库驱动
├── Lib/                        // 第三方库
│   └── etl/                    // Embedded Template Library (嵌入式模板库)
├── Middlewares/                // 中间件
│   └── Third_Party/
│       └── FreeRTOS/           // FreeRTOS 实时操作系统源码
├── build/                      // 编译输出目录 (Debug/Release)
├── cmake/                      // CMake 构建脚本与工具链配置
├── CMakeLists.txt              // 项目根 CMake 配置文件
└── STM32F103C8Tx_FLASH.ld      // 链接脚本
```

#### App结构
```txt
ChipTanks/App
├── gameEntityManager.hpp       // 游戏实体管理器（管理所有角色和子弹）
├── gamePerkCardManager.hpp     // Perk卡片管理器（管理升级抽卡逻辑）
├── gameProgressManager.hpp     // 游戏进度管理器（管理关卡流程）
├── reload_new_delete.cpp       // 重载C++内存分配（适配FreeRTOS）
├── threads.cpp                 // FreeRTOS任务定义与调度
├── Bullet/                     // 子弹模块
│   ├── bullet.cpp              // 子弹逻辑实现
│   └── bullet.hpp              // 子弹基类与派生类定义
├── Data/                       // 数据模块
│   └── basicData.hpp           // 基础数据结构与枚举定义
├── Peripheral/                 // 外设驱动模块
│   ├── KEY/
│   │   ├── key.cpp             // 按键扫描实现
│   │   └── key.hpp             // 按键接口定义
│   └── OLED/
│       ├── font.c              // 字库数据
│       ├── font.h              // 字库声明
│       ├── oled.c              // OLED驱动实现
│       └── oled.h              // OLED接口定义
├── PerkCard/                   // 天赋卡片模块
│   └── prekCard.hpp            // Perk卡片类定义
└── Role/                       // 角色模块
    ├── enemyRole.cpp           // 敌人逻辑实现
    ├── enemyRole.hpp           // 敌人派生类定义
    ├── leadingrole.cpp         // 主角逻辑实现
    ├── leadingRole.hpp         // 主角派生类定义
    ├── role.cpp                // 角色基类逻辑实现
    └── role.hpp                // 角色基类定义
```
### 架构与设计思路
主要采用 **面向对象（OOP）** 的设计思路，结合FreeRTOS实时操作系统，构建简单的嵌入式游戏框架。

主要分为5各层级：
1. 硬件抽象层：基于HAL库的对底层硬件的抽象，对外设硬件操作进行再次封装
2. 核心数据层（Data）：定义游戏通用的数据结构和枚举
3. 游戏实体层（Entity/Role）：定义角色(Role)基类和子弹(Bullet)基类 及其派生类
4. 管理器层（Manager）：负责实体生命周期管理，碰撞检测，进度控制和PerkSelection系统
5. 应用逻辑层（App Logic）：多线程任务处理，游戏业务逻辑

#### 硬件抽象层
在HAL库基础上，对外设进行面向对象的进一步封装

 **OLED**(使用的keysking的开源库):
     提供OLED_DrawImage，OLED_PrintString 等绘图 API。
     包含字库 (`font.c`)，支持文本显示。
     作为游戏的**输出设备**，负责将游戏状态可视化。
 **KEY**:
     提供按键扫描逻辑。
     作为游戏的**输入设备**，将物理按键映射为游戏内的控制信号

#### 核心数据层
 定义了所有游戏实体共用的数据结构，如 `HealthData`（血量）、`AttackData`（攻击）、`SpatialData`（位置与大小）、`HeatData`（热量）等。
 使用 `struct` 组合数据，便于在不同类之间传递和管理状态。
 定义了 `RoleIdentity`（身份）、（子弹类型）等关键枚举。
#### 游戏实体层
这是游戏逻辑的核心部分，采用了**继承与多态**的设计。
 ##### **基类设计**:
**`IRole` （role.hpp**）: 所有角色（玩家和敌人）的基类。包含 `RoleData` 指针，管理角色的所有属性。
定义了虚函数接口：`init()`, `think()`, `doAction()`, `drawRole()`, `die()`, `shoot()`。
实现了通用的物理逻辑：`move()`（移动与边界检查）、`update()`（状态更新）、`createBullet()`。

**`IBullet` (bullet.hpp)**: 所有子弹的基类。
定义了 `move()`, `update()`, `drawBullet()`, `die()` 等接口。
 ##### **派生类设计**:
**`LeadingRole` (`leadingRole.cpp`)**: 玩家控制的主角。
实现了复杂的升级逻辑 (`levelUp`)、热量控制和多武器切换。
单例模式或全局对象管理（通常作为唯一主角存在）。

**`EnemyRole` (`enemyRole.hpp`)**: 各种敌人的实现。
**`FeilianEnemy`**: 高速骚扰型，只发普通弹。
**`TaotieEnemy`** (BOSS): 状态机控制的复杂行为（吞噬、冲撞）。
**`XiangliuEnemy`** (BOSS): 召唤与全屏弹幕。

**`BasicBullet` / `FireBallBullet` / `LightningLineBullet`**: 不同类型的子弹实现，具有不同的伤害逻辑和特效。

#### 管理器层

##### 1.**`GameEntityManager` (`gameEntityManager.hpp`)**:
 核心职责:
	  维护所有活跃的角色和子弹容器，使用固定容量的嵌入式向量。包含角色池（最多20个）和子弹池（最多100个），以及游戏结束标记。提供注册新增实体、分阶段的行为与状态更新、碰撞与范围伤害检测、统一渲染、失效实体清理、获取主角指针和经验投递等功能。行为与状态分离，更新顺序通常为：角色行为、子弹行为、角色状态、子弹状态、清理、渲染。碰撞使用轴对齐包围盒，子弹碰撞允许穿透；范围伤害使用圆形距离判定。所有容器操作使用临界区保护以确保一致性。
```cpp
	etl::vector<IRole *, 20>    m_roles;   //角色池，存放所有角色指针
    etl::vector<IBullet *, 100> m_bullets; //子弹池，存放所有子弹指针
```


功能:
1. `addRole()`, `addBullet()`: 注册新实体。
2. `updateAllRolesActions()`: 每帧调用所有角色的 `doAction()`。
3. `updateAllRolesState()`：每帧调用所有角色的 `update()`。 
4. `checkxxxxCollision()`: 进行碰撞检测（AABB 碰撞盒），处理伤害结算。
5. `drawAllRoles()`和`drawAllBullets`: 统一渲染所有实体。
6. `cleanUpInvalidRoles()`和`cleanupInvalidBullets()`: 回收非活跃实体的内存，处理主角经验值获取。
7. `getPlayerRole()`：获取主角的指针


##### 2.`GamePerkCardManager` (`gamePerkCardManager.hpp`)
核心职责：
	管理 Rogue-like 的随机强化流程，包括卡片仓库管理、抽取待选卡、选择应用以及未选卡回库。使用两个固定容量的嵌入式向量存储卡片仓库与选卡槽，并维护选卡状态、选中索引与数量。初始化时按配置表生成卡片；触发选卡会随机抽取最多三张并进入选卡状态；选择后将效果应用到玩家（如提高生命、攻击、攻速、移动速度、散热效率，或解锁火球与闪电，或提升范围与伤害倍率），随后将未选卡退回仓库并退出选卡状态。选卡界面显示卡名并有选中指示器。
    
```cpp
etl::vector<PerkCard, MAX_CARD_COUNT>       m_cardWarehouse;  // 卡片仓库（存所有卡片）
etl::vector<PerkCard, SELECTION_SLOT_COUNT> m_selectionSlots; // 选卡槽（临时存3张待选）
```

 功能
1. `initWarehouse()`：初始化卡片仓库
2. `triggerPerkSelection()`：触发`PerkSelection`
3. `selectCard`：选择卡片，同时将卡片效果应用
4. `drawSelectionUI`：绘制选卡界面的UI


##### 3.`GameProgressManager` (`gameProgressManager.hpp`)
核心职责：
	控制游戏整体流程，包括关卡推进、波次刷新、Boss 触发与展示、胜利/失败判定，以及开场/通关/警告等过场的展示与计时。与实体管理器和卡片管理器协作，驱动每一波敌人生成与选卡节点的触发。
功能
1. `resetGameProgress()`：重置整局流程，创建玩家实体并加入实体管理器，初始化卡片管理器仓库
2. `updateGameProgress()`：推进波次/关卡与选卡触发，特定条件调用 `AddWaveEnemies()` 生成新的一波敌人
3. `AddWaveEnemies()`：根据当前关卡与波次生成敌人阵型或 Boss
4. `drawOpeningCG()`：绘制开场过场动画
5. `drawClearCG()`：绘制通关动画
6. `drawShowBoss()`：绘制 Boss 海报或挑战警告
#### 应用逻辑层
`threads.cpp`: 有三个核心任务线程来驱动整体逻辑

 `oledTaskThread`: 负责渲染时序。根据状态（开场、通关、Boss 展示、选卡、正常战斗）选择对应绘制路径；最终输出帧并控制刷新频率。
 `keyScanThread`: 负责输入采集与分发。非选卡状态下更新玩家动作（上下左右）；选卡状态下移动 UI 光标并执行选中操作；根据游戏/过场状态决定是否响应按键。
 `gameControlThread`: 负责主控流程与核心时序。Game Over 时重置游戏；正常状态下在非选卡与非过场时推进进度、执行实体行为与状态更新、清理无效实体。

### 各层级和功能的代码实现与解析

#### 硬件抽象层 
##### KEY

当前按键采用“左右各 4 独立键，低电平按下”方案，不再是 4x4 矩阵。CubeMX 已将引脚配置为上拉输入，扫描直接读电平即可。左右各有独立状态缓存，方向枚举在上层做映射。

```cpp
#ifndef KEY_HPP
#define KEY_HPP

#include "gpio.h"

typedef struct {
    GPIO_TypeDef* GPIOx;
    uint16_t GPIO_Pin;
} KeyGPIO;

enum class LeftKeyState {
    KEY_DOWN = 0,
    KEY_UP   = 1,
    KEY_RIGHT= 2,
    KEY_LEFT = 3
};

enum class RightKeyState {
    KEY_DOWN = 0,
    KEY_LEFT = 1,
    KEY_RIGHT= 2,
    KEY_UP   = 3
};

class Key {
public:
    uint8_t m_leftKeyButton[4]  = {0}; // 左手4键，低电平按下
    uint8_t m_rightKeyButton[4] = {0}; // 右手4键，低电平按下
    KeyGPIO* m_rightInput = nullptr;
    KeyGPIO* m_leftInput  = nullptr;
public:
    Key(KeyGPIO* rightInput, KeyGPIO* leftInput)
        : m_rightInput(rightInput), m_leftInput(leftInput) {}
    ~Key() {};

    void init() {
        // 引脚已在 MX_GPIO_Init 配为上拉输入，无需重复配置
    }

    void scan() {
        for (uint8_t i = 0; i < 4; i++) {
            m_rightKeyButton[i] = (HAL_GPIO_ReadPin(m_rightInput[i].GPIOx, m_rightInput[i].GPIO_Pin) == GPIO_PIN_RESET);
        }
        for (uint8_t i = 0; i < 4; i++) {
            m_leftKeyButton[i]  = (HAL_GPIO_ReadPin(m_leftInput[i].GPIOx,  m_leftInput[i].GPIO_Pin)  == GPIO_PIN_RESET);
        }
    }
};

extern Key key;
#endif // KEY_HPP
```

```cpp
#include "key.hpp"

KeyGPIO keyRightOutput[4] = {
    {KEY_R1_GPIO_Port, KEY_R1_Pin},
    {KEY_R2_GPIO_Port, KEY_R2_Pin},
    {KEY_R3_GPIO_Port, KEY_R3_Pin},
    {KEY_R4_GPIO_Port, KEY_R4_Pin}
};

KeyGPIO keyLeftInput[4] = {
    {KEY_C1_GPIO_Port, KEY_C1_Pin},
    {KEY_C2_GPIO_Port, KEY_C2_Pin},
    {KEY_C3_GPIO_Port, KEY_C3_Pin},
    {KEY_C4_GPIO_Port, KEY_C4_Pin}
};

Key key(keyRightOutput, keyLeftInput);

```

##### OLED
使用的是keysking的开源，该代码中提供OLED的驱动和绘画渲染的API，直接使用即可

#### 核心数据层
**概览**
 定义了游戏所需的基础数据类型：身份、子弹类型、动作/移动/攻击/死亡/空间等数据结构，以及角色与子弹的数据聚合体。
 “纯数据载体”，不含行为方法；行为由上层实体类与管理器驱动。
 时间与数值单位采用“毫秒 + 小整型”为主，满足嵌入式 MCU 资源约束。
**枚举类型**
 `RoleIdentity`: 标识阵营与身份（UNKNOWN/ENEMY/Player）。用于伤害互斥、经验结算等。
 ```cpp
  enum class RoleIdentity {
    UNKNOWN = 0,
    ENEMY   = 1,
    Player  = 2,
};
  ```
 
 `BulletType`: 子弹种类（BASIC/FIRE_BALL/LIGHTNING_LINE）。影响碰
 撞、穿越、范围与倍率等期望行为。
```cpp
enum class BulletType {
    BASIC          = 0,
    FIRE_BALL      = 1,
    LIGHTNING_LINE = 2,
};
```

 `ActionState`: 动作态（IDLE/MOVING/ATTACKING）。由输入或 AI 控制，驱动移动/攻击决策。
```cpp
enum class ActionState { IDLE = 0, MOVING = 1, ATTACKING = 2 };
```
 
 `AttackMode`: 预留的攻击形态枚举（MODE_1…MODE_6），可用于多段或多式攻击切换。
 ```cpp
 enum class AttackMode { NONE = 0, MODE_1 = 1, MODE_2 = 2, MODE_3 = 3 , MODE_4 = 4 , MODE_5 = 5 , MODE_6 = 6 };
 ```
 
 `MoveMode`: 移动方向（NONE/UP/DOWN/LEFT/RIGHT）。
 `CollisionDirection`: 碰撞方向（NONE/LEFT/RIGHT/UP/DOWN），供状态响应/回退使用。
```cpp
enum class MoveMode { NONE = 0, UP = 1, DOWN = 2, LEFT = 3, RIGHT = 4 };
```

**HeatData（热量数据）**
 变量：`currentHeat`，`maxHeat`，`heatCoolDownRate`，`heatCoolDownTimer`，`heatPerShot`。
 构造：可用构造函数设置“初值、上限、冷却速率、每枪增量”。默认不显式初始化 `heatCoolDownTimer`（保持 0 即可）。
 用法：开火前检查 `currentHeat + heatPerShot <= maxHeat`；每帧/每周期按 `heatCoolDownRate` 降温（受 `heatCoolDownTimer` 和节拍控制）。
```cpp
class HeatData {
public:
    uint8_t currentHeat{};
    uint8_t maxHeat{};
    uint8_t heatCoolDownRate{};
    uint8_t heatCoolDownTimer{};
    uint8_t heatPerShot{};
public:
    HeatData() = default;
    HeatData(uint8_t current, uint8_t max, uint8_t coolDownRate, uint8_t perShot)
    : currentHeat(current)
    , maxHeat(max)
    , heatCoolDownRate(coolDownRate)
    , heatPerShot(perShot) { }
};
```


**HealthData（生命与回复）**
 变量：`currentHealth`，`maxHealth`，`healValue`，`healTimeCounter`，`healResetTime`，`healSpeed`。
 语义：每 `healResetTime / healSpeed` 毫秒回复 `healValue` 点；`healTimeCounter` 为内部累加计时。
 默认值：最大/当前 100，`healValue=3`，`healResetTime=15000ms`，`healSpeed=5`（等效更高频率）。
 ```cpp
// 血量数据结构体
// 每隔一段时间自动回复一定量的血量
// 每 healResetTime/healSpeed 毫秒回复 healValue 点血量
class HealthData {
public:
    uint16_t currentHealth{};
    uint16_t maxHealth{};
    uint8_t  healValue       = 3;
    uint16_t healTimeCounter = 0;
    uint16_t healResetTime   = 15000;
    uint8_t  healSpeed       = 5;
};
 ```

**SpatialMovementData（空间与移动）**
 变量：`canCrossBorder`，`consecutiveCollisionCount`，`moveSpeed`（int8），`currentPosX/Y`，`refPosX/Y`，`sizeX/Y`。
 构造：可一次性设置“是否可越界、速度、当前位置/参考位置、尺寸”。
 语义：
     `currentPos`：实际显示/碰撞后的生效位置。
     `refPos`：预期位置（用于先计算再做碰撞回退）。
     `consecutiveCollisionCount`：连续碰撞计数，可用于卡边/弹跳等处理。
     `canCrossBorder`：是否允许越界飞行（闪电线之类特种弹可开启）。
```cpp
/**
 * @brief 空间移动数据结构体
 * @note  包含位置尺寸和移动速度
 * @param moveSpeed 移动速度
 * @param currentPosX 初始X位置，是物体左上角的X坐标
 * @param currentPosY 初始Y位置，是物体左上角的Y坐标
 * @param refPosX 参考X位置，用于碰撞检测后的回退
 * @param refPosY 参考Y位置，用于碰撞检测后的回退
 * @param sizeX 宽度
 * @param sizeY 高度
 */
class SpatialMovementData {
public:
    //能否穿越边界
    bool canCrossBorder = false;
    //连续触发碰撞次数（用来碰撞检测后的处理）
    uint8_t consecutiveCollisionCount = 0;
    //移动速度
    int8_t moveSpeed{};
    //位置尺寸
    int16_t currentPosX{};
    int16_t currentPosY{};
    int16_t refPosX{};
    int16_t refPosY{};
    uint8_t sizeX{};
    uint8_t sizeY{};
public:
    SpatialMovementData() = default;
    /**
     * @brief 构造函数
     * @param speed 移动速度，可以取负值表示反方向移动
     * @param currentPosX 现在X位置
     * @param currentPosY 现在Y位置
     * @param refPosX 期望X位置
     * @param refPosY 期望Y位置
     * @param sx 宽度
     * @param sy 高度
     */
    SpatialMovementData(
        bool canCrossBorder, int8_t speed, uint8_t currentPosX, uint8_t currentPosY, uint8_t refPosX, uint8_t refPosY,
        uint8_t sx, uint8_t sy
    )
    : canCrossBorder(canCrossBorder)
    , moveSpeed(speed)
    , currentPosX(currentPosX)
    , currentPosY(currentPosY)
    , refPosX(refPosX)
    , refPosY(refPosY)
    , sizeX(sx)
    , sizeY(sy) { }
};
```


**RoleAttackData（攻击参数）**
 字段：`shootSpeed`，`shootCooldownTimer`，`shootCooldownResetTime`，`shootCooldownSpeed`，`attackPower`，`collisionPower`，`bulletSpeed`，`bulletRange`，`bulletDamageMultiplier`。
 语义：
     冷却体系：开火将 `shootCooldownTimer` 置为 `shootCooldownResetTime`，随后按帧减速（受 `shootCooldownSpeed` 影响）；计时至 0 才可再开火。
     `attackPower`：子弹/技能的基础伤害。
     `collisionPower`：实体体撞造成的伤害。
     `bulletSpeed`/`bulletRange`/`bulletDamageMultiplier`：子弹飞行速度、范围（火球类有效）、伤害倍率（闪电链类有效）。
```cpp
class RoleAttackData {
public:
    //攻击速度
    uint8_t  shootSpeed;
    uint16_t shootCooldownTimer;
    uint16_t shootCooldownResetTime;
    uint8_t  shootCooldownSpeed;
    //攻击力
    uint8_t attackPower;
    uint8_t collisionPower;
    //子弹速度
    int8_t bulletSpeed;
    //子弹伤害范围
    uint8_t bulletRange;//只对火球弹生效
    //子弹伤害倍率
    float bulletDamageMultiplier;//只对闪电链弹生效
};
```

**InitData（初始化）**
 变量：`isInited`，`posX/posY`，`init_count`。
 用法：实体首次加入场景时从此读取出生点/初始状态；`isInited`控制是否进入正常逻辑。

**ActionData（动作控制）**
 变量：`currentState`，`attackMode`，`moveMode`。
 用法：由输入或 AI 写入；逻辑层依据其驱动运动与攻击。

**DeathData（死亡信息）**
 变量：`isDead`，`deathTimer`（默认 500ms），`dropExperiencePoints`。
 用法：死亡后倒计时播放动画；清理阶段根据阵营发经验（由管理器处理）。

**RoleData（角色聚合体）**
 聚合：`InitData`、`RoleIdentity`、`isActive`、`DeathData`、`ActionData`、`HealthData`、`level`、`HeatData`、`RoleAttackData`、`SpatialMovementData`、`img`。
 默认构造初始化要点：
     身份 UNKNOWN、`isActive=false`、`level=1`。
     生命/热量/攻击参数设了合理初值：最大生命 100；热量上限 100、每枪+10、冷却速率 10；冷却重置 1000ms；攻 10/撞 5；子弹速度 1；火球范围 1；闪电倍率 1.5。
     空间数据初始移动速度 1，位置/尺寸 0。
     `img=nullptr`（注释明确“此处易出错，若有问题优先检查”）。
 典型交互：
     管理器在更新状态前用 `refPos` 做碰撞探测，结果再更新 `currentPos`。
     玩家升级逻辑在管理器的状态更新阶段主动调用（基于 `experiencePoints` 等外部字段，非此处）。
     Perk 卡片直接修改 `RoleData`内子段（例如血量上限、射速、热量、移动等）。
```cpp
class RoleData {
public:
    //初始化位置
    InitData initData;
    //基本信息
    RoleIdentity identity;
    bool         isActive = false;
    //死亡状态信息
    DeathData deathData;
    //动作信息
    ActionData actionData;
    //血量信息
    HealthData healthData;
    //等级信息
    uint8_t level;
    //热量信息
    HeatData heatData;
    //攻击信息
    RoleAttackData attackData;
    //空间移动信息
    SpatialMovementData spatialData;
    //图片信息
    const Image *img = nullptr; //易出错的地方，有问题优先检查
    //角色图片，指向同一片内存，利用浅拷贝的特性
public:
    RoleData() {
        identity = RoleIdentity::UNKNOWN;
        //初始化信息
        initData.posX       = 0;
        initData.posY       = 0;
        initData.init_count = 0;
        //血量信息初始化
        healthData.currentHealth   = 100;
        healthData.maxHealth       = 100;
        healthData.healValue       = 3;
        healthData.healTimeCounter = 0;
        healthData.healResetTime   = 15000;
        healthData.healSpeed       = 5;
        //等级初始化
        level = 1;
        //热量信息初始化
        heatData = HeatData(0, 100, 10, 10);
        //射击后，cooldown计时器增加，只有当计时器达到0时才能再次射击
        attackData.shootSpeed             = 5;
        attackData.shootCooldownTimer     = 0;
        attackData.shootCooldownSpeed     = 5;
        attackData.attackPower            = 10;
        attackData.collisionPower         = 5;
        attackData.shootCooldownResetTime = 1000; // Added initialization for shootCooldownResetTime
        //子弹速度
        attackData.bulletSpeed            = 1;
        //火球弹伤害范围
        attackData.bulletRange            = 1;
        //闪电链伤害倍率
        attackData.bulletDamageMultiplier = 1.5f;
        //空间移动信息初始化
        spatialData = SpatialMovementData(0, 1, 0, 0, 0, 0, 0, 0);
        //死亡状态信息初始化
        deathData.deathTimer = 500;
        deathData.isDead     = false;
        deathData.dropExperiencePoints = 0;
        //图片信息初始化
        img = nullptr;
    }
};
```


**BulletData（子弹聚合体）**
 聚合：`fromIdentity`（默认 UNKNOWN）、`DeathData`、`SpatialMovementData`、`damage`、`range`、`type`、`isActive`、`img`。
 构造函数（重载二）：
     默认构造：不做初始化。
     参数构造：
         参数：`speed`，`currentPosX/Y`，`dmg`，`rg`，`type`，`dmgMultiplier=1.0f`。
         逻辑：按 `type` 分支设置 `damage`、`spatialData`、`isActive=true`、`range=rg`；`LIGHTNING_LINE` 设为 `canCrossBorder=true`。
```cpp
class BulletData {
public:
    //来自
    RoleIdentity fromIdentity{RoleIdentity::UNKNOWN};
    //死亡信息
    DeathData deathData;
    //空间移动信息
    SpatialMovementData spatialData;
    //伤害值
    uint8_t damage{};
    uint8_t range{};
    //子弹类型
    BulletType type{};
    //是否激活
    bool isActive{true};
    //图片信息
    const Image *img = nullptr;
public:
    BulletData() = default;
    BulletData(int8_t speed, uint8_t currentPosX, uint8_t currentPosY, uint8_t dmg, uint8_t rg, BulletType type , float dmgMultiplier = 1.0f) {
        //根据类型进行初始化
        this->type = type;
        switch (this->type) {
        case BulletType::BASIC:
            //img = &bulletImg;
            damage = dmg;
            spatialData =
                SpatialMovementData(false, speed, currentPosX, currentPosY, currentPosX, currentPosY, img->w, img->h);
            isActive = true;
            range    = rg;
            break;
        case BulletType::FIRE_BALL:
            //img = &fireBallImg;
            damage = dmg;
            spatialData =
                SpatialMovementData(false, speed, currentPosX, currentPosY, currentPosX, currentPosY, img->w, img->h);
            isActive = true;
            range    = rg;
            break;
        case BulletType::LIGHTNING_LINE:
            //img = &lightningLineImg;
            damage = dmg;
            spatialData =
                SpatialMovementData(true, speed, currentPosX, currentPosY, currentPosX, currentPosY, img->w, img->h);
            isActive = true;
            range    = rg;
            break;
        }
    }
};
```

#### 游戏实体层
实体层以抽象的角色基类为核心，所有角色共享一套数据驱动的状态与行为流程：初始化、决策、执行、推进与渲染。

玩家与敌人分别在其派生类中实现具体的思考与行动——玩家由输入驱动，敌人由定时与模式切换驱动。当角色需要开火时，通过角色基类提供的子弹工厂按类型生成不同机制的弹药：普通弹以基础伤害直击，火球弹在直击之外叠加范围溅射，闪电链以倍数加成的穿透伤害形成压制。

角色推进采用“参考位置 + 碰撞回退”的模式：先根据移动意图计算参考坐标，再用轴对齐包围盒检测并按碰撞方向和连续碰撞次数逐步回退，避免卡边。

数值系统围绕冷却、热量和回血三条独立的时序通道运行：冷却按步长与速度递减至零方可再次射击，热量每固定周期按散热速率衰减，回血以计时器触发离散恢复。

所有实体的创建、更新、碰撞与回收由实体管理器统一编排，管理器在行为与状态阶段之间明确分离职责：先让角色与子弹各自做决策与动作，再推进位置与伤害结算，随后清理失效对象并为玩家结算经验，最后渲染画面。

整体以固定容量的嵌入式容器与临界区保护保证在资源受限与多任务环境下的确定性与安全性，同时通过将机制落在数据结构上，保留足够的可调性与扩展空间。

```cpp
#ifndef ROLE_HPP
#define ROLE_HPP

#include "basicData.hpp"
#include "bullet.hpp"
#include "etl/algorithm.h"

extern uint8_t controlDelayTime ;
/**
 * @brief 角色接口类
 * @note 所有角色类均需继承该接口类
 * @note think()函数用于实现AI逻辑，决定Action状态
 */
class IRole {
protected:
    RoleData* m_pdata = nullptr ;
public:
    IRole()  {
        if(m_pdata == nullptr)
            m_pdata = new RoleData();
    }
    virtual ~IRole() {
        if(m_pdata != nullptr)
            delete [] m_pdata;
        m_pdata = nullptr;
    }
public:
    RoleData* getData() {
        return m_pdata;
    }
    virtual void shoot(uint8_t x , uint8_t y , BulletType type) = 0;
    virtual void doAction() = 0;
    virtual void init() = 0;
    virtual void die() = 0;
    virtual void think() = 0;
    virtual void drawRole() = 0; // 画角色函数声明
    IBullet* createBullet(uint8_t x , uint8_t y , BulletType type ) ;
    void move(int8_t dirX, int8_t dirY , bool ignoreSpeed  = false) ;
    void update(CollisionResult collisionResult) ;
    void takeDamage(uint8_t damage) ;
    bool isActive() ;
};
#endif // ROLE_HPP
```

实现上仅重点展示`update()`函数，该函数会在`GameEntityManager`中调用，

```cpp
void IRole::update(CollisionResult collisionResult) {
    //初始化
    if (!m_pdata->initData.isInited) {
        //首次初始化
        init();
        return;
    }
    if (m_pdata->deathData.isDead) {
        die();
        return;
    }
    //位置更新
    if (!collisionResult.isCollision) {
        m_pdata->spatialData.currentPosX = m_pdata->spatialData.refPosX;
        m_pdata->spatialData.currentPosY = m_pdata->spatialData.refPosY;
    } else {
        m_pdata->spatialData.refPosX = m_pdata->spatialData.currentPosX;
        m_pdata->spatialData.refPosY = m_pdata->spatialData.currentPosY;
    }
    //碰撞处理
    //碰撞处理在位置更新后进行
    if (collisionResult.isCollision) {
        //用来避免单次碰撞一次退回的无法解决碰撞的问题
        m_pdata->spatialData.consecutiveCollisionCount++;
        //碰撞后退回
        uint8_t backStep = 2*m_pdata->spatialData.consecutiveCollisionCount ;
        if (m_pdata->identity == RoleIdentity::Player) backStep = 5*m_pdata->spatialData.consecutiveCollisionCount ;
        if (collisionResult.direction == CollisionDirection::LEFT)
            m_pdata->spatialData.refPosX = etl::max(m_pdata->spatialData.refPosX - backStep, 0);
        else if (collisionResult.direction == CollisionDirection::RIGHT)
            m_pdata->spatialData.refPosX =
                etl::min(m_pdata->spatialData.refPosX + backStep, 128 - m_pdata->spatialData.sizeX);
        else if (collisionResult.direction == CollisionDirection::UP)
            m_pdata->spatialData.refPosY = etl::max(m_pdata->spatialData.refPosY - backStep, 0);
        else if (collisionResult.direction == CollisionDirection::DOWN)
            m_pdata->spatialData.refPosY =
                etl::min(m_pdata->spatialData.refPosY + backStep, 64 - m_pdata->spatialData.sizeY);
    } else {
        m_pdata->spatialData.consecutiveCollisionCount = 0;
    }
    //射击冷却处理
    if (m_pdata->attackData.shootCooldownTimer > 0) {
        uint8_t shootCooldownSpeed = m_pdata->attackData.shootCooldownSpeed;
        if (controlDelayTime * shootCooldownSpeed >= m_pdata->attackData.shootCooldownTimer)
            m_pdata->attackData.shootCooldownTimer = 0;
        else
            m_pdata->attackData.shootCooldownTimer -= controlDelayTime * shootCooldownSpeed;
    }
    //热量冷却处理
    if (m_pdata->heatData.currentHeat > 0) {
        m_pdata->heatData.heatCoolDownTimer += controlDelayTime;
        if (m_pdata->heatData.heatCoolDownTimer >= 200) //每200ms降温一次
        {
            if (m_pdata->heatData.currentHeat <= m_pdata->heatData.heatCoolDownRate)
                m_pdata->heatData.currentHeat = 0;
            else
                m_pdata->heatData.currentHeat -= m_pdata->heatData.heatCoolDownRate;
            m_pdata->heatData.heatCoolDownTimer = 0;
        }
    }
    //回血处理
    if (m_pdata->healthData.currentHealth < m_pdata->healthData.maxHealth) {
        m_pdata->healthData.healTimeCounter += controlDelayTime * m_pdata->healthData.healSpeed;
        if (m_pdata->healthData.healTimeCounter >= m_pdata->healthData.healResetTime) {
            if (m_pdata->healthData.currentHealth + m_pdata->healthData.healValue >= m_pdata->healthData.maxHealth)
                m_pdata->healthData.currentHealth = m_pdata->healthData.maxHealth;
            else
                m_pdata->healthData.currentHealth += m_pdata->healthData.healValue;
            m_pdata->healthData.healTimeCounter = 0;
        }
    }
    //检查血量状态
    if (m_pdata->healthData.currentHealth == 0 && !m_pdata->deathData.isDead) {
        m_pdata->deathData.isDead = true;
    }
}
```

其余代码就不展示了，基本就是复制，只需根据需求对虚接口进行重写即可

#### 管理器层
**该层级的设计主要是针对整个游戏运行的职责进行划分解耦**，`GameEntityManager`,`GameProgressManager`,`GamePerkCardManager`三者协作的关键在于清晰的职责边界与状态门控：进度与选卡控制何时推进逻辑或暂停到过场/选卡，实体管理器提供确定性更新与资源回收，卡片管理器以数据驱动改变玩家的RoleData


`GameEntityManager`统一掌控角色与子弹的生命周期与时序，采用固定容量的嵌入式向量存储多态指针，所有增删与遍历在临界区内执行，按“行动→状态→清理→渲染”的分阶段流程驱动：行动阶段调用角色/子弹的思考与动作，状态阶段推进位置与AABB碰撞、伤害与范围伤害并对玩家执行升级检查，清理阶段删除失效实体并向玩家结算经验，渲染阶段输出图像。
```cpp
#ifndef GAMEENTITYMANAGER_HPP
#define GAMEENTITYMANAGER_HPP

#include "math.h"
#include "role.hpp"
#include "leadingRole.hpp"
#include "enemyRole.hpp"
#include "etl/vector.h"

#include "FreeRTOS.h"
#include "task.h"

  
// 管理游戏中的实体：角色和子弹
// 会被多个模块引用
// 归gameProgressManager管理
class GameEntityManager {
public:
    etl::vector<IRole *, 20>    m_roles;   //角色池，存放所有角色指针
    etl::vector<IBullet *, 100> m_bullets; //子弹池，存放所有子弹指针
    bool                       isGameOver = false;
public:
    GameEntityManager() = default;
    ~GameEntityManager() {
        cleanUpAllRoles(true);
        cleanUpAllBullets(true);
    }
    GameEntityManager(const GameEntityManager &)            = delete;
    GameEntityManager &operator=(const GameEntityManager &) = delete;
public:

    //Add添加/********************************************************************/
    bool addRole(IRole *role) {
        if (role == nullptr || m_roles.full()) return false;
        taskENTER_CRITICAL();
        m_roles.push_back(role);
        taskEXIT_CRITICAL();
        return true;
    }

    bool addBullet(IBullet *bullet) {
        if (bullet == nullptr || m_bullets.full()) return false;
        taskENTER_CRITICAL();
        m_bullets.push_back(bullet);
        taskEXIT_CRITICAL();
        return true;
    }
/********************************************************************/
    //remove删除并销毁
 /********************************************************************/
    bool removeAndDestroyRole(IRole *role) {
        if (role == nullptr) return false;
        taskENTER_CRITICAL();
        auto it = etl::find(m_roles.begin(), m_roles.end(), role);
        if (it != m_roles.end()) {
            m_roles.erase(it);
            delete role;
            taskEXIT_CRITICAL();
            return true;
        }
        taskEXIT_CRITICAL();
        return false;
    }

  
    bool removeAndDestroyBullet(IBullet *bullet) {
        if (bullet == nullptr) return false;
        taskENTER_CRITICAL();
        auto it = etl::find(m_bullets.begin(), m_bullets.end(), bullet);
        if (it != m_bullets.end()) {
            m_bullets.erase(it);
            delete bullet;
            taskEXIT_CRITICAL();
            return true;
        }
        taskEXIT_CRITICAL();
        return false;
    }
    /********************************************************************/
    //获取player角色指针
 /********************************************************************/

    IRole *getPlayerRole() {
        taskENTER_CRITICAL();
        for (auto role : m_roles) {
            if (role != nullptr && role->getData()->identity == RoleIdentity::Player) {
                taskEXIT_CRITICAL();
                return role;
            }
        }
        taskEXIT_CRITICAL();
        return nullptr;
    }
   /********************************************************************/
    //清除需要回收的角色和子弹
 /********************************************************************/
    void cleanupInvalidRoles() {
       //...
    }

    void cleanupInvalidBullets() {
       //...
    }
  /********************************************************************/

    //碰撞检测
/********************************************************************/
    CollisionResult checkRoleRefPositionCollision(IRole *role_A) {
        //...
        return collisionDetected;
    }

  
    bool checkBulletRefPositionCollision(IBullet *bullet_A) {	        //...
        return collisionDetected;
    }
    /********************************************************************/
    //子弹范围伤害检测
/********************************************************************/
    void checkBulletRangeDamage(IBullet *bullet_A) {
        //....
    }
 /********************************************************************/
    //更新所有实体位置和状态
  /********************************************************************/

    /**
     * @brief 更新所有角色状态和位置
     */
    void updateAllRolesState() {
        taskENTER_CRITICAL();
        for (auto rolePtr : m_roles) {
            if (rolePtr != nullptr) {
                CollisionResult collisionResult = {false, CollisionDirection::NONE};
                if (rolePtr->getData()->initData.isInited) {
                    //碰撞处理
                    collisionResult = checkRoleRefPositionCollision(rolePtr);
                }
                rolePtr->update(collisionResult);
                if(rolePtr->getData()->identity == RoleIdentity::Player) {
                    LeadingRole* playerRole = (LeadingRole*)rolePtr;
                    playerRole->levelUp(); //检查升级
                }
            }
        }
        taskEXIT_CRITICAL();
    }

    /**
     * @brief 更新所有子弹状态和位置
     */
    void updateAllBulletsState() {
        taskENTER_CRITICAL();
        for (auto bulletPtr : m_bullets) {
            if (bulletPtr != nullptr) {
                CollisionResult collisionResult = {false, CollisionDirection::NONE};
                //碰撞处理
                bool isCollision = checkBulletRefPositionCollision(bulletPtr);
                if (isCollision) {
                    collisionResult.isCollision = true;
                    checkBulletRangeDamage(bulletPtr);
                }
                bulletPtr->update(collisionResult);
            }
        }
        taskEXIT_CRITICAL();
    }
  /********************************************************************/

    // 更新所有角色的AI行动决策
    /********************************************************************/
    void updateAllRolesActions() {
        taskENTER_CRITICAL();
        for (auto rolePtr : m_roles) {
            if (rolePtr != nullptr && rolePtr->getData()->initData.isInited) {
                rolePtr->think();
                rolePtr->doAction();
            }
        }
        taskEXIT_CRITICAL();
    }


    void updateAllBulletsActions() {
        taskENTER_CRITICAL();
        for (auto bulletPtr : m_bullets) {
            if (bulletPtr != nullptr) {
                bulletPtr->doAction();
            }
        }
        taskEXIT_CRITICAL();
    }
    /********************************************************************/
    // 绘制所有实体
   /********************************************************************/
    void drawAllRoles() {
        taskENTER_CRITICAL();
        for (auto rolePtr : m_roles) {
            if (rolePtr != nullptr && rolePtr->getData() != nullptr && rolePtr->getData()->img != nullptr) {
                rolePtr->drawRole();
            }
        }
        taskEXIT_CRITICAL();
    }

    void drawAllBullets() {
        taskENTER_CRITICAL();
        for (auto bulletPtr : m_bullets) {
            if (bulletPtr != nullptr && bulletPtr->m_data != nullptr && bulletPtr->m_data->img != nullptr) {
                bulletPtr->drawBullet();
            }
        }
        taskEXIT_CRITICAL();
    }
    /********************************************************************/

    //清除所有实体，资源回收
    void clearAllEntities(bool deleteObjects = true) {
        cleanUpAllRoles(deleteObjects);
        cleanUpAllBullets(deleteObjects);
    }
private:
    //清除所有角色和子弹，资源回收
    /********************************************************************/
    void cleanUpAllRoles(bool deleteObjects = true) {
        if (!deleteObjects) return;
        for (auto rolePtr : m_roles) {
            if (rolePtr != nullptr) delete rolePtr;
        }
        m_roles.clear();
    }

    void cleanUpAllBullets(bool deleteObjects = true) {
        if (!deleteObjects) return;
        for (auto bulletPtr : m_bullets) {
            if (bulletPtr != nullptr) delete bulletPtr;
        }
        m_bullets.clear();
    }
    /********************************************************************/
    //玩家角色获取经验值
  /********************************************************************/
    void gainDropExperiencePoints(uint16_t points) {
        LeadingRole* playerRole = (LeadingRole*)getPlayerRole();
        if (playerRole != nullptr && playerRole->getData()->level < 10) {
            playerRole->experiencePoints += points;
        }
    }
    /********************************************************************/
};

#endif // GAMEENTITYMANAGER_HPP
```




`GameProgressManager`负责关卡与波次推进以及Boss触发和过场展示：当清场（场上仅剩玩家）时递进波次/关卡，在关键节点触发选卡并按章节规则生成阵型或Boss，从而把战斗节奏和演出串联起来。
```cpp
#ifndef GAMEPROGRESSMANAGER_HPP
#define GAMEPROGRESSMANAGER_HPP
#include "gamePerkCardManager.hpp"
extern uint8_t             controlDelayTime; // 由 threads.cpp 定义
extern GameEntityManager   g_entityManager;
extern GamePerkCardManager g_perkCardManager;
enum class WaveType {
    //魑魅阵型
    CHIMEI_LINE = 0, // 10 魑魅直线阵
    CHIMEI_TRIANGLE, // 10 魑魅三角阵
    //飞廉阵型
    THREE_Feilian,   // 三飞廉
    FEILIAN_CLUSTER, // 飞廉群
    //古雕阵型
    GUDIAO_SINGLE,
    GUODIAO_DOUBLE,
    GUDIAO_SQUARE,
    //混合阵型
    MIXED_SMALL,  // 混合小型阵型 2feilian + 1Gudiao
    MIXED_MEDIUM, // 混合中型阵型 5feilian +1Gudiao
    MIXED_LARGE,  // 混合大型阵型 5feilian + 2Gudiao
};

enum class BOSS_TYPE {
    NONE = 0,
    TAO_TIE,   // 饕餮
    XIANG_LIU, // 相柳
    TAO_WU,    // 梼杌
};

class GameProgressManager {
public:
    uint8_t currentChapter = 1; // 当前游戏关卡
    uint8_t lastChapter    = 4; // 最大游戏关卡
    uint8_t currentWave            = 1;  // 当前波次
    uint8_t maxWave                = 15; // 最大波次
    uint8_t currentChapterMaxWaves = 0;  // 当前关卡总波次
    uint8_t time_count = 0;
    bool     isPlayingOpeningCG = false; // 是否播放开场动画
    uint16_t openingCGTimer     = 0;     // 开场动画计时器
    bool     isPlayingClearCG = false; // 是否播放通关动画
    uint16_t clearCGTimer     = 0;     // 通关动画计时器
    bool chatpter4Warning = false; // 第四章警告标记
    BOSS_TYPE showWhichBoss = BOSS_TYPE::NONE; // 展示Boss类型
    bool      showBoss      = false;           // 是否 展示Boss海报
    uint16_t  showBossTimer = 0;               // 展示Boss海报计时器
    //播放3秒Boss海报
    bool PauseGame = false; // 暂停游戏标记
public:
    GameProgressManager()  = default;
    ~GameProgressManager() = default;
    GameProgressManager(const GameProgressManager &)            = delete;
    GameProgressManager &operator=(const GameProgressManager &) = delete;
public:
    // 重置游戏进度
    void resetGameProgress() {
        // 重置实体管理器
        //...
    }

  

    // 更新游戏进度

    void updateGameProgress() {
     //...
    }

  

    void AddWaveEnemies() {
    //...
    }

  

    // 绘图展示功能
    // 绘制开场动画
    void drawOpeningCG() {
        if (openingCGTimer >= 2 * controlDelayTime)
            openingCGTimer -= 2 * controlDelayTime;
        else
            isPlayingOpeningCG = false;
        // 绘制动态圆圈效果
        OLED_DrawCircle(64, 32, 30 - (openingCGTimer / 100)+10, OLED_COLOR_NORMAL);
        OLED_DrawCircle(64, 32, 20 - (openingCGTimer / 150)+30 , OLED_COLOR_NORMAL);
        OLED_PrintString(30, 28, "CHIP TANKS", &font8x6, OLED_COLOR_NORMAL);
    }

  

    void drawClearCG() {
        if (clearCGTimer >= 2 * controlDelayTime)
            clearCGTimer -= 2 * controlDelayTime;
        else {
            isPlayingClearCG           = false;
            g_entityManager.isGameOver = true; // 游戏结束
        }
        // 致谢
        OLED_PrintString(1, 28, "THANK YOU FOR", &font8x6, OLED_COLOR_NORMAL);
        OLED_PrintString(1, 40, "PLAYING MY GAME", &font8x6, OLED_COLOR_NORMAL);
    }

    // 绘制展示Boss海报
    void drawShowBoss() {
        //...
    }
};

#endif // GAMEPROGRESSMANAGER_HPP
```


`GamePerkCardManager`管理Roguelike式成长：基于配置初始化卡片仓库，按需抽取待选卡、应用所选增益到玩家并回库未选卡，同时提供选卡界面渲染与交互状态。
```cpp
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
    GamePerkCardManager()  = default;
    ~GamePerkCardManager() = default;
    GamePerkCardManager(const GamePerkCardManager &)            = delete;
    GamePerkCardManager &operator=(const GamePerkCardManager &) = delete;

private:
    static const uint8_t MAX_CARD_COUNT       = 22; // 总卡片数（2*10 +1*2=22）
    static const uint8_t SELECTION_SLOT_COUNT = 3;  // 选卡槽数量（3张）
    etl::vector<PerkCard, MAX_CARD_COUNT>       m_cardWarehouse;  // 卡片仓库（存所有卡片）
    etl::vector<PerkCard, SELECTION_SLOT_COUNT> m_selectionSlots; // 选卡槽（临时存3张待选）

public:
    bool    isInited        = false; // 是否已初始化
    bool    m_isSelecting   = false; // 选卡状态（标记是否处于“选卡中”，避免重复触发）
    uint8_t m_selectedIndex = 0;     // 选中卡片索引（选卡槽内索引）
    uint8_t m_selectedSize  = 0;     // 选中卡片数量（选卡槽内数量）
public:
    // 初始化卡片仓库
    void initWarehouse() {
        m_cardWarehouse.clear();
        m_selectionSlots.clear();
        m_isSelecting = false;
        isInited      = true;
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
        // 可用卡片不足3张时，取全部（避免选卡槽为空）
        uint8_t selectCount = etl::min(SELECTION_SLOT_COUNT, (uint8_t)m_cardWarehouse.size());
        if (selectCount == 0) {
            return false; // 无可用卡片
        }
        m_selectedSize = selectCount;
        // 随机抽取selectCount张卡片，放入选卡槽（从仓库移除，避免重复抽）
        for (uint8_t i = 0; i < selectCount; ++i) {
            // 嵌入式随机算法：用系统滴答定时器做模运算（无需rand()）
            uint8_t randIdx      = rand() % m_cardWarehouse.size();
            uint8_t warehouseIdx = randIdx;
            // 从仓库移动到选卡槽（避免拷贝，提升效率）
           m_selectionSlots.push_back(m_cardWarehouse[warehouseIdx]);
            m_cardWarehouse.erase(m_cardWarehouse.begin() + warehouseIdx);
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
        PerkCard     selectedCard = m_selectionSlots[slotIndex];
        LeadingRole *player       = (LeadingRole *)g_entityManager.getPlayerRole();
        if (player == nullptr) {
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
            OLED_PrintString(10, 10 + (i * 20), m_selectionSlots[i].name, &font8x6, OLED_COLOR_NORMAL);
        }
        OLED_DrawCircle(5, 12 + (m_selectedIndex * 20), 2, OLED_COLOR_NORMAL); // 绘制选中指示器
    }
};

#endif // GAMEPERKCARDMANAGER_HPP
```
#### 应用逻辑层
按“主循环驱动、输入分发、渲染输出”三类职责拆分成独立的 FreeRTOS 任务，全局状态作为门控，保证在过场、选卡、战斗三种模式之间平滑切换。

核心数据与行为推进集中在 `gameControlThread`，输入解析与状态写入在 `keyScanThread`，画面组合与输出在 `oledTaskThread`。

给出伪代码如下：
```cpp
oledTaskThread:
初始化: delay(30) → OLED_Init()
每帧:
OLED_NewFrame()
若 !g_entityManager.isGameOver:
若 g_progressManager.isPlayingOpeningCG: drawOpeningCG()
否则若 g_progressManager.isPlayingClearCG: drawClearCG()
否则若 g_progressManager.showBoss: drawShowBoss()
否则若 g_perkCardManager.m_isSelecting && g_perkCardManager.isInited: drawSelectionUI()
否则（战斗界面）:
若玩家存在: drawPlayerHUD(HP, LV)
g_entityManager.drawAllRoles()；g_entityManager.drawAllBullets()
OLED_ShowFrame()
delay(controlDelayTime * 2)



keyScanThread:
初始化: scanDelay=40 → key.init()
每轮:
key.scan()
若游戏进行中（非 GameOver/开场/Boss 展示/通关）:
若 !g_perkCardManager.m_isSelecting（战斗输入态）:
scanDelay=40
pLeadingRole = (LeadingRole*) g_entityManager.getPlayerRole()
若玩家存在: 按键映射到动作态与方向（LEFT/DOWN/UP/RIGHT → ActionState::MOVING + MoveMode）
否则（选卡输入态）:
scanDelay=100
光标索引在 [0, m_selectedSize-1] 间夹紧
keyDown → 索引+1（上限保护）；keyUp → 索引-1（下限保护）
keyConfirm → g_perkCardManager.selectCard(m_selectedIndex)
delay(scanDelay)



gameControlThread:
启动: g_entityManager.isGameOver = true（强制初始重置）
每轮:
若 g_entityManager.isGameOver: g_progressManager.resetGameProgress()（清空实体→添加玩家→初始化卡池→开场动画计时）
否则若逻辑可推进（非开场/非Boss展示/非通关/非选卡）:
进度推进: g_progressManager.updateGameProgress()（清场判定→波次/关卡递进→半程/过关触发选卡→AddWaveEnemies()）
行为阶段: g_entityManager.updateAllRolesActions()；g_entityManager.updateAllBulletsActions()
状态阶段:
g_entityManager.updateAllRolesState()（AABB碰撞→方向判定→位移/回退→冷却/热量/回血→玩家 levelUp()）
g_entityManager.updateAllBulletsState()（命中检测→范围伤害→位移/寿命）
清理阶段: cleanupInvalidRoles()（敌人死亡发经验、玩家死亡置 GameOver）；cleanupInvalidBullets()
delay(controlDelayTime)


```



#### timer定时器的设计
代码比较重点一个设计在于timer计时器，每一帧增加或减少帧间隔的时间（相当于决定多少帧触发某一个动作，可以用此来作为一个门控）

- shootCooldownTimer：射击冷却倒计时，归零前禁止再次开火。
- heatCoolDownTimer：热量冷却触发间隔计时，累计到阈值（200ms）降低一次热量。
- healTimeCounter：被动回血累积时间，达到阈值（healResetTime）回复 healValue。
- deathTimer：死亡后保留与动画展示时间窗，便于延迟清理。
- consecutiveCollisionCount（计数型“时间替代”）：连续帧碰撞累计，扩大回退步幅直至解除。
- action_timer（敌人/技能内部使用）：驱动攻击模式阶段切换与技能节拍。
- openingCGTimer / clearCGTimer / showBossTimer（过场类）：控制开场、通关、Boss 展示持续时间。
- shootCooldownResetTime / healResetTime 等“阈值型常量”：定义倒计时起点或触发周期长度。

**核心：所有计时器是“离散步长 + 阈值触发”模型**
增加每一帧的`controlDelayTime`，用统一帧步长驱动一组简单的线性计时器，以门槛触发和双重约束塑造节奏与资源循环

**核心作用**：**解耦时间进程和行为**，计时器只管理时间进程，不直接耦合行为；行为在触发点读取状态执行。


## Bug 记录与解决方案

### 1. 热量系统临界区死锁

**问题现象**：当玩家热量达到上限后，游戏完全冻结，所有角色停止更新，画面不再刷新。

**问题分析**：
在 `LeadingRole::shoot()` 函数中，代码在函数开头调用了 `taskENTER_CRITICAL()` 进入 FreeRTOS 临界区，但在热量检查失败的早期 `return` 语句中**没有调用 `taskEXIT_CRITICAL()` 就直接返回**。

```cpp
// 问题代码
void LeadingRole::shoot(uint8_t x, uint8_t y, BulletType type) {
    taskENTER_CRITICAL();  // 进入临界区
    
    if(m_pdata->heatData.currentHeat + ... > m_pdata->heatData.maxHeat)
        return; // BUG! 没有退出临界区就返回  调度器永久锁死
    ...
}
```

当热量超限时，函数直接返回，导致调度器被永久禁用，所有 FreeRTOS 任务都无法运行。

**解决方案**：
为所有早期返回点添加 `taskEXIT_CRITICAL()` 调用：

```cpp
// 修复后
if(m_pdata->heatData.currentHeat + ... > m_pdata->heatData.maxHeat) {
    taskEXIT_CRITICAL();  // 先退出临界区
    return;
}
```

---

### 2. 热量冷却无符号下溢

**问题现象**：玩家热量值会莫名其妙突然变得非常大（如从10点突然变成240多点）。

**问题分析**：
在 `IRole::update()` 的热量冷却逻辑中，条件检查与实际减少量不匹配：

```cpp
// 问题代码
if (m_pdata->heatData.currentHeat <= m_pdata->heatData.heatCoolDownRate)  // 检查 4
    m_pdata->heatData.currentHeat = 0;
else
    m_pdata->heatData.currentHeat -= m_pdata->heatData.heatCoolDownRate * 5; // 减 20
```

当 `currentHeat` 在 5~19 之间时：
- 条件 `<= 4` 不满足，进入 else 分支
- 执行 `currentHeat -= 20`，由于 `currentHeat` 是 `uint8_t`（无符号），发生下溢
- 例如：`10 - 20 = -10`  无符号表示为 `246`

**解决方案**：
统一条件检查与减少量：

```cpp
// 修复后
uint8_t coolAmount = m_pdata->heatData.heatCoolDownRate * 5;
if (m_pdata->heatData.currentHeat <= coolAmount)
    m_pdata->heatData.currentHeat = 0;
else
    m_pdata->heatData.currentHeat -= coolAmount;
```

---

### 3. 快速上下电导致OLED显示翻转

**问题现象**：使用独立电源快速上下电时，OLED 屏幕显示内容发生翻转，有时是180度全屏翻转，有时是部分内容翻转。

**问题分析**：

根据 SSD1306 数据手册，显示方向由两个关键命令控制：
- `0xA0`/`0xA1`：段重映射（Segment Re-map），控制水平方向
- `0xC0`/`0xC8`：COM 输出扫描方向，控制垂直方向

快速上下电时出现翻转的原因：

1. **SSD1306 内部状态不确定**：快速上下电时，SSD1306 的内部寄存器可能没有完全复位到默认值，保留了上次的配置状态
2. **初始化等待时间不足**：原代码只等待 30ms，不足以让电源和 SSD1306 内部电路稳定
3. **I2C 通信可能失败**：上电瞬间 I2C 总线可能不稳定，部分初始化命令丢失
4. **方向配置命令未重复确认**：若单次发送方向命令时恰好传输失败，就会导致显示翻转

**解决方案**：

**1. 增强 `OLED_Init()` 初始化序列**：

```c
void OLED_Init()
{
  // 阶段1: 确保SSD1306完全就绪
  osDelay(100);  // 等待电源稳定和内部复位完成
  
  OLED_SendCmd(0xAE); // 关闭显示
  
  // ... 其他配置 ...
  
  // 阶段4: 显示方向配置（关键！防止翻转）
  // 先发送默认方向命令，再发送目标方向命令，确保状态确定
  OLED_SendCmd(0xA0); // 段重映射：先设为默认
  OLED_SendCmd(0xA1); // 段重映射：再设为翻转
  
  OLED_SendCmd(0xC0); // COM扫描方向：先设为默认
  OLED_SendCmd(0xC8); // COM扫描方向：再设为翻转
  
  // ... 其他配置 ...
  
  osDelay(10);      // 等待电荷泵稳定
  OLED_SendCmd(0xAF); // 开启显示
}
```

**2. 调整线程初始化延时**：

```cpp
void oledTaskThread(void *argument) {
    osDelay(50); // 增加等待时间（加上OLED_Init内部的100ms，共约150ms）
    OLED_Init();
    // ...
}
```

**改进要点总结**：

| 改进项 | 说明 |
|--------|------|
| 增加初始化前延时 | 等待 100ms 让电源和 SSD1306 内部复位完成 |
| 双重方向配置 | 先发默认方向命令，再发目标方向命令，确保状态确定 |
| 规范化命令顺序 | 按 SSD1306 手册推荐顺序重新组织初始化命令 |
| 增加电荷泵稳定延时 | 开启显示前等待 10ms |

**硬件层面补充措施**（若软件修复后问题仍存在）：
1. 在 OLED 模块的 RES 引脚接 GPIO，初始化时先拉低 10ms 再拉高进行硬复位
2. 在 OLED 电源入口增加 100μF 电解电容 + 100nF 陶瓷电容进行滤波
3. 确保 I2C 的 SDA/SCL 有合适的上拉电阻（通常 4.7kΩ）
