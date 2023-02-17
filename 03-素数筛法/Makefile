CFLAGS += -DN=$(num)
a.out:prime.c
	@echo "【usage e.g.: make clean && make num=1000】"
	-@gcc prime.c $(CFLAGS) -o a.out
	-@./a.out
.PHONY: clean
clean:
	-@rm -rf *.o  a.out
	-@rm -rf $(TARGET)
