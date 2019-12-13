# 系统调用相关代码分析

## 系统调用的初始化

* x86-32位系统：start_kernel --> trap_init --> idt_setup_traps --> 0x80--entry_INT80_32，在5.0内核int0x80对应的中断服务例程是entry_INT80_32，而不是原来的名称system_call了。
* x86-64位系统：start_kernel --> trap_init --> cpu_init --> syscall_init
  * [64位的系统调用中断向量与服务例程绑定](https://github.com/torvalds/linux/blob/c3bfc5dd73c6f519ff0636d4e709515f06edef78/arch/x86/kernel/cpu/common.c#L1670)
```
void syscall_init(void)
{
	wrmsr(MSR_STAR, 0, (__USER32_CS << 16) | __KERNEL_CS);
	wrmsrl(MSR_LSTAR, (unsigned long)entry_SYSCALL_64);
  ...
```
## 系统调用的正常执行

用户态程序发起系统调用，对于x86-64位程序应该是直接跳到entry_SYSCALL_64
  * [64位的系统调用服务例程](https://github.com/torvalds/linux/blob/ab851d49f6bfc781edd8bd44c72ec1e49211670b/arch/x86/entry/entry_64.S#L175)
```
SYM_CODE_START(entry_SYSCALL_64)
...
	/* IRQs are off. */
	movq	%rax, %rdi
	movq	%rsp, %rsi
	call	do_syscall_64		/* returns with IRQs disabled */
```
  * [do_syscall_64](https://github.com/torvalds/linux/blob/ab851d49f6bfc781edd8bd44c72ec1e49211670b/arch/x86/entry/common.c#L282)
```
#ifdef CONFIG_X86_64
__visible void do_syscall_64(unsigned long nr, struct pt_regs *regs)
{
...
	if (likely(nr < NR_syscalls)) {
		nr = array_index_nospec(nr, NR_syscalls);
		regs->ax = sys_call_table[nr](regs);
...
}
#endif
```
