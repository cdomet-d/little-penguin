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
		- [The device model](#the-device-model)
			- [kobject](#kobject)
			- [ksets](#ksets)
			- [Subsystems](#subsystems)
		- [Sysfs Operations](#sysfs-operations)
		- [Simulating a keyboard hotplug in QEMU](#simulating-a-keyboard-hotplug-in-qemu)
	- [General applications sources](#general-applications-sources)

## ex00 - Building the kernel

The hardest thing about this part was fixing the disk corruption that happened on my LFS distributions, though I guess I learned stuff `(:`

Getting the kernel from [git.kernel.org](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/) is quite straightforward, though it requires installing git on your LFS distribution - which I didn't do before, but is easily managed by following the appropriate [BLFS page](https://linuxfromscratch.org/blfs/view/stable/general/git.html) (make sure to use the approriate version of BLFS, the one that matches your LFS).

Then, you may either go through the manual configuration of the kernel with `make menuconfig`  or `make defconfig`. However, as I did not quite fancy doing that, I recovered my LFS `.config` and used `make olddefconfig`, which :

> The make target `olddefconfig` (and the yes "" | used when utilizing localmodconfig) will set any undefined build options to their default value. This among others will disable many kernel features that were introduced after your base kernel was released. [[doc.kernel.org](https://docs.kernel.org/admin-guide/quickly-build-trimmed-linux.html), 08.10.2026]

I chose to not use `localmodconfig` as this [documentation](https://docs.kernel.org/admin-guide/quickly-build-trimmed-linux.html) warns that only the kernel modules related to actions already performed wouldn't be autoloaded when said action is perfomed after kernel compilation. As I'm not quite sure how much stuff I'll do with my distribution, I chose to remain as I was ; you do what you think is best for your usage.

![However](.assets/cependant-jdg.gif)

Since we will be developpig modules later on, I recommend you enrich your configuration file with debug options.

Rename your `.congif` file to avoid overwritting it, then use `make menuconfig` to more option. This will create a new `.config` file, which we will later merge with the one we got from `make olddefconfig`.

[The Linux Kernel Programming Guide](https://sysprog21.github.io/lkmpg/) recommends having the following options enabled:

- **CONFIG_DEBUG_INFO**: include debug symbols so gdb, addr2line, and objdump can map crash addresses to source lines.
- **CONFIG_DYNAMIC_DEBUG**: enable runtime-switchable pr_debug() statements (see Item 2).
- **CONFIG_KASAN (Kernel Address Sanitizer)**: instruments memory accesses to detect out-of-bounds, use-after-free, and other memory errors at the cost of roughly 2× memory usage and some CPU overhead.
- **CONFIG_LOCKDEP (lock dependency checker)**: detects potential deadlocks (lock order inversions, sleeping under spinlocks, wrong lock type for context) at runtime, before they actually hang the system.
- **CONFIG_DEBUG_ATOMIC_SLEEP**: flags attempts to sleep in atomic context, catching the most common spinlock misuse.
- **CONFIG_MODULE_FORCE_UNLOAD**: allows rmmod -f as a last resort during development, but use it with care because force-unloading can hide lifetime bugs and leave the kernel in an inconsistent state.

Using `menuconfig`, you can use `/` and then type the option name to find what toggles manages it. Save that new configuration - I'll call that file `.config` in my example - then use

```sh
./scripts/kconfig/merge_config.sh config.base config .config
```

Once you are satisfied with the config, you can build the kernel, and all that jazz, and you're done ! Don't forget to do `make modules_install` to ensure you have the correct libraries for the next steps of the challenge.

(personnaly, [I followed the LFS step of the kernel build](https://www.linuxfromscratch.org/lfs/view/development/chapter10/kernel.html) - it helps covering the important steps of building the kernel. Be mindful of using the latest version, though, as we are working witht the "current" Linux source).

- [**Displaying kernel messages**](https://linuxvox.com/blog/kern-log-linux/)
- [**Quick Linux build**](https://docs.kernel.org/admin-guide/quickly-build-trimmed-linux.html)
- [**Merging `.config` files**](https://wiki.gentoo.org/wiki/User:Egberts/Drafts/Gentoo_Kernel_Configuration_Guide)

## ex01 - Hello World Module

That one is pretty interresting ; we get to discover `kbuild`, which is essentially `make` with conventions. It relies on `kconfig`, which is what helps the user create the initial `.config` file.

The Kernel build system is, expectedly, quite heavy. The sources get recursively compiled based on the configuration files, with the top-level Makefile descending into children directories to call on the resident Makefile.

Here, we care mostly about is the way the build system compiles modules. [The Linux Kernel Programming Guide](https://sysprog21.github.io/lkmpg/) provide an example Makefile, which I've tweaked to compile my own module:

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

It's not very detailed, but it lets us understand that our job as kernel module developer is not to write our own compilation rules ; instead, we list our sources (with `obj-m`), then we simply call the top-level Kernel Makefile `modules` rule, which will do all the compilation work, thus avoiding flag mismatch and other horrors.
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

[**Virtual Filesystem (VFS)**](https://en.wikipedia.org/wiki/Virtual_file_system): an abstraction layer sitting in between userspace and the filesystems implementation. When a syscall originates from the userspace (`open`, `read`, `write`...), the call transits via the VFS. This is the *strict* name of the kernel's abstraction layer that enables actions on the filesystems.

[**Pseudo Filesystem**](https://en.wikipedia.org/wiki/Synthetic_file_system): a filesystem *implementation* that exposes kernel state or lives in memory. It is rules by the VFS, but doesn't rely on a physical device. `/sys` and `/proc` are pseudo filesystems: even if they present files to the users, most live in memory and their contents are generated on demand by the kernel. For instance, the directories in `/proc` representing the running processes come and go as the processes live and die.

[**sysfs**](https://en.wikipedia.org/wiki/Sysfs): a pseudo filesystem that exposes the kernel's devide model to user space in a hierarchy of files.

### The device model

- [Recommended reading: Ch. 14 of *Linux Device Drivers, Third Edition*](https://static.lwn.net/images/pdf/LDD3/ch14.pdf)
- [*Linux Device Drivers, Third Edition*, full book](https://lwn.net/Kernel/LDD3/)

**WARNING:** this is my understanding from the LDD3. It might be wrong, or outdated ; I mostly write those as a way to understand things better. **Feel free to create an issue or fork and update :)**

#### kobject

The struct that supports the entire device model. It's never used on its own, but instead embedded in other structs that rely on it.

Most importantly, it holds a reference counter that allows the device to remain in memory as long as something is referencing it, and then be released as soon nothing does. That release function is a callback (which, in C, is a function pointer), held in `ktype`. This enables all devices of a given type to share a release function, reducing the memory needs while still allowing tailored behavior for different devices.

It is declared thusly: (from `include/linux/kobject.h`):

```c
struct kobject {
	const char				*name;
	struct list_head		entry;
	struct kobject			*parent;
	struct kset				*kset;
	const struct kobj_type	*ktype;
	struct kernfs_node		*sd; /* sysfs directory entry */
	struct kref				kref;

	unsigned int state_initialized:1;
	unsigned int state_in_sysfs:1;
	unsigned int state_add_uevent_sent:1;
	unsigned int state_remove_uevent_sent:1;
	unsigned int uevent_suppress:1;

#ifdef CONFIG_DEBUG_KOBJECT_RELEASE
	struct delayed_work	release;
#endif
};
```

Note the `unsigned int` have `:1` appended to them. This is a **bit-field**, which allows us to pack the values of all five flags into the space of a single unsigned int (32 bits). This is significant, as it allows the developper to save on padding space, reducing the size of the struct.

- *`container_of()`* is a macro used to recover the address of a containing struct from one of its member. It's used in places where the kernel only has, for instance, a pointer to a `kobj` member, and it needs to access the containing `cdev` struct. [Detailed explanations are available here](https://radek.io/posts/magical-container_of-macro/).

#### ksets

A `kset` is a collection of `kobj`. Any `kobj` is a `kset` is referenced by that `kset`.

Best decribed by their comment:

```c
/**
 * struct kset - a set of kobjects of a specific type, belonging to a specific subsystem.
 *
 * A kset defines a group of kobjects.  They can be individually
 * different "types" but overall these kobjects all want to be grouped
 * together and operated on in the same manner.  ksets are used to
 * define the attribute callbacks and other common events that happen to
 * a kobject.
 *
 * @list: the list of all kobjects for this kset
 * @list_lock: a lock for iterating over the kobjects
 * @kobj: the embedded kobject for this kset (recursion, isn't it fun...)
 * @uevent_ops: the set of uevent operations for this kset.  These are
 * called whenever a kobject has something happen to it so that the kset
 * can add new environment variables, or filter out the uevents if so
 * desired.
 */

```

It's worth noting that a `kset` contains a list of all the `kobjs` it holds, but also a single `kobj` that describes itself.

#### Subsystems

=> **THIS PART IS OUTDATED**

The subsystems are the highest level representations of the device model. It's basically a struct containing a `kset` and a [rw_semaphore](https://www.kernel.org/doc/html/latest/locking/locktypes.html), and the driver's developper need not create any new ones. One should simply assign their kset to the appropriate subsystem (`/sys/block`, `/sys/device`...) so that it may show up at the correct place in user space and appear in the correct hierarchy.

Examples of kobject and ksample usage are available in the kernel source tree, at `/sources/linux/samples/kobject/{kobject-example.c,kset-example.c}`.

Here, I got lost and discovered the concept of [RCU](https://www.kernel.org/doc/html/latest/RCU/whatisRCU.html) and the [percpu-rw-semaphore](https://docs.kernel.org/locking/percpu-rw-semaphore.html). Enjoy the sidequest[^1]!

[^1]: The RCU semaphore is used to ensure that the kset's list of kobjects can only be modified by a single thread or CPU at a time, but read only operations may continue during part of the modification or destruction process ; [that is achieved by separating the process in two parts](https://www.kernel.org/doc/html/latest/RCU/rcu.html), removal then reclamation; it's used to [protect read-mostly linked lists](https://www.kernel.org/doc/html/latest/RCU/listRCU.html), such as `ksets`.

### Sysfs Operations

Now that we covered (vaguely) kobjects and understood that they are the structure at work behind every sysfs mechanism, we are going to look at the operations that rely on `kobjs`.

For every directory that we find in /sys (which is normally where *sysfs* is mounted), there is an underlying `kobj` that was added with `kobject_add`. That directory name is specified by `kobject_set_name`, and its place in the hierarchy defined based on the `parent` tag. It contains kernel-generated file(s) representing the `kobj`'s attributes.

All that is very nice and well, but I didn't find it really helpful to make the module autoload, so I've moved on to greener pastures. I admit that I relied on AI for this, since basic sources were pretty hard to come by.

It's worth noting that my LFS runs `udev`, which will become relevant to the exercice soon.

### Simulating a keyboard hotplug in QEMU

I had to jump through a few hoops here, so I'll document it so you don't have to run in circles like me.

**NB**: I run my VM with `--nographic` because there are few things I hate more than a VM's console ; mostly I communicate with the guest through SSH, but I also redirected the kernel boot messages to `/dev/ttyS0` so I could monitor the booting process.

Now in order for us to "plug in" a keyboard, there are several things we have to do. First, we must add USB support to the VM itself by adding `-device qemu-xhci`[^2] to our startup command. That gives the guest a USB controler, allowing us to connect a virtual keyboard.

[^2]: XHCI stands for E[xtensible Host Controler Interface](https://en.wikipedia.org/wiki/Extensible_Host_Controller_Interface). Documentation for [`qemu-xhci`](https://www.qemu.org/docs/master/system/devices/usb.html).

Then, we have to `ctrl + c` `a` in the console to access the Qemu monitor. If that doesn't work for you, [you can read my book about it](qemu-sidequest.md).



## General applications sources

- [Message logging with printk](https://www.kernel.org/doc/html/latest/core-api/printk-basics.html)
- [The always useful list of GNU warning options](https://gcc.gnu.org/onlinedocs/gcc-15.2.0/gcc/Warning-Options.html#index-Wno-old-style-declaration)
