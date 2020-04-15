# Compiler Info
COMPILER    = gcc
CFLAGS      = -g -I . -Wall -Wshadow -Wconversion
RFLAGS      = -O3 -s -Wall -DNDEBUG
SRCDIR      = src

# Binaries
BINNAME     = main
BINDIR      = bin

# Testing
TESTDIR     = test
TESTFLAGS   = -fprofile-arcs -ftest-coverage
GTESTFLAGS  = -pthread -lgtest


default: all

all: 
	build
	test
	memcheck

build: 
	$(SRCDIR)/$(BINNAME).c
	mkdir -p BINDIR
	$(COMPILER) $(CFLAGS) -o $(BINDIR)/dev_$(BINNAME) $(SRCDIR)/$(BINNAME).c

build-tests: 
	$(SRCDIR)/$(BINNAME).c
	mkdir -p BINDIR
	$(COMPILER) $(CFLAGS) $(TESTFLAGS) -o $(BINDIR)/test_$(BINNAME) $(TESTDIR)/$(BINNAME).c $(GTESTFLAGS)

build-release:
	$(SRCDIR)/$(BINNAME).c
	mkdir -p BINDIR
	$(COMPILER) $(RFLAGS) -o $(BINDIR)/$(BINNAME) $(SRCDIR)/release_$(BINNAME).c

run: 
	build
	@echo "Running program"
	./$(BINDIR)/dev_$(BINNAME)

test:
	build
	build-tests
	@echo "Running tests..."
	./$(BINDIR)/test_$(BINNAME)

memcheck:
	build-test
	valgrind --error-exitcode=1 --leak-check=full --trace-children=yes ./$(BINDIR)/test_$(BINNAME)
