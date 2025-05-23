#include <rtthread.h>
#include "kalman_filter.h"
#include <stdlib.h>  // 添加标准库头文件
#include <rtthread.h>
#include <stdlib.h>
#include <time.h>

/* 线程配置 */
#define THREAD_STACK_SIZE 1024
#define THREAD_PRIORITY   9
#define THREAD_TIMESLICE  5

ALIGN(RT_ALIGN_SIZE)
static char thread_k_stack[THREAD_STACK_SIZE];
static struct rt_thread thread_k;

/* 传感器上下文 */
static multi_kalman_filter_t sensor_filter;

/* 模拟传感器读数 */
static float simulate_sensor1(void)
{
    //return 50.0f + (rt_hw_rand() % 200 - 100) / 10.0f; // 50±10
    return 50.0f + (rand() % 200 - 100) / 10.0f;  // 使用标准库rand()
    //return 50.0f + (rt_rand() % 200 - 100) / 10.0f;  // 使用标准库rand()
}

static float simulate_sensor2(void)
{
    //return 50.0f + (rt_hw_rand() % 300 - 150) / 10.0f; // 50±15
    return 50.0f + (rand() % 300 - 150) / 10.0f; // 使用标准库rand()
    //return 50.0f + (rt_rand()% 300 - 150) / 10.0f; // 使用标准库rand()
}

/* 卡尔曼线程入口 */
static void kalman_thread_entry(void *parameter)
{

    /* 初始化滤波器 (真实值设为50) */
    multi_kalman_init(&sensor_filter, 
                    0.01f, 0.1f, 0.1f, 
                    0.0f, 50.0f);

    while (1) 
    {
        /* 获取传感器数据 */
        float z1 = simulate_sensor1();
        float z2 = simulate_sensor2();

        /* 执行卡尔曼滤波 */
        float result = multi_kalman_update(&sensor_filter, z1, z2);

        /* 输出结果 */
        rt_kprintf("Sensor1: %d.%02d (bias:%d.%02d) | ",
            (int)z1, (int)(fabs(z1 - (int)z1) * 100),
            (int)sensor_filter.bias[0], (int)(fabs(sensor_filter.bias[0] - (int)sensor_filter.bias[0]) * 100));
        rt_kprintf("Sensor2: %d.%02d (bias:%d.%02d)\n",
            (int)z2, (int)(fabs(z2 - (int)z2) * 100),
            (int)sensor_filter.bias[1], (int)(fabs(sensor_filter.bias[1] - (int)sensor_filter.bias[1]) * 100));
        rt_kprintf("=> Kalman Result: %d.%02d\n\n",
            (int)result, (int)(fabs(result - (int)result) * 100));
        /* 500ms 采样间隔 */
        rt_thread_mdelay(500);
    }
}

/* 卡尔曼示例初始化 */
int kalman_sample(void)
{

    srand(rt_tick_get());  // 初始化随机种子
    //rt_srand(rt_tick_get());
    //srand(time(NULL));

    rt_err_t result;

    /* 创建线程 */
    result = rt_thread_init(&thread_k,
                          "thread_k",
                          kalman_thread_entry,
                          RT_NULL,
                          thread_k_stack,
                          sizeof(thread_k_stack),
                          THREAD_PRIORITY,
                          THREAD_TIMESLICE);
    
    if (result == RT_EOK) {
        rt_thread_startup(&thread_k);
        rt_kprintf("多传感器卡尔曼滤波线程启动成功!\n");
    } else {
        rt_kprintf("线程创建失败: 0x%08X\n", result);
        return -RT_ERROR;
    }

    return RT_EOK;
}

/* 导出到命令列表 */
MSH_CMD_EXPORT(kalman_sample, multi-sensor kalman demo);
