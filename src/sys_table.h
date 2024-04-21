#define __in_systable(group) __attribute__((section(".sys_table" group)))

extern unsigned long __in_systable() __aligned(4096) sys_table_ptrs[];
