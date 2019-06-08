CFLAGS  = $(CFLAGSIN)

SIMPLE_OBJS   = charsets.o \
                is_ws.o \
                strsearch.o

NEWSTR_OBJS   = entities.o \
                gb18030.o \
                latex.o \
                str.o \
                str_conv.o \
                unicode.o \
                utf8.o

CONTAIN_OBJS  = fields.o \
                intlist.o \
                slist.o \
                vplist.o \
                xml.o \
                xml_encoding.o

BIBL_OBJS     = bibl.o \
                bu_auth.o \
                iso639_1.o \
                iso639_2.o \
                iso639_3.o \
                marc_auth.o \
                name.o \
                notes.o \
                pages.o \
                reftypes.o \
                serialno.o \
                title.o \
                url.o

INPUT_OBJS    = bibtexin.o \
                bibtextypes.o \
		biblatexin.o \
                bltypes.o \
		copacin.o \
                copactypes.o \
                ebiin.o \
		endin.o \
                endtypes.o \
		endxmlin.o \
		generic.o \
		isiin.o \
                isitypes.o \
                marc_auth.o \
		medin.o \
		modsin.o \
                modstypes.o \
		nbibin.o \
		nbibtypes.o \
		risin.o \
                ristypes.o \
		wordin.o

OUTPUT_OBJS   = adsout.o \
                bibtexout.o \
                endout.o \
                isiout.o \
                modsout.o \
		nbibout.o \
                risout.o \
                wordout.o

BIBCORE_OBJS  = $(SIMPLE_OBJS) \
                $(NEWSTR_OBJS) \
                $(CONTAIN_OBJS) \
                $(BIBL_OBJS) \
                bibcore.o

BIBUTILS_OBJS = $(INPUT_OBJS) \
                $(OUTPUT_OBJS) \
                bibutils.o

all: libbibcore.a libbibutils.a

libbibcore.a: $(BIBCORE_OBJS)
	ar -cr $@ $^
	$(RANLIB) $@

libbibutils.a: $(BIBUTILS_OBJS)
	ar -cr $@ $^
	$(RANLIB) $@

clean: 
	rm -f *.o core

realclean:
	rm -f *.o *.a core

install:

test:
