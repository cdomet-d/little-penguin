# Ch. 14 of Linux Device Drivers, Third Edition

- [Ch. 14 of Linux Device Drivers, Third Edition](#ch-14-of-linux-device-drivers-third-edition)
	- [The device model](#the-device-model)
		- [kobject](#kobject)
		- [ksets](#ksets)
		- [Subsystems](#subsystems)
		- [Sysfs Operations](#sysfs-operations)

[**Virtual Filesystem (VFS)**](https://en.wikipedia.org/wiki/Virtual_file_system): an abstraction layer sitting in between userspace and the filesystems implementation. When a syscall originates from the userspace (`open`, `read`, `write`...), the call transits via the VFS. This is the *strict* name of the kernel's abstraction layer that enables actions on the filesystems.

[**Pseudo Filesystem**](https://en.wikipedia.org/wiki/Synthetic_file_system): a filesystem *implementation* that exposes kernel state or lives in memory. It is rules by the VFS, but doesn't rely on a physical device. `/sys` and `/proc` are pseudo filesystems: even if they present files to the users, most live in memory and their contents are generated on demand by the kernel. For instance, the directories in `/proc` representing the running processes come and go as the processes live and die.

[**sysfs**](https://en.wikipedia.org/wiki/Sysfs): a pseudo filesystem that exposes the kernel's devide model to user space in a hierarchy of files.

## The device model

- [Recommended reading: Ch. 14 of *Linux Device Drivers, Third Edition*](https://static.lwn.net/images/pdf/LDD3/ch14.pdf)
- [*Linux Device Drivers, Third Edition*, full book](https://lwn.net/Kernel/LDD3/)

**WARNING:** this is my understanding from the LDD3. It might be wrong, or outdated ; I mostly write those as a way to understand things better. **Feel free to create an issue or fork and update :)**

### kobject

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

### ksets

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

### Subsystems

=> **THIS PART IS OUTDATED**

The subsystems are the highest level representations of the device model. It's basically a struct containing a `kset` and a [rw_semaphore](https://www.kernel.org/doc/html/latest/locking/locktypes.html), and the driver's developper need not create any new ones. One should simply assign their kset to the appropriate subsystem (`/sys/block`, `/sys/device`...) so that it may show up at the correct place in user space and appear in the correct hierarchy.

Examples of kobject and ksample usage are available in the kernel source tree, at `/sources/linux/samples/kobject/{kobject-example.c,kset-example.c}`.

Here, I got lost and discovered the concept of [RCU](https://www.kernel.org/doc/html/latest/RCU/whatisRCU.html) and the [percpu-rw-semaphore](https://docs.kernel.org/locking/percpu-rw-semaphore.html). Enjoy the sidequest[^1]!

[^1]: The RCU semaphore is used to ensure that the kset's list of kobjects can only be modified by a single thread or CPU at a time, but read only operations may continue during part of the modification or destruction process ; [that is achieved by separating the process in two parts](https://www.kernel.org/doc/html/latest/RCU/rcu.html), removal then reclamation; it's used to [protect read-mostly linked lists](https://www.kernel.org/doc/html/latest/RCU/listRCU.html).

### Sysfs Operations

Now that we covered (vaguely) kobjects and understood that they are the structure at work behind every sysfs mechanism, we are going to look at the operations that rely on `kobjs`.

For every directory that we find in /sys (which is normally where *sysfs* is mounted), there is an underlying `kobj` that was added with `kobject_add`. That directory name is specified by `kobject_set_name`, and its place in the hierarchy defined based on the `parent` tag. It contains kernel-generated file(s) representing the `kobj`'s attributes.

All that is very nice and well, but I didn't find it really helpful to make the module autoload, so I've moved on to greener pastures. I admit that I relied on AI for this, since basic sources were pretty hard to come by.

It's worth noting that my LFS runs `udev`, which will become relevant to the exercice soon.