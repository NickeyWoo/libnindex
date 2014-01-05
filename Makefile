#
# globalmem library
# author: nickeywoo
# date: 2013-12-20
#

include ./Makefile.env

all:
	$(MAKE) -C src
	$(MAKE) -C example

clean:
	$(MAKE) -C example clean
	$(MAKE) -C src clean


