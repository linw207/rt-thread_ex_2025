/**
 * @file   main.c
 * @brief  简易计算器示例程序
 */

#include <rtthread.h>
#include <rthw.h>
#include "calculator.h"
#include <stdint.h>

#define SegDig_ADDR     0x8000103C
#define GPIO_SWs        0x80001400
#define GPIO_LEDs       0x80001404

#define READ_GPIO(dir) (*(volatile unsigned *)dir)
#define WRITE_GPIO(dir, value) { (*(volatile unsigned *)dir) = (value); }

// LED闪烁线程
void thread_led_entry(void *parameter)
{
    rt_uint32_t count = 0;
    while (1)
    {
        if (count % 2 == 0)
        {
            WRITE_GPIO(GPIO_LEDs, 0xAAAA);
        }
        else
        {
            WRITE_GPIO(GPIO_LEDs, 0x5555);
        }
        count++;
        rt_thread_mdelay(500);
    }
}

int main(void)
{
    rt_thread_t thread_led = RT_NULL;
    rt_thread_t calculator_thread = RT_NULL;
    rt_uint32_t sw_v = 0;
    int calculator_running = 0;
    
    rt_kprintf("\n\n=== 简易计算器演示程序 ===\n");
    rt_kprintf("按下 SW8 启动计算器\n");  // 改为SW8
    
    // 创建LED闪烁线程
    thread_led = rt_thread_create("led", thread_led_entry, RT_NULL,
                                 512, 1, 5);
    rt_thread_startup(thread_led);
    
    // 主循环
    while (1)
    {
        rt_uint32_t current_sw = READ_GPIO(GPIO_SWs) >> 16;
        
        // 计算器正在运行时，不处理其他按键
        if (calculator_running && calculator_thread != RT_NULL)
        {
            if (calculator_thread->stat == RT_THREAD_CLOSE)
            {
                calculator_running = 0;
                calculator_thread = RT_NULL;
            }
            else
            {
                // 让出CPU，让计算器线程处理按键
                rt_thread_mdelay(200);
                continue;
            }
        }
        
        // 按键值没变化，继续循环
        if (sw_v == current_sw)
        {
            rt_thread_mdelay(200);
            continue;
        }
        
        // 更新按键值
        sw_v = current_sw;
        
        // 处理按键
        if (sw_v == 0)
        {
            // 按键释放，检查是否需要停止计算器线程
            if (calculator_thread != RT_NULL)
            {
                rt_kprintf("计算器已停止\n");
                rt_thread_suspend(calculator_thread);
                rt_thread_delete(calculator_thread);
                calculator_thread = RT_NULL;
                calculator_running = 0;
            }
        }
        else if (sw_v == 0x80)  // 改为SW8 (值为0x80)
        {
            // SW8: 启动计算器
            if (calculator_thread == RT_NULL)
            {
                rt_kprintf("启动计算器...\n");
                calculator_thread = rt_thread_create("calculator", 
                                                   calculator_sample, 
                                                   RT_NULL,
                                                   1024, 9, 5);
                if (calculator_thread != RT_NULL)
                {
                    rt_thread_startup(calculator_thread);
                    calculator_running = 1;
                    rt_kprintf("计算器已启动！\n");
                    rt_kprintf("使用说明:\n");
                    rt_kprintf("SW1: 设置数字(0-9循环)\n");
                    rt_kprintf("SW2: 确认数字/操作\n");
                    rt_kprintf("SW3: 清除/重置\n");
                }
                else
                {
                    rt_kprintf("计算器启动失败！\n");
                }
            }
        }
        
        rt_thread_mdelay(200);
    }
    
    return 0;
}