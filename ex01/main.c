#include <linux/module.h>
#include <linux/printk.h>

static inline int hello(void) 
{
	pr_info("Hello world!\n");
	return 0;
}

static inline void goodbye(void) 
{
	pr_info("Cleaning up module.\n");
}

module_init(hello);
module_exit(goodbye);

MODULE_DESCRIPTION("Hello World Module");
MODULE_AUTHOR("cdomet-d");
MODULE_LICENSE("Really ?");