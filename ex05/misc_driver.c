#include <linux/module.h>
#include <linux/printk.h>
#include <linux/miscdevice.h>

#define INVALID -1
#define LOGIN_LEN 8

static ssize_t ft_read(struct file *filp, char __user *buf, size_t len, loff_t *off);
static ssize_t ft_write(struct file *file, const char __user *buf, size_t size, loff_t *offset);

static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.read = ft_read,
	.write = ft_write,
};

static struct miscdevice ft_chrdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "fortytwo",
	.fops = &fops,
	.mode = 0666,
};

static ssize_t ft_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
	static const char kbuf[] = "cdomet-d\n";
	return simple_read_from_buffer(buf, LOGIN_LEN, off, kbuf, LOGIN_LEN + 1);
}

static ssize_t ft_write(struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
	char kbuf[LOGIN_LEN + 1];
	size_t len;

	if (size == LOGIN_LEN)
		len = LOGIN_LEN;
	else if (size == LOGIN_LEN + 1)
		len = LOGIN_LEN + 1;
	else
		return INVALID;

	if (copy_from_user(kbuf, buf, len) != 0)
		return INVALID;
	if (strncmp(kbuf, "cdomet-d\n", len) == 0 || strncmp(kbuf, "cdomet-d", len) == 0)
		return len;
	return INVALID;
}

static __init int hello(void)
{
	pr_info("Hello world!\n");
	return misc_register(&ft_chrdev);
}

static __exit void goodbye(void)
{
	pr_info("Cleaning up module.\n");
	misc_deregister(&ft_chrdev);
}

module_init(hello);
module_exit(goodbye);

MODULE_DESCRIPTION("Hello World Module");
MODULE_AUTHOR("cdomet-d");
MODULE_LICENSE("GPL");