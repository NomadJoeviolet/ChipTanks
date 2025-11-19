#include "key.hpp"

KeyGPIO keyRankOutput[4] = {
    {KEY_R1_GPIO_Port, KEY_R1_Pin},
    {KEY_R2_GPIO_Port, KEY_R2_Pin},
    {KEY_R3_GPIO_Port, KEY_R3_Pin},
    {KEY_R4_GPIO_Port, KEY_R4_Pin}
};
KeyGPIO keyColInput[4] = {
    {KEY_C1_GPIO_Port, KEY_C1_Pin},
    {KEY_C2_GPIO_Port, KEY_C2_Pin},
    {KEY_C3_GPIO_Port, KEY_C3_Pin},
    {KEY_C4_GPIO_Port, KEY_C4_Pin}
};
Key key(keyRankOutput, keyColInput);


