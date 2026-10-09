# Little Pinguin

The genus Eudyptula from Ancient Greek εὖ (eû), meaning "well", δύπτης (dúptes), meaning "diver", and Latin -ula, a diminutive suffix, contains a single extant species of penguin, found in southern Australia, Tasmania, and New Zealand (including the Chatham Islands). It is commonly known as the little penguin, little blue penguin, or, in Australia, fairy penguin. In the language of the Māori people of New Zealand, little penguins are known as kororā.[3]

For many years, a white-flippered form of the little penguin found only in North Canterbury, New Zealand was considered either a separate species, Eudyptula albosignata, or just a subspecies, Eudyptula minor albosignata. Analysis of mtDNA revealed that Eudyptula falls instead into two groups: a western one, found along the southern coast of Australia and the Otago region of New Zealand, and another found in the rest of New Zealand.[4] E. novaehollandiae probably arrived in New Zealand from Australia less than 500 years ago, following the local extinction of E. minor in Otago.[5]
[The Eudyptula Challenge](http://eudyptula-challenge.org/)

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

(personnaly, I followed the LFS step of the kernel build - it helps covering the important steps of building the kernel).

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

- [**In-Tree vs Out-Of-Tree Kernel Modules**](https://medium.com/@adityapatnaik27/linux-kernel-module-in-tree-vs-out-of-tree-build-77596fc35891)
- [**Definition and Examples**](https://linux-kernel-labs.github.io/refs/heads/master/labs/kernel_modules.html)

### Official Kernel documentation

- [**Kbuild documentation**](https://docs.kernel.org/kbuild/index.html)
- [**More information and complicated kbuild/Makefiles examples**](https://www.kernel.org/doc/html/latest/kbuild/modules.html), including multi-files build.
- [**More information about the kernel build in general**](https://www.kernel.org/doc/html/latest/kbuild/makefiles.html)

## ex02 - Patching

- [**Submitting patches**](https://www.kernel.org/doc/html/latest/process/submitting-patches.html)
- [**Using git format-patch**](https://git-scm.com/docs/git-format-patch)