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
		- [Device table](#device-table)
			- [Single-function and multi-function USB devices](#single-function-and-multi-function-usb-devices)
			- [The descriptor triple: device vs interface](#the-descriptor-triple-device-vs-interface)
			- [So, `USB_DEVICE_INFO` or `USB_INTERFACE_INFO`?](#so-usb_device_info-or-usb_interface_info)
			- [Building the descriptor triple](#building-the-descriptor-triple)
		- [Udev rules](#udev-rules)
			- [How a rule works](#how-a-rule-works)
			- [The rule](#the-rule)
			- [Do I even need this?](#do-i-even-need-this)
			- [Testing and debugging](#testing-and-debugging)
			- [A word about unloading](#a-word-about-unloading)
			- [Links](#links)
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

I found it a pretty irrelevant reading recommendation; it mostly focuses on the inner workings of `sysfs`, which helps only very marginally to write device drivers.

### Simulating a keyboard hotplug in Qemu

See [The Qemu Sidequest, part 2](./qemu-sidequest.md).

### Autoloading the module

Now, as the subject hints, we have two ways to go about making the module autoload:

- your updated code.
- a rules file, appropriate for your system;

Autoloading, however, is the easy part. What's much harder is determining how to trigger it; to do that, we'll have to sidequest (again).

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

`usb_device_id` allows us to specify information describing one or several USB devices. To explain how, we have to go on a small(ish) (I lied, [this is like Ranni's quest](https://eldenring.wiki.fextralife.com/Ranni_the_Witch)) sidequest to the USB protocol specification.

**Warning:** As always with my READMEs, they are written primarily for my own understanding and are not to be taken at face value. Chapter 9 of the USB 3.2 specification explains everything both better and more accurately, so I recommend you go read it. I tend to ramble sometimes.

Our example snippet uses `USB_SKEL_VENDOR_ID` and `USB_SKEL_PRODUCT_ID`, which need to contain the values of `idVendor` and `idProduct` (which are found in the device descriptor, explained a bit later in [this section](#the-descriptor-triple-device-vs-interface)). Setting our USB module to be recognized by its vendor and product ID is a bit restrictive for our case, since we just want the module to load whenever we plug in any keyboard.

So we must look for alternatives. The [USB header documentation](https://docs.kernel.org/driver-api/usb/usb.html#usb-header) tells us that:

> **What Makes Good `usb_device_id` Tables:**

> The most specific match specifiers use device descriptor data. These are commonly used with product-specific matches; the `USB_DEVICE` macro lets you provide vendor and product IDs, and you can also match against ranges of product revisions. These are widely used for devices with application or vendor specific `bDeviceClass` values.

> Matches based on device class/subclass/protocol specifications are slightly more general; use the `USB_DEVICE_INFO` macro, or its siblings. These are used with **single-function devices** where `bDeviceClass` doesn’t specify that each interface has its own class.

> Matches based on interface class/subclass/protocol are the most general; they let drivers bind to any interface on a **multiple-function device**. Use the `USB_INTERFACE_INFO` macro, or its siblings, to match class-per-interface style devices (as recorded in bInterfaceClass).

So it appears we have several options, from most to least restrictive:

- using vendor and product IDs,
- using `USB_DEVICE_INFO`,
- using `USB_INTERFACE_INFO`.

Since we already excluded the vendor and product IDs as too restrictive, we still have to choose between `USB_DEVICE_INFO` and `USB_INTERFACE_INFO`. But before going into that, we are going to backtrack a little to define what we are even talking about.

#### Single-function and multi-function USB devices

The USB header documentation mentions single-function vs multi-function devices, and says that their handling differs. Indeed, USB devices can have a single function (like a USB hub) or several (like a webcam that has a microphone). Keyboards, deceptively, may have several functionalities (for instance, a laptop keyboard with a trackpad). Each function of a USB device is described by an *interface*; a more in-depth explanation of how that works is available in the [USB device layout](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/usb-device-layout) documentation.

That distinction matters because of where a device chooses to describe itself. Let's look at what the host actually receives when the device is plugged in.

#### The descriptor triple: device vs interface

When a USB device is plugged in, it must be configured. [That configuration is a conversation that happens between the host and the device at plug-in time](./usb_config_negociation.md). In the course of that negotiation, the device sends information about itself in different structures called descriptors.

Two of these descriptors carry a **class/subclass/protocol triple**:

- the **device descriptor**: `bDeviceClass`, `bDeviceSubClass`, `bDeviceProtocol`;
- each **interface descriptor**: `bInterfaceClass`, `bInterfaceSubClass`, `bInterfaceProtocol`.

Which of the two holds the meaningful values depends on the class: if `bDeviceClass` is `00h`, the device triple says nothing and the real information is in the interface triples.

Once the device is enumerated, the kernel's USB core registers the device and each of its interfaces, and emits an event for each one. That event carries a `MODALIAS` string encoding the vendor/product ID and both triples. `udev` passes it to `modprobe`, which checks it against the aliases generated from every installed module's `MODULE_DEVICE_TABLE`. If one of the module's entries matches, the module is loaded.

An entry does not need to match every field. The macro used to build it decides which fields are compared: `USB_DEVICE` compares the vendor/product ID, `USB_DEVICE_INFO` compares the device triple, and `USB_INTERFACE_INFO` compares the interface triple.

#### So, `USB_DEVICE_INFO` or `USB_INTERFACE_INFO`?

The USB header documentation explained that `USB_DEVICE_INFO` is used for *single-function* devices, and `USB_INTERFACE_INFO` for *multi-function* ones. We should be more specific than that, because some single-function devices declare their class through an interface. To explain this better, let's rewind to the [defined class codes](https://www.usb.org/defined-class-codes) we mentioned earlier.

Fundamentally, device classes need `USB_DEVICE_INFO`, and interface classes need `USB_INTERFACE_INFO`:

| Base Class | Descriptor Usage | Description                                        | Location           | Macro                |
| ---------- | ---------------- | -------------------------------------------------- | ------------------ | -------------------- |
| 00h        | Device           | Use class information in the Interface Descriptors | `bDeviceClass`     | `USB_INTERFACE_INFO` |
| 01h        | Interface        | Audio                                              | `bInterfaceClass`  | `USB_INTERFACE_INFO` |
| 02h        | Both             | Communications and CDC Control                     | Hardware dependent | -                    |
| 03h        | Interface        | HID (Human Interface Device)                       | `bInterfaceClass`  | `USB_INTERFACE_INFO` |
| 05h        | Interface        | Physical                                           | `bInterfaceClass`  | `USB_INTERFACE_INFO` |
| 06h        | Interface        | Image                                              | `bInterfaceClass`  | `USB_INTERFACE_INFO` |
| 07h        | Interface        | Printer                                            | `bInterfaceClass`  | `USB_INTERFACE_INFO` |
| 08h        | Interface        | Mass Storage                                       | `bInterfaceClass`  | `USB_INTERFACE_INFO` |
| 09h        | Device           | Hub                                                | `bDeviceClass`     | `USB_DEVICE_INFO`    |
| 0Ah        | Interface        | CDC-Data                                           | `bInterfaceClass`  | `USB_INTERFACE_INFO` |
| 0Bh        | Interface        | Smart Card                                         | `bInterfaceClass`  | `USB_INTERFACE_INFO` |
| 0Dh        | Interface        | Content Security                                   | `bInterfaceClass`  | `USB_INTERFACE_INFO` |
| 0Eh        | Interface        | Video                                              | `bInterfaceClass`  | `USB_INTERFACE_INFO` |
| 0Fh        | Interface        | Personal Healthcare                                | `bInterfaceClass`  | `USB_INTERFACE_INFO` |
| 10h        | Interface        | Audio/Video Devices                                | `bInterfaceClass`  | `USB_INTERFACE_INFO` |
| 11h        | Device           | Billboard Device Class                             | `bDeviceClass`     | `USB_DEVICE_INFO`    |
| 12h        | Interface        | USB Type-C Bridge Class                            | `bInterfaceClass`  | `USB_INTERFACE_INFO` |
| 13h        | Interface        | USB Bulk Display Protocol Device Class             | `bInterfaceClass`  | `USB_INTERFACE_INFO` |
| 14h        | Interface        | MCTP over USB Protocol Endpoint Device Class       | `bInterfaceClass`  | `USB_INTERFACE_INFO` |
| 3Ch        | Interface        | I3C Device Class                                   | `bInterfaceClass`  | `USB_INTERFACE_INFO` |
| DCh        | Both             | Diagnostic Device                                  | Hardware dependent | -                    |
| E0h        | Interface        | Wireless Controller                                | `bInterfaceClass`  | `USB_INTERFACE_INFO` |
| EFh        | Both             | Miscellaneous                                      | Hardware dependent | -                    |
| FEh        | Interface        | Application Specific                               | `bInterfaceClass`  | `USB_INTERFACE_INFO` |
| FFh        | Both             | Vendor Specific                                    | Vendor dependent   | -                    |

The "Both" classes can't be given a single answer: it depends on what the hardware reports, which you can check with `lsusb -v`.

#### Building the descriptor triple

That's all well and good, but the class is only the first part of the triple needed to identify a device. Thankfully, no guesswork is involved here: one must simply determine the appropriate subclass and protocol for their device. Aside from the links below, the class headers in `/usr/include/linux` contain the macros for subclasses and protocols.

For instance, for our keyboard, all the relevant information can be found in `/usr/include/linux/hid.h`:

```c
#define USB_INTERFACE_CLASS_HID		3
#define USB_INTERFACE_SUBCLASS_BOOT	1
#define USB_INTERFACE_PROTOCOL_KEYBOARD	1
#define USB_INTERFACE_PROTOCOL_MOUSE	2
```

HID defines a single interface class and a single subclass, and we do have a keyboard and not a mouse, so our descriptor triple will be:

```c
	USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID,
		USB_INTERFACE_SUBCLASS_BOOT,
		USB_INTERFACE_PROTOCOL_KEYBOARD
	)
```

Some useful links:

- [Find a USB device with its vendor/product ID info](https://rxtx.org/)
- [List of USB Class/Subclass/Protocols](https://usb-ids.gowdy.us/read/UC)

### Udev rules

#### How a rule works

Udev rules live in `/etc/udev/rules.d/` (the ones shipped by your distribution are in `/usr/lib/udev/rules.d/`, and you shouldn't edit those). Files are read in lexical order, which is why they're prefixed with a number, and they must end in `.rules`.

Each line is a list of comma-separated conditions (`==`) followed by assignments (`=` or `+=`). When a kernel event matches every condition, the assignments are applied. For us, the interesting assignment is `RUN+=`, which executes a program.

Remember that the kernel emits one event for the device and one for *each interface*. Our triple is an interface triple (HID, boot, keyboard), so the rule has to match the interface event, not the device one. The `DEVTYPE` property is what tells them apart: it is `usb_device` for the former and `usb_interface` for the latter.

#### The rule

Interface attributes show up in sysfs as two-digit hexadecimal strings, so HID's class `3` is `"03"`:

```udev
# /etc/udev/rules.d/99-usb-keyboard.rules
ACTION=="add", SUBSYSTEM=="usb", ENV{DEVTYPE}=="usb_interface", \
    ATTR{bInterfaceClass}=="03", \
    ATTR{bInterfaceSubClass}=="01", \
    ATTR{bInterfaceProtocol}=="01", \
    RUN+="/sbin/modprobe my_module"
```

A few remarks:

- The three `ATTR` lines are the same triple we built in the previous section, just written in another syntax. If you used `USB_INTERFACE_INFO(3, 1, 1)` in your module, you should match the same values here.
- `RUN` needs an absolute path to the program. Check where `modprobe` lives on your system with `command -v modprobe`.
- `my_module` is the name of your module, not its file name. For `modprobe` to find it, it must be installed under `/lib/modules/$(uname -r)/` and indexed with `depmod`. Otherwise, use `insmod` with the full path to the `.ko` instead.
- Long lines can be split with a trailing `\`.

#### Do I even need this?

Not necessarily, and that's the point of this exercise's two options. If your module declares a `MODULE_DEVICE_TABLE`, its aliases are generated by `depmod`, and the default rules shipped with udev already run `modprobe` on the `MODALIAS` of every event. The module will be loaded without any custom rule. A rules file is what you write for the opposite case: a module with *no* device table, that someone has to load by hand. This is also why doing both at once is harmless but redundant.

#### Testing and debugging

Don't wait for a hotplug to find out your rule has a typo:

- Look at the real attribute values of an interface with `udevadm info -a -p /sys/bus/usb/devices/<interface>` (something like `1-1:1.0`; `ls /sys/bus/usb/devices/` will show you the names). This prints the whole chain of attributes in rule syntax, so you can copy from it.
- Check that your rule parses and see what it would do, without plugging anything: `udevadm test /sys/bus/usb/devices/<interface>`.
- Watch events live with `udevadm monitor --property` while you plug the keyboard in (or hotplug it in Qemu, see [The Qemu Sidequest, part 2](./qemu-sidequest.md)). You'll see the `DEVTYPE`, `MODALIAS` and the other properties we've been talking about.
- After editing a rule, reload with `udevadm control --reload`, then replay the events with `udevadm trigger --action=add --subsystem-match=usb`.
- Confirm that the module was loaded with `lsmod` and `dmesg`.

#### A word about unloading

You may be tempted to add a matching rule on `ACTION=="remove"` that calls `modprobe -r`. Be careful: on removal, the sysfs attributes are already gone, so `ATTR{...}` conditions will never match. You'd have to match on something that survives, such as `ENV{PRODUCT}` or `ENV{INTERFACE}`, and unloading a module that may still be used by another keyboard is rarely what you want anyway.

#### Links

- [Write Udev Rules](https://linuxconfig.org/tutorial-on-how-to-write-basic-udev-rules-in-linux)
- [udevadm](https://www.man7.org/linux/man-pages/man8/udevadm.8.html)

## ex05 - Misc Character Driver

- [Block VS Character drivers](https://linux-kernel-labs.github.io/refs/heads/master/labs/device_drivers.html)

`int alloc_chrdev_region(dev_t *dev, unsigned baseminor, unsigned count, const char *name)`

Parameters

- `dev_t *dev:` output parameter for first assigned number 
- `unsigned baseminor`: first of the requested range of minor numbers
- `unsigned count` the number of minor numbers required
- `const char *name` the name of the associated device or driver

Description

Allocates a range of char device numbers. The major number will be chosen dynamically, and returned (along with the first minor number) in dev. Returns zero or a negative error code.

## General applications sources

- [Message logging with printk](https://www.kernel.org/doc/html/latest/core-api/printk-basics.html)
- [The always useful list of GNU warning options](https://gcc.gnu.org/onlinedocs/gcc-15.2.0/gcc/Warning-Options.html#index-Wno-old-style-declaration)
