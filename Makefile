# Compiler Info
COMPILER    = gcc
CFLAGS      = -g -I . -Wall -Wshadow
RFLAGS      = -O3 -s -Wall -DNDEBUG
SRCDIR      = src

# Binaries
BINNAME     = main
BINDIR      = bin

# Disk file
DISKNAME    = ext2-dev-disk

# Testing
TESTDIR     = test
TESTFLAGS   = -fprofile-arcs -ftest-coverage
GTESTFLAGS  = -pthread -lgtest


default: all

all: 
	make build
#	make test
#	make memcheck

build: 
	make build-disk
	mkdir -p ./$(BINDIR)
	$(COMPILER) $(CFLAGS) -o $(BINDIR)/dev_$(BINNAME) $(SRCDIR)/*.c

build-tests: 
	make build-disk
	mkdir -p ./$(BINDIR)
	$(COMPILER) $(CFLAGS) $(TESTFLAGS) -o $(BINDIR)/test_$(BINNAME) $(TESTDIR)/$(BINNAME).c $(GTESTFLAGS)

build-release:
	make build-disk
	mkdir -p ./$(BINDIR)
	$(COMPILER) $(RFLAGS) -o $(BINDIR)/$(BINNAME) $(SRCDIR)/release_$(BINNAME).c

build-disk:
	mkdir -p ./$(BINDIR)
	rm -f ./$(BINDIR)/$(DISKNAME)
#	mkdir -p /mnt/$(DISKNAME)

	# Build and mount the disk
	dd if=/dev/zero of=./$(BINDIR)/$(DISKNAME) bs=1024K count=1
	mkfs.ext2 ./$(BINDIR)/$(DISKNAME)
#	mount ./$(BINDIR)/$(DISKNAME) /mnt/$(DISKNAME) 

	# Add some sample data to the disk..
#	echo "This is a test file!" >> /mnt/$(DISKNAME)/test.txt
#	mkdir /mnt/$(DISKNAME)/test-folder
#	echo "This is a test file in the test folder!" >> /mnt/$(DISKNAME)/test-folder/test2.txt

	# Unmount the disk
#	umount /mnt/$(DISKNAME)
#	rm -rf /mnt/$(DISKNAME)

run: 
	make build
	@echo "Running program"
	./$(BINDIR)/dev_$(BINNAME) $(BINDIR)/$(DISKNAME)

test:
	make build
	make build-tests
	@echo "Running tests..."
	./$(BINDIR)/test_$(BINNAME)

memcheck:
	make build-test
	valgrind --error-exitcode=1 --leak-check=full --trace-children=yes ./$(BINDIR)/test_$(BINNAME)

