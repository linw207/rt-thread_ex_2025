#ifndef __KALMAN_FILTER_H__
#define __KALMAN_FILTER_H__

#include <rtthread.h>

/* 多传感器卡尔曼滤波器结构 */
typedef struct {
    /* 滤波参数 */
    float q;      // 过程噪声
    float r[2];   // 传感器测量噪声 [0]:传感器1 [1]:传感器2
    float x_hat;  // 状态估计值
    float p;      // 误差协方差
    
    /* 校准参数 */
    float bias[2];    // 传感器偏差 [0]:传感器1 [1]:传感器2
    float true_value; // 校准用真实值
} multi_kalman_filter_t;

/* 初始化多传感器卡尔曼滤波器 */
void multi_kalman_init(multi_kalman_filter_t* filter, 
                      float q, float r1, float r2,
                      float initial_value, float true_value);

/* 执行多传感器卡尔曼更新 */
float multi_kalman_update(multi_kalman_filter_t* filter,
                         float z1, float z2);

#endif
