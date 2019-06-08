#
# bibutils unit tests MAKEFILE
#

CFLAGS     = -I ../lib $(CFLAGSIN)
LDFLAGS    = $(LDFLAGSIN)
PROGS      = doi_test \
             entities_test \
             intlist_test \
             slist_test \
             str_test \
             utf8_test

all: $(PROGS)

entities_test : entities_test.o ../lib/libbibcore.a
	$(CC) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

utf8_test : utf8_test.o ../lib/libbibcore.a
	$(CC) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

doi_test : doi_test.o ../lib/libbibcore.a
	$(CC) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

str_test : str_test.o ../lib/libbibcore.a
	$(CC) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

slist_test : slist_test.o ../lib/libbibcore.a
	$(CC) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

intlist_test : intlist_test.o ../lib/libbibcore.a
	$(CC) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

test: $(PROGS) FORCE
	./str_test
	./slist_test
	./intlist_test
	./entities_test
	./doi_test
	./utf8_test

clean:
	rm -f *.o core 

realclean:
	rm -f *.o core $(PROGS)
	@for p in $(PROGS); \
               do ( rm -f $$p$(EXEEXT) ); \
        done

FORCE:
