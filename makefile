VPATH = src:ppd:bin

ppds = np211.ppd.gz np326.ppd.gz
lphelpdocs = np211-options.txt np326-options.txt

DEFS=
LIBS=-lcupsimage -lcups

ifdef RPMBUILD
DEFS=-DRPMBUILD
LIBS=-ldl
endif

define dependencies
@if [ ! -e /usr/include/cups ]; then echo "CUPS headers not available - exiting"; exit 1; fi
@if ! (ls /usr/lib | grep libcups.* > /dev/null); then echo "libcups not available - exiting"; exit 1; fi
@if ! (ls /usr/lib | grep libcupsimage.* > /dev/null); then echo "libcupsimage not available - exiting"; exit 1; fi
endef

define init
@if [ ! -e bin ]; then echo "mkdir bin"; mkdir bin; fi
endef

define sweep
@if [ -e bin ]; then echo "rm -f bin/*"; rm -f bin/*; rmdir bin; fi
@if [ -e docs ]; then echo "rm -f docs/*"; rm -f docs/*; rmdir docs; fi
@if [ -e install ]; then echo "rm -f install/*"; rm -f install/*; rmdir install; fi
endef

install/setup: rastertonp $(ppds) setup
	# packaging
	@if [ -e install ]; then rm -f install/*; rmdir install; fi
	mkdir install
	cp bin/rastertonp install
	cp bin/*.ppd.gz install
	cp bin/setup install

.PHONY: install
install: 
	@if [ ! -e install ]; then echo "Please run make package first."; exit 1; fi
	# installing
	cd install; exec ./setup

.PHONY: remove
remove:
	#removing from default location (other locations require manual removal)
	@if [ -e /usr/lib/cups/filter/rastertonp ]; then echo "Removing rastertonp"; rm -f /usr/lib/cups/filter/rastertonp; fi
	@if [ -d /usr/share/cups/model/np ]; then echo "Removing dir .../cups/model/np"; rm -rf /usr/share/cups/model/np; fi

.PHONY: rpmbuild
rpmbuild: 
	@if [ ! -e install ]; then echo "Please run make package first."; exit 1; fi
	# installing
	RPMBUILD="true"; export RPMBUILD; cd install; exec ./setup

.PHONY: help
help: 
	# Help for npcupsdrv make file usage
	# 
	# command          purpose
	# ------------------------------------
	# make              compile all sources and create the install directory
	# make install      execute the setup shell script from the install directory [require root user permissions]
	# make remove       removes installed files from your system (assumes default install lication) [requires root user permissions]
	# make docs         compile printer driver option setting documentation
	# make clean        deletes all compiles files and their folders

rastertonp: rastertonp.c
	$(dependencies)
	$(init)
	# compiling rastertonp filter
	gcc -Wall -fPIC -O2 $(DEFS) -o bin/rastertonp src/rastertonp.c $(LIBS)

$(ppds): %.ppd.gz: %.ppd
	# gzip ppd file
	gzip -c $< >> bin/$@

setup: setup.sh
	$(dependencies)
	$(init)
	# create setup shell script
	cp src/setup.sh bin/setup
	chmod +x bin/setup

docs: $(lphelpdocs)

$(lphelpdocs): %-options.txt: %.ppd
	@if [ ! -e docs ]; then echo "mkdir docs"; mkdir docs; fi
	#get ppd options via lphelp
	lphelp $< >> docs/$@

.PHONY: clean
clean:
	# cleaning
	$(sweep)

