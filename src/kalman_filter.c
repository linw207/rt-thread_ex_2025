#include "kalman_filter.h"

void multi_kalman_init(multi_kalman_filter_t* filter,
                      float q, float r1, float r2,
                      float initial_value, float true_value)
{
    filter->q = q;
    filter->r[0] = r1;
    filter->r[1] = r2;
    filter->x_hat = initial_value;
    filter->p = 1.0f;
    filter->true_value = true_value;
    filter->bias[0] = 0;
    filter->bias[1] = 0;
}

float multi_kalman_update(multi_kalman_filter_t* filter,
    float z1, float z2)
{
/* 校准传感器读数 */
float calibrated_z1 = z1 - filter->bias[0];
float calibrated_z2 = z2 - filter->bias[1];

/* 计算卡尔曼增益 */
float k1 = filter->p / (filter->p + filter->r[0]);
float k2 = filter->p / (filter->p + filter->r[1]);

/* 归一化卡尔曼增益 */
float sum_k = k1 + k2;
k1 /= sum_k;
k2 /= sum_k;

/* 状态更新 */
filter->x_hat = k1 * calibrated_z1 + k2 * calibrated_z2;

/* 误差协方差更新 */
filter->p = (1 - k1 - k2) * filter->p + filter->q;

/* 更新传感器偏差 (指数移动平均) */
filter->bias[0] = 0.8f * filter->bias[0] + 0.2f * (z1 - filter->true_value);
filter->bias[1] = 0.8f * filter->bias[1] + 0.2f * (z2 - filter->true_value);

return filter->x_hat;
}
