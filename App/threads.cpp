#include "main.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h"

#include "etl/vector.h"

etl::vector<int, 10> myVector;


#ifdef __cplusplus
extern "C" {
#endif

void oledTaskThread(void *argument) {
    myVector.push_back(1);
    myVector.push_back(2);
    myVector.push_back(3);
    myVector.push_back(4);
    myVector.push_back(5);
    myVector.push_back(6);
    myVector.push_back(7);
    myVector.push_back(8);
    myVector.push_back(9);
    myVector.push_back(10);
    for (;;) {
        int arraySize = myVector.size();
        int arraySum = 0;
        for (int i = 0; i < arraySize; ++i) {
            arraySum += myVector[i];
        }
        // OLED task implementation
        osDelay(1000);
    }
}

#ifdef __cplusplus
}
#endif




#ifdef __cplusplus
extern "C" {
#endif

void drawPicThread(void *argument) {
    for (;;) {
        // Draw picture task implementation
        osDelay(500);
    }
} 


#ifdef __cplusplus
}
#endif



