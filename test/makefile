# Makefile

build_dir = ./build
dist_dir = ./dist
target = test

inchdr = -I ".."
CFLAGS += -MMD -Wall $(inchdr)
LDFLAGS += -lyaml

src0_dir = .
obj0_dir = $(build_dir)/obj0
src0 = $(wildcard $(src0_dir)/*.c)
obj0= $(patsubst $(src0_dir)/%.c, $(obj0_dir)/%.o, $(src0))
src += $(src0)
obj += $(obj0)

src1_dir = ..
obj1_dir = $(build_dir)/obj1
src1 = $(wildcard $(src1_dir)/*.c)
obj1= $(patsubst $(src1_dir)/%.c, $(obj1_dir)/%.o, $(src1))
src += $(src1)
obj += $(obj1)

dep = $(obj:.o=.d)

.PRECIOUS: $(build_dir)/. $(build_dir)%/. $(dist_dir)/. $(dist_dir)%/.

.PHONY: clean

build: $(dist_dir)/$(target)

all: clean build

clean:
	rm -rf $(dep) $(obj) $(dist_dir)/$(target)

$(dist_dir)/.:
	mkdir -p $@

$(obj0_dir)/.:
	mkdir -p $@

$(obj1_dir)/.:
	mkdir -p $@

.SECONDEXPANSION:

$(dist_dir)/$(target): $(obj) | $$(@D)/.
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(obj0_dir)/%.o: $(src0_dir)/%.c | $$(@D)/.
	$(CC) $(CFLAGS) -c -o $@ $<

$(obj1_dir)/%.o: $(src1_dir)/%.c | $$(@D)/.
	$(CC) $(CFLAGS) -c -o $@ $<

-include $(dep)
