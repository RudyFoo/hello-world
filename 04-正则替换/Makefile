CFLAGS += -DGCUNAME=\"$(ver)\"
CFLAGS += -lpcre
TARGET=hex_trans
$(TARGET):hex_trans.c
	gcc hex_trans.c $(CFLAGS) -o hex_trans
.PHONY : clean
clean:
	@rm -rf *.o
	@rm -rf $(TARGET)