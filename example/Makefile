#
# nindex library
# author: nickeywoo
# date: 2013-12-20
#

include ../Makefile.env

TARGET := ../bin/hashtable_example ../bin/bitmap_example ../bin/bloomfilter_example ../bin/rbtree_example ../bin/blocktable_example ../bin/kdtree_example ../bin/heap_example

all: $(TARGET)

clean:
	rm -rf objs/*.o
	rm -rf $(TARGET)

../bin/hashtable_example: objs/hashtable_main.o
	$(CXX) $^ -o $@ $(LIBS)

../bin/bitmap_example: objs/bitmap_main.o
	$(CXX) $^ -o $@ $(LIBS)

../bin/bloomfilter_example: objs/bloomfilter_main.o
	$(CXX) $^ -o $@ $(LIBS)

../bin/rbtree_example: objs/rbtree_main.o
	$(CXX) $^ -o $@ $(LIBS)

../bin/blocktable_example: objs/blocktable_main.o
	$(CXX) $^ -o $@ $(LIBS)

../bin/kdtree_example: objs/kdtree_main.o
	$(CXX) $^ -o $@ $(LIBS) -L/usr/lib64 -lMagick++

../bin/heap_example: objs/heap_main.o
	$(CXX) $^ -o $@ $(LIBS)




