#include "app.h"

/* 裸机"内核"：SystemInit 已由启动代码在进入 main 前完成（保持 72MHz——
 * UVC 需要 PLL/1.5 = 48MHz USB 时钟，128MHz 超频与 USB 不兼容），
 * main 只负责把控制权交给应用入口 app_main()。 */
int main(void)
{
    app_main(); /* 不返回 */

    while (1)
    {
    }
}
