sub_dir := $(filter %/, $(obj))
sub_make := $(patsubst %/, %_SUB_MAKE, $(sub_dir))
sub_clean := $(patsubst %/, %_SUB_CLEAN, $(sub_dir))

cur_obj := $(filter %.o, $(obj))
sub_obj := $(patsubst %/, %/build_in.o, $(sub_dir))

.PHONY:clean
.PHONY:$(sub_dir)

build_in.o: $(cur_obj) $(sub_make)
	@$(LD) $(LDFLAGS) -r $(cur_obj) $(sub_obj) -o build_in.o
	@echo LD $@
%.d: %.c
	@$(CC) -M $(CFLAGS) $< >$@

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo CC $@

$(sub_make):
	@$(MAKE) -C $(patsubst %_SUB_MAKE, %/, $@)

$(sub_clean):
	@$(MAKE) -C $(patsubst %_SUB_CLEAN, %/, $@) clean

clean: $(sub_clean)
	@rm -f *.o *.c~ *.d

-include $(cur_obj:.o=.d)
