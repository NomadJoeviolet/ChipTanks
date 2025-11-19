#ifndef KEY_HPP
#define KEY_HPP

#include "gpio.h"

typedef struct {
    GPIO_TypeDef* GPIOx;
    uint16_t GPIO_Pin;
}KeyGPIO;

class Key{
public:
    uint8_t m_keyButton[16] = {0}; //按键状态缓存
    KeyGPIO* m_rankOutput = nullptr;
    KeyGPIO* m_colInput = nullptr;

public:
    Key(KeyGPIO* rankOutput, KeyGPIO* colInput)
        :m_rankOutput(rankOutput), m_colInput(colInput){}
    ~Key(){};

    void init(){
        //初始化行输出
        for(uint8_t r = 0; r <4; r++){
            HAL_GPIO_WritePin(m_rankOutput[r].GPIOx, m_rankOutput[r].GPIO_Pin, GPIO_PIN_SET);
        }
    }

    void scan(){
        for(uint8_t r = 0; r <4; r++){
            //拉低当前行
            HAL_GPIO_WritePin(m_rankOutput[r].GPIOx, m_rankOutput[r].GPIO_Pin, GPIO_PIN_RESET);
            //扫描列
            for(uint8_t c = 0; c <4; c++){
                if(HAL_GPIO_ReadPin(m_colInput[c].GPIOx, m_colInput[c].GPIO_Pin) == GPIO_PIN_RESET){
                    m_keyButton[r*4 + c] = 1;
                }else{
                    m_keyButton[r*4 + c] = 0;
                }
            }
            //拉高当前行
            HAL_GPIO_WritePin(m_rankOutput[r].GPIOx, m_rankOutput[r].GPIO_Pin, GPIO_PIN_SET);
        }
    }
};

extern Key key;

#endif // KEY_HPP
