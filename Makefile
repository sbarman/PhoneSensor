BIN = $(CURDIR)/bin
export BIN
 
.PHONY: clean distclean
SCOPE = phonescope
FUNC = functiongen
SRCDIR = src/c

all: $(SCOPE) $(FUNC)

$(SCOPE) :
	cd $(SRCDIR)/$(SCOPE) && $(MAKE)
$(FUNC) :
	cd $(SRCDIR)/$(FUNC) && $(MAKE)
	
clean:
	cd $(SRCDIR)/$(SCOPE) && $(MAKE) clean
	cd $(SRCDIR)/$(FUNC) && $(MAKE) clean

.PHONY: clean
