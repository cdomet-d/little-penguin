NAME:= misc_cdriver
obj-m += misc_cdriver.o

PWD := $(CURDIR)

RED    := \033[1;31m
GREEN  := \033[1;32m
CYAN   := \033[1;36m
YELLOW := \033[1;33m
RESET  := \033[0m
LOG    := proof.log

define section
	@printf '\n$(CYAN)━━ %s ━━$(RESET)\n' '$(1)'
endef

define run
	@printf '$(YELLOW)$$ %s$(RESET)\n' '$(1)'
	@$(1); rc=$$?; \
	if [ $$rc -eq 0 ]; then printf '$(GREEN)Exit status: %s\n$(RESET)' $$rc; \
	else printf '$(RED)Exit status: %s\n$(RESET)' $$rc; fi
endef

all:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	$(sudo shell cp $(NAME).ko /lib/modules/$(shell uname -r)/)
	$(sudo shell depmod)


clean: unload
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

re: clean all

cp:
	cp $(NAME).ko /lib/modules/$(shell uname -r)/

unload:
	$(call section,unload)
	-$(call run,rmmod $(NAME))
	$(call run,dmesg -T | tail -1)

load: unload all cp
	$(call section,load)
	$(call run,insmod $(NAME).ko)
	$(call run,dmesg -T | tail -1)

proof: load
	@$(MAKE) --no-print-directory _proof 2>&1 | tee $(LOG)

_proof:
	$(call section,Device exists)
	$(call run,ls -la /dev | grep forty)

	$(call section,Read Operation)
	$(call run,cat /dev/fortytwo)

	$(call section,Valid Write Operation)
	$(call run,echo cdomet-d > /dev/fortytwo)
	
	$(call section,Invalid Write Operation)
	$(call run,echo cdomet-di > /dev/fortytwo)

	$(call section,Invalid Write Operation)
	$(call run,echo > /dev/fortytwo)
log: 
	dmesg -T | tail -5