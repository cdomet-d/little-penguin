# Little Pinguin

The genus Eudyptula from Ancient Greek εὖ (eû), meaning "well", δύπτης (dúptes), meaning "diver", and Latin -ula, a diminutive suffix, contains a single extant species of penguin, found in southern Australia, Tasmania, and New Zealand (including the Chatham Islands). It is commonly known as the little penguin, little blue penguin, or, in Australia, fairy penguin. In the language of the Māori people of New Zealand, little penguins are known as kororā.[3]

For many years, a white-flippered form of the little penguin found only in North Canterbury, New Zealand was considered either a separate species, Eudyptula albosignata, or just a subspecies, Eudyptula minor albosignata. Analysis of mtDNA revealed that Eudyptula falls instead into two groups: a western one, found along the southern coast of Australia and the Otago region of New Zealand, and another found in the rest of New Zealand.[4] E. novaehollandiae probably arrived in New Zealand from Australia less than 500 years ago, following the local extinction of E. minor in Otago.[5]
[The Eudyptula Challenge](http://eudyptula-challenge.org/)

## ex00

The hardest thing about this part was fixing the disk corruption that happened on my LFS distributions, though I guess I learned stuff `(:`

Getting the kernel from [git.kernel.org](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/) is quite straightforward, though it requires installing git on your LFS distribution - which I didn't do before, but is easily managed by following the appropriate [BLFS page](https://linuxfromscratch.org/blfs/view/stable/general/git.html) (make sure to use the approriate version of BLFS, the one that matches your LFS).

Then, you may either go through the manual configuration of the kernel with `make menuconfig`  or `make defconfig`. However, as I did not quite fancy doing that, I recovered my LFS `.config` and used `make olddefconfig`, which :

> The make target `olddefconfig` (and the yes "" | used when utilizing localmodconfig) will set any undefined build options to their default value. This among others will disable many kernel features that were introduced after your base kernel was released. [[doc.kernel.org](https://docs.kernel.org/admin-guide/quickly-build-trimmed-linux.html), 08.10.2026]

I chose to not use `localmodconfig` as this [documentation](https://docs.kernel.org/admin-guide/quickly-build-trimmed-linux.html) warns that only the kernel modules related to actions already performed wouldn't be autoloaded when said action is perfomed after kernel compilation. As I'm not quite sure how much stuff I'll do with my distribution, I chose to remain as I was ; you do what you think is best for your usage.

![However](.assets/cependant-jdg.gif)

Since we will be developpig modules later on, I recommend your ensure `CONFIG_MODULE_DEBUG` is set to `=y`. Indeed, this parameter *Allows you to enable / disable features which can help you debug modules.*, which sounds helpful [[lkddb](https://cateee.net/lkddb/web-lkddb/MODULE_DEBUG.html), 08.10.2026]

Once you are satisfied with the config, you can build the kernel and update your grub configuration to take into account your new kernel.

- [**Displaying kernel messages**](https://linuxvox.com/blog/kern-log-linux/)
- [**Quick Linux build**](https://docs.kernel.org/admin-guide/quickly-build-trimmed-linux.html)

## ex01

### Sources

- [**In-Tree vs Out-Of-Tree Kernel Modules**](https://medium.com/@adityapatnaik27/linux-kernel-module-in-tree-vs-out-of-tree-build-77596fc35891)
- [**Definition and Examples**](https://linux-kernel-labs.github.io/refs/heads/master/labs/kernel_modules.html)
- [**Building external modules with Kbuild**](https://www.kernel.org/doc/html/latest/kbuild/modules.html)
