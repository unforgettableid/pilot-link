# Generated automatically from Makefile.in by configure.

prefix = /usr/pilot
exec_prefix = ${prefix}
bindir = ${exec_prefix}/bin
libdir = ${exec_prefix}/lib
pixdir = ${libdir}/pix
includedir = ${prefix}/include
mandir = ${prefix}/man/man1
srcdir = .

top_builddir = .

PURELINK =
PURIFY =

# Libraries needed to link with Tcl & (optionally) Tk
TCLTK_LIBS = -L/usr/local/tcl8.0/lib -ltk8.0 -L/usr/local/tcl8.0/lib -ltcl8.0 -L/usr/X11R6/lib -lX11 -ldl  -lm
# Include directives needed to compile with Tcl & Tk, and
# -DTCL if Tcl is available, and
# -DTK if Tk is available.
TCLTK_FLAGS = -DTCL -DTK -I/usr/local/tcl8.0/include

# Libraries needed to link with GNU readline
RLLIBS = -lreadline -ltermcap
# Include directives needed to compile with GNU readline, and
# -DREADLINE_2_0 if GNU readline 2.0 is available, or 
# -DREADLINE_2_1 if GNU readline 2.1 (or later) is available.
RLDEFS = -DREADLINE_2_0

PERL5 = /usr/local/bin/perl

CC = gcc
CXX = c++
CDEFS = -DLIBDIR=\"$(libdir)\" 
# -DDEBUG
CFLAGS = -I./include -I$(srcdir)/include -g -O2 -Wall $(RLDEFS) $(CDEFS) $(TCLTK_FLAGS)
CXXFLAGS = -I./include -I$(srcdir)/include -g -O2 -Wall $(RLDEFS) $(CDEFS) $(TCLTK_FLAGS)
LDFLAGS = 
LIBS =  -lm
# -lefence
CXXLIBS =  -lg++

libcclib = libcc/libpicc.a
cclib = libpicc.a

INSTALL = $(LIBTOOL) /usr/bin/ginstall -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
YACC = bison -y
RANLIB = ranlib
RM = rm -f
STRIP = strip
SHELL = /bin/sh
CPLIB = cp
EXT = 
SUBMAKE_COMM = cd $(top_builddir)/libsock ; $(MAKE) 
SUBMAKE_COMM_DUBIOUS = cd $(top_builddir)/dubious ; $(MAKE) 
SUBMAKE_COMM_CC = cd $(top_builddir)/libcc ; $(MAKE) 

LIBTOOL = $(top_builddir)/libtool
LIBTOOLLINK = $(LIBTOOL) --mode=link

CCLINK = $(LIBTOOLLINK)  $(PURELINK) $(PURIFY) $(CC) $(LDFLAGS) $(CFLAGS)
CXXLINK = $(LIBTOOLLINK) $(PURELINK) $(PURIFY) $(CXX) $(LDFLAGS) $(CXXFLAGS)

.SUFFIXES: .out .o .cc .c .e .r .f .y .l .s .p

.c.o:
	$(CC) $(CFLAGS) -c $<

.cc.o:
	$(CXX) $(CXXFLAGS) -c $<


WITHTCL = $(TCLEXECS)
WITHCXX = $(CXXEXECS)
WITHPERL5 = $(PERLEXECS)

CEXECS = pilot-xfer$(EXT) pilot-file$(EXT) pilot-dedupe$(EXT) pilot-schlep$(EXT)\
		install-memo$(EXT) install-user$(EXT) install-datebook$(EXT)\
		install-todos$(EXT) pilot-addresses$(EXT) pilot-clip$(EXT)\
		read-ical$(EXT) pilot-mail$(EXT) read-expenses$(EXT) $(CCEXECS) \
	    reminders$(EXT) memos$(EXT) addresses$(EXT) read-todos$(EXT)\
		debugsh$(EXT) dlpsh$(EXT) \
		getrom$(EXT) pi-getrom$(EXT) pi-getram$(EXT) pi-port$(EXT) \
		pi-csd$(EXT) pi-nredir$(EXT) 

CXXEXECS = ccexample$(EXT) iambicexample$(EXT)

TCLEXECS = pilot-debug$(EXT)

PERLEXECS = pilot-undelete ietf2datebook sync-plan

GETOPT = getopt.o getopt1.o

EXECS = $(CEXECS) $(WITHTCL) $(WITHCXX) $(WITHPERL5)

PILIB = libsock/libpisock.la
PICCLIB = libcc/libpicc.a

all: $(EXECS)

$(PILIB): FORCE
	$(SUBMAKE_COMM) libpisock.la

libcc/libpicc.a: FORCE
	$(SUBMAKE_COMM_CC) libpicc.a

installdirs:
	$(SHELL) ${srcdir}/scripts/mkinstalldirs $(bindir) $(includedir) $(libdir) $(pixdir) $(mandir)

install: $(EXECS) installdirs $(PILIB) $(PICCLIB)
	$(INSTALL_DATA) $(PILIB) $(libdir)/libpisock.la
	if test -n "$(PICCLIB)"; then \
		$(INSTALL_DATA) $(PICCLIB) $(libdir)/libpicc.la; \
	fi
	for p in $(EXECS); do \
		$(INSTALL_PROGRAM) $$p $(bindir)/$$p; \
	done
	for h in include/*.h $(srcdir)/include/*.h include/*.hxx $(srcdir)/include/*.hxx; do \
		$(INSTALL_DATA) $$h $(includedir)/`basename $$h`; \
	done
	for m in man/*.1; do \
		$(INSTALL_DATA) $$m $(mandir)/`basename $$m`; \
	done
	for p in $(srcdir)/pix/*.gif $(srcdir)/pix/*.xbm; do \
		$(INSTALL_DATA) $$p $(pixdir)/`basename $$p`; \
	done
	$(LIBTOOL) -n --finish $(libdir)

uninstall:
	-for p in $(EXECS) $(PERLEXECS) $(CXXEXECS) $(TCLEXECS); do \
		$(RM) $(bindir)/$$p; \
	done
	-rmdir $(bindir)
	-for h in include/*; do \
		$(RM) $(includedir)/`basename $$h`; \
	done
	-rmdir $(includedir)
	-$(LIBTOOL) $(RM) $(libdir)/libpisock.la
	-$(LIBTOOL) $(RM) $(libdir)/libpicc.la
	-rmdir $(libdir)

install-memo$(EXT): $(PILIB) $(GETOPT) install-memo.o
	$(CCLINK) install-memo.o $(PILIB) $(GETOPT) -o $@ $(LIBS)

install-datebook$(EXT): $(PILIB) $(GETOPT) install-datebook.o parsedate.o
	$(CCLINK) install-datebook.o parsedate.o $(PILIB) $(GETOPT) -o $@ $(LIBS)

install-todos$(EXT): $(PILIB) $(GETOPT) install-todos.o
	$(CCLINK) install-todos.o $(PILIB) $(GETOPT) -o $@ $(LIBS)

pilot-addresses$(EXT): $(PILIB) $(GETOPT) pilot-addresses.o
	$(CCLINK) pilot-addresses.o $(PILIB) $(GETOPT) -o $@ $(LIBS)

pilot-dedupe$(EXT): $(PILIB) $(GETOPT) pilot-dedupe.o
	$(CCLINK) pilot-dedupe.o $(PILIB) $(GETOPT) -o $@ $(LIBS)

install-user$(EXT): $(PILIB) $(GETOPT) install-user.o
	$(CCLINK) install-user.o $(PILIB) $(GETOPT) -o $@ $(LIBS)

pilot-xfer$(EXT): $(PILIB) $(GETOPT) pilot-xfer.o
	$(CCLINK) pilot-xfer.o $(PILIB) $(GETOPT) -o $@ $(LIBS)

pilot-schlep$(EXT): $(PILIB) $(GETOPT) pilot-schlep.o
	$(CCLINK) pilot-schlep.o $(PILIB) $(GETOPT) -o $@ $(LIBS)

pilot-mail$(EXT): $(PILIB) $(GETOPT) pilot-mail.o parsedate.o
	$(CCLINK) pilot-mail.o parsedate.o $(PILIB) $(GETOPT) -o $@ $(LIBS)

pi-port$(EXT): $(PILIB) $(GETOPT) pi-port.o
	$(CCLINK) pi-port.o $(PILIB) $(GETOPT) -o $@ $(LIBS)

pi-getrom$(EXT): $(PILIB) $(GETOPT) pi-getrom.o
	$(CCLINK) pi-getrom.o $(PILIB) $(GETOPT) -o $@ $(LIBS)

pi-getram$(EXT): $(PILIB) $(GETOPT) pi-getram.o
	$(CCLINK) pi-getram.o $(PILIB) $(GETOPT) -o $@ $(LIBS)

pilot-file$(EXT): $(PILIB) $(GETOPT) pilot-file.o
	$(CCLINK) pilot-file.o $(PILIB) $(GETOPT) -o $@ $(LIBS)

pilot-clip$(EXT): $(PILIB) $(GETOPT) pilot-clip.o
	$(CCLINK) pilot-clip.o $(PILIB) $(GETOPT) -o $@ $(LIBS)

pilot-debug.o: pilot-debug.c Makefile
	$(CC) $(CFLAGS) -c pilot-debug.c

pd-tty.o: pd-tty.c Makefile
	$(CC) $(CFLAGS) -c pd-tty.c

pilot-debug$(EXT): $(PILIB) $(GETOPT) pilot-debug.o pd-tty.o
	$(CCLINK) pilot-debug.o pd-tty.o $(PILIB) $(GETOPT) -o $@ $(TCLTK_LIBS) $(RLLIBS) $(LIBS)

reminders$(EXT): $(PILIB) $(GETOPT) reminders.o
	$(CCLINK) reminders.o $(PILIB) $(GETOPT) -o $@ $(LIBS)

memos$(EXT): $(PILIB) $(GETOPT) memos.o
	$(CCLINK) memos.o $(PILIB) $(GETOPT) -o $@ $(LIBS)

read-todos$(EXT): $(PILIB) $(GETOPT) read-todos.o
	$(CCLINK) read-todos.o $(PILIB) $(GETOPT) -o $@ $(LIBS)

read-expenses$(EXT): $(PILIB) $(GETOPT) read-expenses.o
	$(CCLINK) read-expenses.o $(PILIB) $(GETOPT) -o $@ $(LIBS)

read-ical$(EXT): $(PILIB) $(GETOPT) read-ical.o
	$(CCLINK) read-ical.o $(PILIB) $(GETOPT) -o $@ $(LIBS)

addresses$(EXT): $(PILIB) $(GETOPT) addresses.o
	$(CCLINK) addresses.o $(PILIB) $(GETOPT) -o $@ $(LIBS)

dlpsh$(EXT): $(PILIB) $(GETOPT) dlpsh.o
	$(CCLINK) dlpsh.o $(PILIB) $(GETOPT) -o $@ $(LIBS)

debugsh$(EXT): $(PILIB) $(GETOPT) debugsh.o
	$(CCLINK) debugsh.o $(PILIB) $(GETOPT) -o $@ $(LIBS)

pi-csd$(EXT): $(PILIB) $(GETOPT) pi-csd.o
	$(CCLINK) pi-csd.o $(PILIB) $(GETOPT) -o $@ $(LIBS)

pi-nredir$(EXT): $(PILIB) $(GETOPT) pi-nredir.o
	$(CCLINK) pi-nredir.o $(PILIB) $(GETOPT) -o $@ $(LIBS)

ccexample$(EXT): $(PILIB) $(GETOPT) $(PICCLIB) ccexample.o
	$(CXXLINK) ccexample.o $(PILIB) $(GETOPT) $(PICCLIB) -o $@ $(CXXLIBS) $(LIBS)

validate$(EXT): $(PILIB) $(GETOPT) $(PICCLIB) validate.o
	$(CXXLINK) validate.o $(PILIB) $(GETOPT) $(PICCLIB) -o $@ $(CXXLIBS) $(LIBS)

iambicexample$(EXT): $(PILIB) $(GETOPT) $(PICCLIB) iambicexample.o
	$(CXXLINK) iambicexample.o $(PILIB) $(GETOPT) $(PICCLIB) -o $@ $(CXXLIBS) $(LIBS)

getrom$(EXT): $(PILIB) $(GETOPT) getrom.o
	$(CCLINK) getrom.o $(PILIB) $(GETOPT) -o $@ $(LIBS)

parsedate.c: parsedate.y
	@echo Expect six shift/reduce conflicts
	$(YACC) parsedate.y
	mv y.tab.c parsedate.c

pilot-undelete: pilot-undelete.PL
	$(PERL5) pilot-undelete.PL > pilot-undelete
	chmod +x pilot-undelete

ietf2datebook: ietf2datebook.PL
	$(PERL5) ietf2datebook.PL > ietf2datebook
	chmod +x ietf2datebook

sync-plan: sync-plan.PL
	$(PERL5) sync-plan.PL > sync-plan
	chmod +x sync-plan

FORCE:

Python:	FORCE
	cd Python; $(MAKE)

Perl5 Perl: FORCE
	cd Perl5; $(PERL5) Makefile.PL && $(MAKE)

Tcl: FORCE
	cd Tcl; $(MAKE)
	
Java: FORCE
	cd Java; $(MAKE)
	
check test tests: FORCE
	cd tests; $(MAKE) tests

strip: $(EXECS)
	$(STRIP) $(EXECS)

depend:
	@-p4 edit Makefile.in */Makefile.in 2>&1 >> /dev/null
	@-chmod +w */Makefile.in 2>&1 >> /dev/null
	$(SUBMAKE_COMM) depend
	$(SUBMAKE_COMM_DUBIOUS) depend
	$(SUBMAKE_COMM_CC) depend
	perl -i~ -ne 'print if 1../^#Depend/' Makefile.in ; gcc -w -MM -Iinclude *.c >> Makefile.in
	@-p4 revert `p4 diff -sr Makefile.in */Makefile.in` 2>&1 >> /dev/null 

os2:
	@-p4 edit Makefile.os2 */Makefile.os2 2>&1 >> /dev/null
	@-chmod +w Makefile.os2 */Makefile.os2 2>&1 >> /dev/null
	perl scripts/os2dist.pl
	@-p4 revert `p4 diff -sr Makefile.os2 */Makefile.os2` 2>&1 >> /dev/null 

distclean: clean depend os2
	$(RM) Makefile libsock/Makefile dubious/Makefile include/pi-config.h include/pi-sockaddr.h
	$(RM) libcc/Makefile
	$(RM) config.log config.status config.cache libtool
	$(RM) PiDebug.log
	-cd Perl5 ; $(MAKE) realclean ; rm Makefile.PL
	-cd Python ; $(MAKE) distclean
	-cd Tcl ; $(MAKE) clean
	-cd Java ; $(MAKE) distclean
	-cd tests ; $(MAKE) distclean
	$(RM) *~ *.orig */*~ */*.orig
	$(RM) Tcl/Makefile Perl5/Makefile.PL Perl5/Makefile Python/Makefile

clean:
	$(RM) *.o *.a *.lo *.la core a.out tarball test_s test_c install-prc$(EXT) pilot-debug$(EXT)
	$(RM) -rf .libs _libs
	$(RM) $(CEXECS) $(TCLEXECS) $(CXXEXECS) $(PERLEXECS) validate 
	$(RM) pilot.rom* pilot2.rom* sync-plan.ids.*
	$(SUBMAKE_COMM) clean
	$(SUBMAKE_COMM_DUBIOUS) clean
	$(SUBMAKE_COMM_CC) clean

tarball: distclean
	gcc -o tarball scripts/tarball.c -Iinclude
	./tarball

#Depend information starts here. Do not edit the text beyond this point!
addresses.o: addresses.c include/pi-source.h include/pi-config.h \
 include/pi-socket.h include/pi-args.h include/pi-version.h \
 include/pi-sockaddr.h include/pi-macros.h include/pi-address.h \
 include/pi-appinfo.h include/pi-dlp.h
debugsh.o: debugsh.c include/pi-source.h include/pi-config.h \
 include/pi-socket.h include/pi-args.h include/pi-version.h \
 include/pi-sockaddr.h include/pi-macros.h include/pi-dlp.h \
 include/pi-syspkt.h
dlpsh.o: dlpsh.c include/pi-source.h include/pi-config.h \
 include/pi-socket.h include/pi-args.h include/pi-version.h \
 include/pi-sockaddr.h include/pi-macros.h include/pi-padp.h \
 include/pi-dlp.h include/pi-serial.h
getopt.o: getopt.c
getopt1.o: getopt1.c getopt.h
getrom.o: getrom.c include/pi-source.h include/pi-config.h \
 include/pi-socket.h include/pi-args.h include/pi-version.h \
 include/pi-sockaddr.h include/pi-macros.h include/pi-serial.h
install-datebook.o: install-datebook.c include/pi-source.h \
 include/pi-config.h include/pi-socket.h include/pi-args.h \
 include/pi-version.h include/pi-sockaddr.h include/pi-macros.h \
 include/pi-dlp.h include/pi-datebook.h include/pi-appinfo.h
install-memo.o: install-memo.c include/pi-source.h include/pi-config.h \
 include/pi-socket.h include/pi-args.h include/pi-version.h \
 include/pi-sockaddr.h include/pi-macros.h include/pi-dlp.h \
 include/pi-memo.h include/pi-appinfo.h
install-todos.o: install-todos.c include/pi-source.h \
 include/pi-config.h include/pi-socket.h include/pi-args.h \
 include/pi-version.h include/pi-sockaddr.h include/pi-macros.h \
 include/pi-dlp.h include/pi-todo.h include/pi-appinfo.h
install-user.o: install-user.c include/pi-source.h include/pi-config.h \
 include/pi-socket.h include/pi-args.h include/pi-version.h \
 include/pi-sockaddr.h include/pi-macros.h include/pi-dlp.h
memos.o: memos.c include/pi-source.h include/pi-config.h \
 include/pi-socket.h include/pi-args.h include/pi-version.h \
 include/pi-sockaddr.h include/pi-macros.h include/pi-memo.h \
 include/pi-appinfo.h include/pi-dlp.h
parsedate.o: parsedate.c
pd-tty.o: pd-tty.c include/pi-source.h include/pi-config.h \
 include/pi-socket.h include/pi-args.h include/pi-version.h \
 include/pi-sockaddr.h include/pi-macros.h include/pi-dlp.h \
 include/pi-syspkt.h
pi-csd.o: pi-csd.c include/pi-source.h include/pi-config.h \
 include/pi-socket.h include/pi-args.h include/pi-version.h \
 include/pi-sockaddr.h include/pi-macros.h include/pi-dlp.h \
 include/pi-serial.h include/pi-slp.h
pi-getram.o: pi-getram.c include/pi-source.h include/pi-config.h \
 include/pi-socket.h include/pi-args.h include/pi-version.h \
 include/pi-sockaddr.h include/pi-macros.h include/pi-syspkt.h \
 include/pi-dlp.h
pi-getrom.o: pi-getrom.c include/pi-source.h include/pi-config.h \
 include/pi-socket.h include/pi-args.h include/pi-version.h \
 include/pi-sockaddr.h include/pi-macros.h include/pi-syspkt.h \
 include/pi-dlp.h
pi-nredir.o: pi-nredir.c include/pi-source.h include/pi-config.h \
 include/pi-socket.h include/pi-args.h include/pi-version.h \
 include/pi-sockaddr.h include/pi-macros.h include/pi-dlp.h
pi-port.o: pi-port.c include/pi-source.h include/pi-config.h \
 include/pi-socket.h include/pi-args.h include/pi-version.h \
 include/pi-sockaddr.h include/pi-macros.h include/pi-dlp.h \
 include/pi-serial.h include/pi-slp.h
pilot-addresses.o: pilot-addresses.c include/pi-source.h \
 include/pi-config.h include/pi-socket.h include/pi-args.h \
 include/pi-version.h include/pi-sockaddr.h include/pi-macros.h \
 include/pi-dlp.h include/pi-address.h include/pi-appinfo.h
pilot-clip.o: pilot-clip.c include/pi-source.h include/pi-config.h \
 include/pi-socket.h include/pi-args.h include/pi-version.h \
 include/pi-sockaddr.h include/pi-macros.h include/pi-syspkt.h \
 include/pi-dlp.h
pilot-debug.o: pilot-debug.c include/pi-source.h include/pi-config.h \
 include/pi-socket.h include/pi-args.h include/pi-version.h \
 include/pi-sockaddr.h include/pi-macros.h include/pi-dlp.h \
 include/pi-syspkt.h
pilot-dedupe.o: pilot-dedupe.c include/pi-source.h include/pi-config.h \
 include/pi-socket.h include/pi-args.h include/pi-version.h \
 include/pi-sockaddr.h include/pi-macros.h include/pi-dlp.h
pilot-file.o: pilot-file.c include/pi-source.h include/pi-config.h \
 include/pi-socket.h include/pi-args.h include/pi-version.h \
 include/pi-sockaddr.h include/pi-macros.h include/pi-dlp.h \
 include/pi-file.h
pilot-mail.o: pilot-mail.c include/pi-source.h include/pi-config.h \
 include/pi-socket.h include/pi-args.h include/pi-version.h \
 include/pi-sockaddr.h include/pi-macros.h include/pi-mail.h \
 include/pi-appinfo.h include/pi-dlp.h
pilot-prc.o: pilot-prc.c include/pi-source.h include/pi-config.h \
 include/pi-socket.h include/pi-args.h include/pi-version.h \
 include/pi-sockaddr.h include/pi-macros.h include/pi-dlp.h \
 include/pi-file.h
pilot-schlep.o: pilot-schlep.c include/pi-source.h include/pi-config.h \
 include/pi-socket.h include/pi-args.h include/pi-version.h \
 include/pi-sockaddr.h include/pi-macros.h include/pi-file.h \
 include/pi-dlp.h
pilot-xfer.o: pilot-xfer.c getopt.h include/pi-source.h \
 include/pi-config.h include/pi-socket.h include/pi-args.h \
 include/pi-version.h include/pi-sockaddr.h include/pi-macros.h \
 include/pi-file.h include/pi-dlp.h
read-expenses.o: read-expenses.c include/pi-source.h \
 include/pi-config.h include/pi-socket.h include/pi-args.h \
 include/pi-version.h include/pi-sockaddr.h include/pi-macros.h \
 include/pi-expense.h include/pi-appinfo.h include/pi-dlp.h
read-ical.o: read-ical.c include/pi-source.h include/pi-config.h \
 include/pi-socket.h include/pi-args.h include/pi-version.h \
 include/pi-sockaddr.h include/pi-macros.h include/pi-todo.h \
 include/pi-appinfo.h include/pi-datebook.h include/pi-dlp.h
read-todos.o: read-todos.c include/pi-source.h include/pi-config.h \
 include/pi-socket.h include/pi-args.h include/pi-version.h \
 include/pi-sockaddr.h include/pi-macros.h include/pi-todo.h \
 include/pi-appinfo.h include/pi-dlp.h
reminders.o: reminders.c include/pi-source.h include/pi-config.h \
 include/pi-socket.h include/pi-args.h include/pi-version.h \
 include/pi-sockaddr.h include/pi-macros.h include/pi-datebook.h \
 include/pi-appinfo.h include/pi-dlp.h
