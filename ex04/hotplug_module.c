#include <linux/module.h>
#include <linux/usb.h> // struct usb_device_id
#include <linux/printk.h>
#include "linux/mod_devicetable.h" // MODULE_DEVICE_TABLE
#include "linux/hid.h" // USB macros

const struct usb_device_id my_usb_id[] = {
	{USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID,
						USB_INTERFACE_SUBCLASS_BOOT,
						USB_INTERFACE_PROTOCOL_KEYBOARD)},
	{}

};

MODULE_DEVICE_TABLE(usb, my_usb_id);

static __init int hello(void)
{
	pr_info("Hello world!\n");
	return 0;
}

static __exit void goodbye(void)
{
	pr_info("Cleaning up module.\n");
}

module_init(hello);
module_exit(goodbye);

MODULE_DESCRIPTION("Hello World Module");
MODULE_AUTHOR("cdomet-d");
MODULE_LICENSE("GPL");