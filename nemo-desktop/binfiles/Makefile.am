## We require new-style dependency handling.
AUTOMAKE_OPTIONS = 1.7

SUBDIRS=

NULL =

%:%.in Makefile
	$(AM_V_GEN)sed                  \
        -e s!\@datadir\@!$(datadir)!        \
        < $< > $@

all-local: nemo-desktop

bin_SCRIPTS = nemo-desktop

EXTRA_DIST =                \
    nemo-desktop.in \
    $(NULL)

MAINTAINERCLEANFILES =          \
    *~              \
    Makefile.in

-include $(top_srcdir)/git.mk
