
CC      := gcc
CFLAGS  := -no-pie -Wall -Wextra

depend  := .depend

define make_progs
src-$(1) := $(1).c
obj-$(1) := $(1).o $(obj-common)

$(1): $$(obj-$(1))
endef

# Do not compile into executable
src-common := common.c
obj-common := $(src-common:.c=.o)

.PHONY: all
all: debug

bin-all := attack server master
$(foreach progname,$(bin-all),\
    $(eval $(call make_progs,$(progname))))

# sort function is only needed to remove duplicates...
src-all := $(sort $(foreach progname,$(bin-all),$(src-$(progname))))
obj-all := $(obj-common)
obj-all += $(patsubst %.c,%.o,$(src-all))

-include $(depend)
$(depend): $(src-all)
	$(CC) -MM $^ > $@

debug: CFLAGS += -g
release: CFLAGS += -DNDEBUG

.PHONY: debug release
debug release: $(bin-all) $(memory)

.PHONY: clean
clean:
	$(RM) $(obj-all)
	$(RM) $(bin-all)
	$(RM) $(depend)

