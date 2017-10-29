
DIRS=liblock/ memcached/memcached-1.4.6-ffwd/ memcached/memcached-1.4.6-patched/ memcached/libmemcached-1.0.2/ \
	splash2/splash2-ffwd/codes/apps/raytrace splash2/splash2-ffwd/codes/apps/radiosity phoenix/phoenix-ffwd micro-benchmarks/versioning/ micro-benchmarks/skiplist/ micro-benchmarks/skiplist/

.PHONY: liblock memcached splash2 phoenix

all: create_bin liblock microbenchmarks memcached splash2 phoenix

liblock:
	@echo "Compiling RCL's liblock library"
	$(MAKE) -C liblock/

microbenchmarks: fetch-n-add simplelist lazylist hashtable skiplist  versioning
	@echo "Compiling Micro-Benchmarks"

fetch-n-add: liblock
	@echo "Compiling fetch-n-add with multiple shared variables"
	$(MAKE) -C micro-benchmarks/fetch-n-add/ -f Makefile.ffwd
	$(MAKE) -C micro-benchmarks/fetch-n-add/ -f Makefile.ssync
	$(MAKE) -C micro-benchmarks/fetch-n-add/ -f Makefile.liblock
	cp micro-benchmarks/fetch-n-add/bin/*-shared-vars bin/

simplelist: liblock
	@echo "Compiling Simple Linked Lists"
	$(MAKE) -C micro-benchmarks/simple-linkedlist/ -f Makefile.ffwd
	$(MAKE) -C micro-benchmarks/simple-linkedlist/ -f Makefile.ssync
	cp micro-benchmarks/simple-linkedlist/bin/*-simplelist bin/

lazylist: liblock
	@echo "Compiling Lazy Linked Lists"
	$(MAKE) -C micro-benchmarks/lazy-linkedlist/ -f Makefile.ffwd
	$(MAKE) -C micro-benchmarks/lazy-linkedlist/ -f Makefile.ssync
	$(MAKE) -C micro-benchmarks/lazy-linkedlist/ -f Makefile.liblock 
	cp micro-benchmarks/lazy-linkedlist/bin/*-lazylist bin/

hashtable: liblock
	@echo "Compiling Hash Tables"
	$(MAKE) -C micro-benchmarks/hashtable/ -f Makefile.ffwd
	$(MAKE) -C micro-benchmarks/hashtable/ -f Makefile.ssync
	cp micro-benchmarks/hashtable/bin/*-hashtable bin/

skiplist:
	@echo "Compiling skiplist"
	$(MAKE) -C micro-benchmarks/skiplist/

versioning: liblock
	@echo "Compiling versioning"
	$(MAKE) -C micro-benchmarks/versioning/

memcached: memcached-ffwd memcached-liblock libmemcached

memcached-ffwd: liblock
	@echo "Compiling memcached-ffwd"
	$(MAKE) -C memcached/memcached-1.4.6-ffwd/
	cp memcached/memcached-1.4.6-ffwd/memcached bin/ffwd-memcached

memcached-liblock: liblock
	@echo "Compiling memcached-liblock"
	$(MAKE) -C memcached/memcached-1.4.6-patched/
	cp memcached/memcached-1.4.6-patched/memcached bin/liblock-memcached

libmemcached: liblock
	@echo "Compiling libmemcached"
	$(MAKE) -C memcached/libmemcached-1.0.2/
	cp memcached/libmemcached-1.0.2/clients/memslap bin/memslap

splash2: splash2-ffwd splash2-liblock

splash2-ffwd: raytrace radiosity

phoenix: liblock
	@echo "Compiling Phoenix"
	$(MAKE) -C phoenix/phoenix-ffwd

raytrace: liblock
	@echo "Compiling Splash2 Raytrace"
	$(MAKE) -C splash2/splash2-ffwd/codes/apps/raytrace
	cp splash2/splash2-ffwd/codes/apps/raytrace/RAYTRACE bin/ffwd-raytrace

radiosity: liblock
	@echo "Compiling Splash2 Radiosity"
	$(MAKE) -C splash2/splash2-ffwd/codes/apps/radiosity/glibdumb
	$(MAKE) -C splash2/splash2-ffwd/codes/apps/radiosity/glibps
	$(MAKE) -C splash2/splash2-ffwd/codes/apps/radiosity
	cp splash2/splash2-ffwd/codes/apps/radiosity/RADIOSITY bin/ffwd-radiosity

splash2-liblock: liblock
	@echo "Compiling Splash2-Liblock"
	$(MAKE) rebuild-all SPLASH=splash2 BASEDIR=$(PWD)/splash2/splash2-liblock -C splash2/splash2-liblock/
	cp splash2/splash2-liblock/splash2/codes/apps/raytrace/RAYTRACE bin/liblock-raytrace
	cp splash2/splash2-liblock/splash2/codes/apps/radiosity/RADIOSITY bin/liblock-radiosity

create_bin:
	mkdir -p bin

ffwd-clean: memcached-ffwd-clean splash2-ffwd-clean

memcached-ffwd-clean:
	$(MAKE) clean memcached/memcached-1.4.6-ffwd/

splash2-ffwd-clean:
	$(MAKE) clean splash2/splash2-ffwd/codes/apps/raytrace
	$(MAKE) clean splash2/splash2-ffwd/codes/apps/radiosity

clean:
	for d in $(DIRS); do (cd $$d; $(MAKE) clean ); done
	$(MAKE) clean -C micro-benchmarks/fetch-n-add/ -f Makefile.ffwd
	$(MAKE) clean -C micro-benchmarks/fetch-n-add/ -f Makefile.ssync
	$(MAKE) clean -C micro-benchmarks/fetch-n-add/ -f Makefile.liblock
	$(MAKE) clean -C micro-benchmarks/simple-linkedlist/ -f Makefile.ffwd
	$(MAKE) clean -C micro-benchmarks/simple-linkedlist/ -f Makefile.ssync
	$(MAKE) clean -C micro-benchmarks/lazy-linkedlist/ -f Makefile.ffwd
	$(MAKE) clean -C micro-benchmarks/lazy-linkedlist/ -f Makefile.ssync
	$(MAKE) clean -C micro-benchmarks/lazy-linkedlist/ -f Makefile.liblock
	$(MAKE) clean -C micro-benchmarks/hashtable/ -f Makefile.ffwd
	$(MAKE) clean -C micro-benchmarks/hashtable/ -f Makefile.ssync

