# 系统调用相关代码分析

## 系统调用的初始化

* start_kernel --> trap_init --> idt_setup_traps --> 0x80--entry_INT80_32，在5.0内核int0x80对应的中断服务例程是entry_INT80_32，而不是原来的名称system_call了。
