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
  * [32位的系统调用服务程序](https://github.com/mengning/linux/blob/master/arch/x86/entry/entry_32.S#L989)
```
ENTRY(entry_INT80_32)
	ASM_CLAC
	pushl	%eax			/* pt_regs->orig_ax */

	SAVE_ALL pt_regs_ax=$-ENOSYS switch_stacks=1	/* save rest */

	/*
	 * User mode is traced as though IRQs are on, and the interrupt gate
	 * turned them off.
	 */
	TRACE_IRQS_OFF

	movl	%esp, %eax
	call	do_int80_syscall_32
	...
```
     * [do_int80_syscall_32](https://github.com/torvalds/linux/blob/ab851d49f6bfc781edd8bd44c72ec1e49211670b/arch/x86/entry/common.c#L345)
```
static __always_inline void do_syscall_32_irqs_on(struct pt_regs *regs)
{
...
#ifdef CONFIG_IA32_EMULATION
		regs->ax = ia32_sys_call_table[nr](regs);
#else
		/*
		 * It's possible that a 32-bit syscall implementation
		 * takes a 64-bit parameter but nonetheless assumes that
		 * the high bits are zero.  Make sure we zero-extend all
		 * of the args.
		 */
		regs->ax = ia32_sys_call_table[nr](
			(unsigned int)regs->bx, (unsigned int)regs->cx,
			(unsigned int)regs->dx, (unsigned int)regs->si,
			(unsigned int)regs->di, (unsigned int)regs->bp);
#endif /* CONFIG_IA32_EMULATION */
	}

	syscall_return_slowpath(regs);
}

/* Handles int $0x80 */
__visible void do_int80_syscall_32(struct pt_regs *regs)
{
	enter_from_user_mode();
	local_irq_enable();
	do_syscall_32_irqs_on(regs);
}
```
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
## 系统调用表的初始化

ia32_sys_call_table 和 sys_call_table 数组都是由如下目录下的代码初始化的。
https://github.com/mengning/linux/tree/master/arch/x86/entry/syscalls
