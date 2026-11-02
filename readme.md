# Little Pinguin

The genus Eudyptula from Ancient Greek εὖ (eû), meaning "well", δύπτης (dúptes), meaning "diver", and Latin -ula, a diminutive suffix, contains a single extant species of penguin, found in southern Australia, Tasmania, and New Zealand (including the Chatham Islands). It is commonly known as the little penguin, little blue penguin, or, in Australia, fairy penguin. In the language of the Māori people of New Zealand, little penguins are known as kororā.[3]

For many years, a white-flippered form of the little penguin found only in North Canterbury, New Zealand was considered either a separate species, Eudyptula albosignata, or just a subspecies, Eudyptula minor albosignata. Analysis of mtDNA revealed that Eudyptula falls instead into two groups: a western one, found along the southern coast of Australia and the Otago region of New Zealand, and another found in the rest of New Zealand.[4] E. novaehollandiae probably arrived in New Zealand from Australia less than 500 years ago, following the local extinction of E. minor in Otago.[5]
[The Eudyptula Challenge](http://eudyptula-challenge.org/)

- [Little Pinguin](#little-pinguin)
	- [ex00 - Building the kernel](#ex00---building-the-kernel)
	- [ex01 - Hello World Module](#ex01---hello-world-module)
		- [Sources](#sources)
	- [ex02 - Patching](#ex02---patching)
	- [ex03 - Coding Style](#ex03---coding-style)
	- [ex04 - Device Linked Module](#ex04---device-linked-module)
		- [Simulating a keyboard hotplug in Qemu](#simulating-a-keyboard-hotplug-in-qemu)
		- [Autoloading the module](#autoloading-the-module)
		- [Udev rules](#udev-rules)
		- [Device table](#device-table)
			- [The `bDeviceClass`](#the-bdeviceclass)
			- [Single Function and Multi-Function USB devices](#single-function-and-multi-function-usb-devices)
		- [So, `USB_DEVICE_INFO` or `USB_INTERFACE_INFO` ?](#so-usb_device_info-or-usb_interface_info-)
	- [ex05 - Misc Character Driver](#ex05---misc-character-driver)
	- [General applications sources](#general-applications-sources)

## ex00 - Building the kernel

The hardest thing about this part was fixing the disk corruption that happened on my LFS distributions, though I guess I learned stuff `(:`

Getting the kernel from [git.kernel.org](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/) is quite straightforward, though it requires installing git on your LFS distribution - which I hadn't done before, but is easily managed by following the appropriate [BLFS page](https://linuxfromscratch.org/blfs/view/stable/general/git.html) (make sure to use the appropriate version of BLFS, the one that matches your LFS).

Then, you may either go through the manual configuration of the kernel with `make menuconfig`  or `make defconfig`. However, as I did not quite fancy doing that, I recovered my LFS `.config` and used `make olddefconfig`, which :

> The make target `olddefconfig` (and the yes ""   used when utilizing localmodconfig) will set any undefined build options to their default value. This among others will disable many kernel features that were introduced after your base kernel was released. [[doc.kernel.org](https://docs.kernel.org/admin-guide/quickly-build-trimmed-linux.html), 08.10.2026]

I chose to not use `localmodconfig` as this [documentation](https://docs.kernel.org/admin-guide/quickly-build-trimmed-linux.html) warns that only the kernel modules related to actions already performed wouldn't be autoloaded when said action is performed after kernel compilation. As I'm not quite sure how much stuff I'll do with my distribution, I chose to remain as I was ; you do what you think is best for your usage.

![However](.assets/cependant-jdg.gif)

Since we will be developing modules later on, I recommend you enrich your configuration file with debug options.

Rename your `.config` file to avoid overwriting it, then use `make menuconfig` to get more options. This will create a new `.config` file, which we will later merge with the one we got from `make olddefconfig`.

[The Linux Kernel Programming Guide](https://sysprog21.github.io/lkmpg/) recommends having the following options enabled:

- **CONFIG_DEBUG_INFO**: include debug symbols so gdb, addr2line, and objdump can map crash addresses to source lines.
- **CONFIG_DYNAMIC_DEBUG**: enable runtime-switchable pr_debug() statements (see Item 2).
- **CONFIG_KASAN (Kernel Address Sanitizer)**: instruments memory accesses to detect out-of-bounds, use-after-free, and other memory errors at the cost of roughly 2× memory usage and some CPU overhead.
- **CONFIG_LOCKDEP (lock dependency checker)**: detects potential deadlocks (lock order inversions, sleeping under spinlocks, wrong lock type for context) at runtime, before they actually hang the system.
- **CONFIG_DEBUG_ATOMIC_SLEEP**: flags attempts to sleep in atomic context, catching the most common spinlock misuse.
- **CONFIG_MODULE_FORCE_UNLOAD**: allows rmmod -f as a last resort during development, but use it with care because force-unloading can hide lifetime bugs and leave the kernel in an inconsistent state.

Using `menuconfig`, you can use `/` and then type the option name to find what toggle manages it. Save that new configuration - I'll call that file `.config` in my example - then use

```sh
./scripts/kconfig/merge_config.sh config.base config .config
```

Once you are satisfied with the config, you can build the kernel, and all that jazz, and you're done ! Don't forget to do `make modules_install` to ensure you have the correct libraries for the next steps of the challenge.

(personally, [I followed the LFS step of the kernel build](https://www.linuxfromscratch.org/lfs/view/development/chapter10/kernel.html) - it helps cover the important steps of building the kernel. Be mindful of using the latest version, though, as we are working with the "current" Linux source).

- [**Displaying kernel messages**](https://linuxvox.com/blog/kern-log-linux/)
- [**Quick Linux build**](https://docs.kernel.org/admin-guide/quickly-build-trimmed-linux.html)
- [**Merging `.config` files**](https://wiki.gentoo.org/wiki/User:Egberts/Drafts/Gentoo_Kernel_Configuration_Guide)

## ex01 - Hello World Module

That one is pretty interesting ; we get to discover `kbuild`, which is essentially `make` with conventions. It relies on `kconfig`, which is what helps the user create the initial `.config` file.

The Kernel build system is, expectedly, quite heavy. The sources get recursively compiled based on the configuration files, with the top-level Makefile descending into children directories to call on the resident Makefile.

Here, what we care mostly about is the way the build system compiles modules. [The Linux Kernel Programming Guide](https://sysprog21.github.io/lkmpg/) provides an example Makefile, which I've tweaked to compile my own module:

```make
# The value of obj-m specifies our source file, and the final name of our kernel will be <obj-m_value>.ko
obj-m += main.o # `obj-m` allows kbuild to know we're creating a module.

# If the module source code is in multiple files, one must list the sources as goals like that: 
# <modulename>-y = <src1>.o, <src2.o> ...

PWD := $(CURDIR)

all:
    $(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
    $(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
```

It's not very detailed, but it lets us understand that our job as kernel module developers is not to write our own compilation rules ; instead, we list our sources (with `obj-m`), then we simply call the top-level Kernel Makefile `modules` rule, which will do all the compilation work, thus avoiding flag mismatches and other horrors.
****
Here, we are creating an out-of-tree (or external) module, which means the top-level Makefile needs our working directory to output the final `.ko` file here instead of mixing it up with the kernel tree.

### Sources

- [**Definition and Examples**](https://linux-kernel-labs.github.io/refs/heads/master/labs/kernel_modules.html)
- [**In-Tree vs Out-Of-Tree Kernel Modules**](https://medium.com/@adityapatnaik27/linux-kernel-module-in-tree-vs-out-of-tree-build-77596fc35891)
- [**Kbuild documentation**](https://docs.kernel.org/kbuild/index.html)
- [**More information about the kernel build in general**](https://www.kernel.org/doc/html/latest/kbuild/makefiles.html)
- [**More information and complicated kbuild/Makefiles examples**](https://www.kernel.org/doc/html/latest/kbuild/modules.html), including multi-files build.

## ex02 - Patching

This is not complicated, especially if using the `git format-patch` command.

To do so, apply your changes and commit them (you don't have to push). You can then use `git format-patch` with the appropriate parameters to automatically generate a `.patch` file that fits the Kernel Patch Standards.

- [**Submitting patches**](https://www.kernel.org/doc/html/latest/process/submitting-patches.html)
- [**Using git format-patch**](https://git-scm.com/docs/git-format-patch)
-

## ex03 - Coding Style

- [**Linux Coding Style**](https://www.kernel.org/doc/html/latest/process/coding-style.html)

## ex04 - Device Linked Module

You can peruse my understanding of the recommended reading at [LDD3_device_model.md](./LDD3_device_model.md).

I found it a pretty irrelevant reading recommendation ; it mostly focuses on the inner workings of `sysfs`, which helps only very marginally to write device drivers.

### Simulating a keyboard hotplug in Qemu

See [The Qemu Sidequest, part 2](./qemu-sidequest.md)

### Autoloading the module

Now, as the subject hints, we have two ways to go about making the module autoload.

- A rules file, appropriate for your system.
- Your updated code.

- [udevadm](https://www.man7.org/linux/man-pages/man8/udevadm.8.html)

### Udev rules

- [Write Udev Rules](https://linuxconfig.org/tutorial-on-how-to-write-basic-udev-rules-in-linux)

### Device table

The [Driver API documentation](https://docs.kernel.org/driver-api/usb/writing_usb_driver.html) of the Linux Kernel informs us of the following:

> To enable the linux-hotplug system to load the driver automatically when the device is plugged in, you need to create a MODULE_DEVICE_TABLE. The following code tells the hotplug scripts that this module supports a single device with a specific vendor and product ID:

```c
/* table of devices that work with this driver */
static struct usb_device_id skel_table [] = {
        { USB_DEVICE(USB_SKEL_VENDOR_ID, USB_SKEL_PRODUCT_ID) },
        { }                      /* Terminating entry */
};
MODULE_DEVICE_TABLE (usb, skel_table);
```

> There are other macros that can be used in describing a struct `usb_device_id` for drivers that support a whole class of USB drivers. See usb.h for more information on this.

`usb_device_id` allows us to specify information describing one or several USB devices. To explain how, we have to go on a small(ish) sidequest to the USB protocol specification.

Our example snippet uses `USB_SKEL_VENDOR_ID` and `USB_SKEL_PRODUCT_ID`, which need to contain the values of `idVendor` and `idProduct` (which are found in the Device Descriptor, explained a bit later in [this section](#the-bdeviceclass)). Setting our USB module to be recognized by its vendor and product ID is a bit restrictive for our case, since we just want the module to load whenever we plug in any keyboard.

So we must look for alternatives. The [USB header documentation](https://docs.kernel.org/driver-api/usb/usb.html#usb-header) tells us that:

> **What Makes Good `usb_device_id` Tables:**

> The most specific match specifiers use device descriptor data. These are commonly used with product-specific matches; the `USB_DEVICE` macro lets you provide vendor and product IDs, and you can also match against ranges of product revisions. These are widely used for devices with application or vendor specific `bDeviceClass` values.

> Matches based on device class/subclass/protocol specifications are slightly more general; use the `USB_DEVICE_INFO` macro, or its siblings. These are used with **single-function devices** where `bDeviceClass` doesn’t specify that each interface has its own class.

> Matches based on interface class/subclass/protocol are the most general; they let drivers bind to any interface on a **multiple-function device**. Use the `USB_INTERFACE_INFO` macro, or its siblings, to match class-per-interface style devices (as recorded in bInterfaceClass).

So it appears we have several options, from most to least restrictive:

- using vendor and product IDs,
- using USB_DEVICE_INFO,
- using USB_INTERFACE_INFO.

Since we already excluded the vendor and product IDs as too restrictive, we still have to choose between using `USB_DEVICE_INFO` or `USB_INTERFACE_INFO`. But before going into that, we are going to backtrack a little to define what we are even talking about.

#### The `bDeviceClass`

When a USB device is plugged in, it must be configured. That configuration is a negotiation that happens between the host and the device. The first structure that is sent is the **device descriptor**, which is a structure described in section 9.6.1 of the [Universal Serial Bus 3.2 Specification](./.assets/USB_3.2_revision_1-1.pdf), in table 9-11 (pages 380 - 381 of the PDF, pages 349 - 350 by the document's own page numbers)[^3]. You can also [view the table here](./usb_device_fields.md).

The list of values that `bDeviceClass` can take is specified on the USB-IF website; [here is the list](https://www.usb.org/defined-class-codes). You can see that the "Descriptor usage" column can be "Device", "Interface", or both. Keep that in mind, it will become relevant in a minute.

#### Single Function and Multi-Function USB devices

USB devices can have a single function (like a USB hub) or several (like a webcam that has a microphone). Keyboards, deceptively, may have several functionalities (for instance, a laptop keyboard with a trackpad). Multi-function devices (or [composite devices](https://laptopjudge.com/what-is-a-usb-composite-device/)) pack several functionalities into one; they do this by exposing *interfaces* to which the kernel binds individual drivers. A more in-depth explanation of how that works is available in the [USB device layout](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/usb-device-layout) documentation.

### So, `USB_DEVICE_INFO` or `USB_INTERFACE_INFO` ?

`USB_DEVICE_INFO` and `USB_INTERFACE_INFO` are macros used to tell the module where to check whether the device that just connected matches the type of device the driver was written for. The first tells the module to look in `bDeviceClass`, and the second implies that the module expects the device it will control to have several interfaces, and to connect to one of them. To decide which to use, the developer must figure out which device class the device they are writing a driver for belongs to. If the device class's "Descriptor usage" column states "Device", it is safe to use `USB_DEVICE_INFO`; if it is "Interface", one should use `USB_INTERFACE_INFO`; if it is both, the most restrictive identifier must be chosen.

## ex05 - Misc Character Driver

## General applications sources

- [Message logging with printk](https://www.kernel.org/doc/html/latest/core-api/printk-basics.html)
- [The always useful list of GNU warning options](https://gcc.gnu.org/onlinedocs/gcc-15.2.0/gcc/Warning-Options.html#index-Wno-old-style-declaration)
