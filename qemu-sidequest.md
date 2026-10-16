# Do you hate VM consoles ? Me too

I hate VM consoles, so I use `--nographic` and SSH to run headless most of the time.

However, sometimes, it's nice to have more information (during boot, for instance, to see the kernel and GRUB's output). It could also be pretty cool to be able to input stuff in the terminal you're using to run your VM - reduces the amount of open terminals, for instance.

## `--nographic` and what it does

From the [Qemu Invocation Documentation](https://www.qemu.org/docs/master/system/invocation.html):

> `-nographic`
>
> Normally, if QEMU is compiled with graphical window support, it displays output such as guest graphics, guest console, and the QEMU monitor in a window. With this option, you can totally disable graphical output so that QEMU is a simple command line application. The emulated serial port is redirected on the console and muxed with the monitor (unless redirected elsewhere explicitly). Therefore, you can still use QEMU to debug a Linux kernel with a serial console. Use C-a h for help on switching between the console and monitor.

This is the part that interrests us: *The emulated serial port is redirected on the console and muxed with the monitor (unless redirected elsewhere explicitly).*

## Issue 1: Silent Boot

The kernel, by default, uses `printk` to send messages to `/dev/tty0`. `tty0` is always the virtual console that is currently in the foreground. For its part, `GRUB` defaults to the computer's [VGA text-mode](https://en.wikipedia.org/wiki/VGA_text_mode), which is a region of memory mapped to the video card.

With `nographic`, however, neither of those are available to the user. The only thing we have access to is the terminal Qemu is running in, which is, as the documentation just told us, linked to the guest's emulated [serial port](https://en.wikipedia.org/wiki/Universal_asynchronous_receiver-transmitter). That serial port has a name, which is `/dev/ttyS0` ; this is what we need to tell the kernel and GRUB *where* to write.

In the context of LFS, where we litteraly wrote the booting configuration by hand (at least I did, maybe there were easier ways), the easiest way to do that is to add a few lines to our `grub.cfg` file. That way, the kernel and GRUB can redirect their messages to the appropriate console, and Qemu, in turn, can print the output to its own STDOUT, which is the terminal in which it's running.

The lines to add are commented thereafter:

```sh
# Begin /boot/grub/grub.cfg
set default=0
set timeout=2

# this configures the "serial" that we use for terminal_output, only for GRUB
serial --unit=0 --speed=115200 --word=8 --parity=no --stop=1 
terminal_output console serial	# tells GRUB to output to ttyS0
terminal_input console serial	# tells GRUB to accept keystrokes from ttys0 
								# (to pick a disk to boot, open the GRUB command line...)

insmod part_msdos
insmod ext2
search --no-floppy --fs-uuid --set=root 5e6b410d-5ba8-460d-9930-b9d63d0b72b0
set gfxpayload=1024x768x32

menuentry "GNU/Linux, Linux 7.3 Torvalds - LITTLE PENGUIN" {
		# This configures only the kernel
		# Here, we add console=ttyS0,115200n8 ; this redirects the kernel to write to the serial port
        linux /vmlinuz-7.3-torvalds root=PARTUUID=cafe2737-03 ro console=ttyS0,115200n8	
}
```

## Issue 2: Lost Input

We did give GRUB instructions to listen to keystrokes from `/dev/ttyS0`, but once the booting process is over, GRUB says bye and hands over control to the kernel, and then we're on our own - the console we specified for the kernel in `grub.cfg` is only for output.

When using Qemu with `nographic`, our only communication channel is the serial port `ttyS0`. Our input is stored in that buffer ; we need to point our very first interactive process - `agetty` (or getty, etc...) to it.

Agetty is the program that runs `/bin/login`. It asks for your login, then runs `login` which validates your password, and if it is correct, starts the shell which inherits the input and output. From there, every process that starts will inherit the correct inputs and output.

If you followed the LFS handbook, `agetty` will probably run on `/dev/tty1`, which we don't want because we cannot access it. Instead, we want to use our faithful `ttyS0`.

If you use `SysV`, simply add that line to your [`/etc/inittab` file](https://manpages.org/inittab/5):

```sh
# this will run for all levels besides halt and reboot
#ID:RUNLEVEL:ACTION:COMMAND
[UNIQUE_ID]:12345:respawn:/sbin/agetty -L --noclear ttyS0 115200
```

## Redirecting the Qemu monitor to a TCP socket (because ain't nobody got time for that)

Since we used the serial to output the content of our guest, access to the Qemu monitor can get a bit finnicky. I got tired of it real quick and instead chose to redirect the monitor to a TCP port that I access with `nc`.

If you would like to do the same, just add `-monitor tcp:127.0.0.1:4444,server,nowait \` to your Qemu invocation, then run `nc 127.0.0.1 4444`. Voila, you have monitor access !