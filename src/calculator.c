#include <rtthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "calculator.h"

#define SegDig_ADDR 0x8000103C
#define GPIO_SWs    0x80001400
#define GPIO_LEDs   0x80001404

#define READ_GPIO(dir) (*(volatile unsigned *)dir)
#define WRITE_GPIO(dir, value) { (*(volatile unsigned *)dir) = (value); }

// 数码管显示函数
static void display_on_seg(int value)
{
    unsigned int seg_val = 0;
    int v = value;
    if (v < 0) {
        // 显示负号 (最高位点亮)
        seg_val |= 0x1000;
        v = -v;  // 取绝对值
    }
    for (int i = 0; i < 4; i++)
    {
        seg_val |= ((v % 10) & 0xF) << (i * 4);
        v /= 10;
    }
    WRITE_GPIO(SegDig_ADDR, seg_val);
}

// 二进制显示函数（在LEDs上显示二进制）
static void display_binary(int value)
{
    // 直接将数值映射到LED，显示二进制形式
    WRITE_GPIO(GPIO_LEDs, (uint32_t)value);
    
    // 打印二进制表示（方便调试）
    rt_kprintf("二进制表示: ");
    for (int i = 15; i >= 0; i--) {
        rt_kprintf("%d", (value & (1 << i)) ? 1 : 0);
        if (i % 4 == 0) rt_kprintf(" ");  // 每4位分组
    }
    rt_kprintf("\n");
}

// 计算器状态机
typedef enum {
    STATE_INPUT_A,     // 输入第一个数字
    STATE_SELECT_OP,   // 选择运算符
    STATE_INPUT_B,     // 输入第二个数字
    STATE_RESULT,      // 显示结果
    STATE_BINARY       // 显示二进制（新增）
} CalcState;

// 运算符
typedef enum {
    OP_ADD,   // 加法
    OP_SUB,   // 减法
    OP_MUL,   // 乘法
    OP_DIV,   // 除法
    OP_BIN,   // 二进制转换（新增）
    OP_NONE   // 无运算
} Operation;

// 操作符对应的符号字符
static const char* op_symbols[] = {"+", "-", "×", "÷", "BIN", ""};

// 计算器线程入口
void calculator_sample(void *parameter)
{
    int a = 0;           // 第一个操作数
    int b = 0;           // 第二个操作数
    int result = 0;      // 计算结果
    CalcState state = STATE_INPUT_A;  // 初始状态
    Operation op = OP_NONE;   // 当前操作符
    int digit = 0;       // 当前输入的数字
    uint32_t old_sw = 0; // 上一次的按键状态
    int binary_mode = 0; // 是否在显示二进制
    int binary_digit = 0;// 二进制显示位置
    
    rt_kprintf("增强型计算器启动\n");
    rt_kprintf("按键说明:\n");
    rt_kprintf("SW1: 设置数字/操作符\n");
    rt_kprintf("SW2: 确认输入\n");
    rt_kprintf("SW3: 清除/重置\n");
    rt_kprintf("SW4: 在二进制模式下切换位显示（新增）\n");
    
    // 初始显示0
    display_on_seg(0);
    
    while (1)
    {
        uint32_t sw = (READ_GPIO(GPIO_SWs) >> 16);
        
        // 按键检测 (只在按键状态变化时处理)
        if (sw != old_sw) {
            if (sw & 0x1) {  // SW1: 设置数字/操作符
                if (state == STATE_INPUT_A || state == STATE_INPUT_B) {
                    // 数字输入模式
                    digit = (digit + 1) % 10;
                    
                    // 显示当前数字
                    display_on_seg(digit);
                    rt_kprintf("数字: %d\n", digit);
                }
                else if (state == STATE_SELECT_OP) {
                    // 操作符选择模式: 循环选择 + - × ÷ BIN
                    op = (op + 1) % 5; // 现在有5个操作符（包括BIN）
                    rt_kprintf("操作符: %s\n", op_symbols[op]);
                    
                    // 在LED上显示当前操作符
                    WRITE_GPIO(GPIO_LEDs, 1 << op);
                }
                else if (state == STATE_BINARY) {
                    // 在二进制模式下，切换不同的二进制位显示
                    binary_digit = (binary_digit + 4) % 16;  // 每次移动4位
                    rt_kprintf("显示二进制位 %d-%d\n", binary_digit+3, binary_digit);
                    
                    // 显示二进制的一部分
                    uint16_t mask = 0xF << binary_digit;
                    uint16_t bits = (result & mask) >> binary_digit;
                    display_on_seg(bits);
                }
            }
            else if (sw & 0x2) {  // SW2: 确认输入
                if (state == STATE_INPUT_A) {
                    // 确认第一个数字，进入选择操作符状态
                    a = digit;
                    digit = 0;
                    state = STATE_SELECT_OP;
                    op = OP_ADD; // 默认加法
                    rt_kprintf("第一个数: %d\n", a);
                    rt_kprintf("请选择操作符: %s\n", op_symbols[op]);
                    
                    // 在LED上显示当前操作符 (1=加)
                    WRITE_GPIO(GPIO_LEDs, 1);
                }
                else if (state == STATE_SELECT_OP) {
                    // 确认操作符
                    if (op == OP_BIN) {
                        // 如果选择了二进制转换，直接进入二进制显示模式
                        result = a;
                        state = STATE_BINARY;
                        binary_digit = 0;
                        rt_kprintf("将 %d 转换为二进制\n", a);
                        display_binary(a);
                        display_on_seg(a & 0xF); // 显示最低4位
                    } else {
                        // 其他操作符，进入输入第二个数字状态
                        state = STATE_INPUT_B;
                        digit = 0;
                        rt_kprintf("操作符已确认: %s\n", op_symbols[op]);
                        rt_kprintf("请输入第二个数字\n");
                        display_on_seg(0);
                    }
                }
                else if (state == STATE_INPUT_B) {
                    // 确认第二个数字，计算结果
                    b = digit;
                    rt_kprintf("第二个数: %d\n", b);
                    
                    // 执行计算
                    switch (op) {
                        case OP_ADD:
                            result = a + b;
                            break;
                        case OP_SUB:
                            result = a - b;
                            break;
                        case OP_MUL:
                            result = a * b;
                            break;
                        case OP_DIV:
                            if (b == 0) {
                                rt_kprintf("错误: 除数不能为0!\n");
                                result = 0;
                            } else {
                                result = a / b;
                            }
                            break;
                        default:
                            result = 0;
                    }
                    
                    rt_kprintf("计算: %d %s %d = %d\n", a, op_symbols[op], b, result);
                    display_on_seg(result);
                    state = STATE_RESULT;
                    
                    // 设置LED显示结果状态
                    WRITE_GPIO(GPIO_LEDs, 0x0100);
                }
                else if (state == STATE_RESULT || state == STATE_BINARY) {
                    // 结果状态下确认，重新开始新的计算
                    state = STATE_INPUT_A;
                    a = 0;
                    b = 0;
                    digit = 0;
                    op = OP_NONE;
                    binary_digit = 0;
                    display_on_seg(0);
                    WRITE_GPIO(GPIO_LEDs, 0);
                    rt_kprintf("开始新的计算\n");
                }
            }
            else if (sw & 0x4) {  // SW3: 清除/重置
                // 无论当前状态如何，都重置计算器
                state = STATE_INPUT_A;
                a = 0;
                b = 0;
                digit = 0;
                op = OP_NONE;
                binary_digit = 0;
                display_on_seg(0);
                WRITE_GPIO(GPIO_LEDs, 0);
                rt_kprintf("计算器已重置\n");
            }
            else if (sw & 0x8) {  // SW4: 在二进制模式下切换LED和数码管显示
                if (state == STATE_BINARY || state == STATE_RESULT) {
                    binary_mode = !binary_mode;
                    if (binary_mode) {
                        display_binary(result);
                        rt_kprintf("切换到LED二进制显示\n");
                    } else {
                        display_on_seg(result);
                        rt_kprintf("切换到数码管十进制显示\n");
                    }
                }
            }
        }
        
        old_sw = sw;
        rt_thread_mdelay(200); // 按键防抖
    }
}