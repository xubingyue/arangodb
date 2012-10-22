# -*- mode: Makefile; -*-

## -----------------------------------------------------------------------------
## --SECTION--                                                   SPECIAL TARGETS
## -----------------------------------------------------------------------------

-include Makefile

################################################################################
### @brief setup
################################################################################

.PHONY: setup

setup:
	@echo ACLOCAL
	@aclocal -I m4
	@echo AUTOMAKE
	@automake --add-missing --force-missing --copy
	@echo AUTOCONF
	@autoconf -I m4
	@echo auto system configured, proceed with configure

################################################################################
### @brief add maintainer files
################################################################################

MAINTAINER = \
	README \
	arangod/Ahuacatl/ahuacatl-tokens.c \
	arangod/Ahuacatl/ahuacatl-grammar.c \
	arangod/Ahuacatl/ahuacatl-grammar.h \
	lib/JsonParser/json-parser.c \
	lib/V8/v8-json.cpp \
	lib/V8/v8-json.h \
	lib/BasicsC/voc-errors.h \
	lib/BasicsC/voc-errors.c \
	js/common/bootstrap/errors.js

AUTOMAGIC = \
	Makefile.in \
	aclocal.m4 \
	config/compile \
	config/config.guess \
	config/config.sub \
	config/depcomp \
	config/install-sh \
	config/missing

.PHONY: add-maintainer add-automagic

add-maintainer:
	@echo adding generated files to GIT
	git add -f $(MAINTAINER)

add-automagic:
	@echo adding automagic files to GIT
	git add -f $(AUTOMAGIC)

################################################################################
### @brief make love
################################################################################

love: 
	@echo ArangoDB loves you

## -----------------------------------------------------------------------------
## --SECTION--                                                       END-OF-FILE
## -----------------------------------------------------------------------------

## Local Variables:
## mode: outline-minor
## outline-regexp: "^\\(### @brief\\|## --SECTION--\\|# -\\*- \\)"
## End:
