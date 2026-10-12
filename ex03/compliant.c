#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/slab.h>

static int do_work(int *val)
{
	int x;
	int y = *val;
	int z;

	for (x = 0; x < y; ++x)
		udelay(10);
	/* That was a long sleep, tell userspace about it */
	if (y < 10)
		pr_info("We slept a long time!\n");
	z = x * y;
	return z;
}
static __init int my_init(void)
{
	int x = 10;
	x = do_work(&x);
	return 0;
}
static __exit void my_exit(void)
{
}

module_init(my_init);
module_exit(my_exit);

MODULE_DESCRIPTION("Coding Style Fixes");
MODULE_AUTHOR("cdomet-d");
MODULE_LICENSE("GPL");