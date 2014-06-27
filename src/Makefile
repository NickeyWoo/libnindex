#
# nindex library
# author: nickeywoo
# date: 2013-12-20
#

include ../Makefile.env

TARGET := ../lib/libnindex.a
LIB_OBJS := objs/hashtable.o objs/utility.o objs/storage.o objs/file.o
CITYHASH_OBJ := ../thirdparty/cityhash-1.1.1/objs/city.o

all: $(TARGET)

clean:
	rm -rf objs/*.o
	rm -rf $(TARGET)

../lib/libnindex.a: $(LIB_OBJS)
	$(AR) -r $@ $^ $(CITYHASH_OBJ)


