#ifndef KEY_HPP
#define KEY_HPP

#include "gpio.h"

typedef struct {
    GPIO_TypeDef* GPIOx;
    uint16_t GPIO_Pin;
}KeyGPIO;

//4*4矩阵键盘类
// class Key{
// public:
//     uint8_t m_keyButton[16] = {0}; //按键状态缓存
//     KeyGPIO* m_rankOutput = nullptr;
//     KeyGPIO* m_colInput = nullptr;

// public:
//     Key(KeyGPIO* rankOutput, KeyGPIO* colInput)
//         :m_rankOutput(rankOutput), m_colInput(colInput){}
//     ~Key(){};

//     void init(){
//         //初始化行输出
//         for(uint8_t r = 0; r <4; r++){
//             HAL_GPIO_WritePin(m_rankOutput[r].GPIOx, m_rankOutput[r].GPIO_Pin, GPIO_PIN_SET);
//         }
//     }

//     void scan(){
//         for(uint8_t r = 0; r <4; r++){
//             //拉低当前行
//             HAL_GPIO_WritePin(m_rankOutput[r].GPIOx, m_rankOutput[r].GPIO_Pin, GPIO_PIN_RESET);
//             //扫描列
//             for(uint8_t c = 0; c <4; c++){
//                 if(HAL_GPIO_ReadPin(m_colInput[c].GPIOx, m_colInput[c].GPIO_Pin) == GPIO_PIN_RESET){
//                     m_keyButton[r*4 + c] = 1;
//                 }else{
//                     m_keyButton[r*4 + c] = 0;
//                 }
//             }
//             //拉高当前行
//             HAL_GPIO_WritePin(m_rankOutput[r].GPIOx, m_rankOutput[r].GPIO_Pin, GPIO_PIN_SET);
//         }
//     }
// };

enum class LeftKeyState {
    KEY_DOWN = 0,
    KEY_UP = 1,
    KEY_RIGHT = 2,
    KEY_LEFT = 3
};

enum class RightKeyState {
    KEY_DOWN = 0,
    KEY_LEFT = 1,
    KEY_RIGHT = 2,
    KEY_UP = 3
};

class Key {
public:
    uint8_t m_leftKeyButton[4] = {0}; //按键状态缓存
    uint8_t m_rightKeyButton[4] = {0};     //按键状态缓存
    KeyGPIO* m_rightInput = nullptr;
    KeyGPIO* m_leftInput = nullptr;
public:
    Key(KeyGPIO* rightInput, KeyGPIO* leftInput)
        :m_rightInput(rightInput), m_leftInput(leftInput) {}
    ~Key(){};

    void init() {
        //初始化按键输入引脚为上拉输入
        //此处假设引脚已经在MX_GPIO_Init中配置为上拉输入，无需重复配置
    }

    void scan() {
        //扫描右侧按键
        for (uint8_t i = 0; i < 4; i++) {
            if (HAL_GPIO_ReadPin(m_rightInput[i].GPIOx, m_rightInput[i].GPIO_Pin) == GPIO_PIN_RESET) {
                m_rightKeyButton[i] = 1;
            } else {
                m_rightKeyButton[i] = 0;
            }
        }
        //扫描左侧按键
        for (uint8_t i = 0; i < 4; i++) {
            if (HAL_GPIO_ReadPin(m_leftInput[i].GPIOx, m_leftInput[i].GPIO_Pin) == GPIO_PIN_RESET) {
                m_leftKeyButton[i] = 1;
            } else {
                m_leftKeyButton[i] = 0;
            }
        }
    }
};


extern Key key;

#endif // KEY_HPP
