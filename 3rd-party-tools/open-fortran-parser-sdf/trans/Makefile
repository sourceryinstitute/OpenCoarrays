# Example usage:
#
#    make SOURCE=foo.f90 OFP_HOME=/opt/ofp-sdf check >  ocaf-foo.f90
#    make OFP_HOME=/opt/ofp-sdf clean
#

include $(OFP_HOME)/make.inc

SYNTAX_DIR = $(OFP_HOME)/fortran/syntax
TRANS_DIR  = $(OFP_HOME)/fortran/trans
PP_DIR     = $(OFP_HOME)/fortran/pp

TBL     = $(SYNTAX_DIR)/Fortran.tbl
TO_SIM  = $(TRANS_DIR)/ofp-simplify
TO_FAST = $(TRANS_DIR)/ofp2fast
TO_PP   = $(TRANS_DIR)/fast2pp
PP      = $(PP_DIR)/Fortran.pp

CFLAGS = $(ST_CFLAGS)  -I $(SYNTAX_DIR) -I $(TRANS_DIR)
LDFLAGS = $(ST_LDFLAGS) -la stratego-lib

## Fortran targets
#  ---------------

FC = ifort
FCFLAGS = -g -I $(FOR_OCL_DIR)

%.o: %.f90
	@echo 'Building for OS $(OSUPPER) file: $<'
	$(FC) -c $(FCFLAGS) -o $@ $<


all : fast-to-ocaf

caf-lower : caf-lower.str
	$(ST_PATH)/strc -i caf-lower.str $(CFLAGS) $(LDFLAGS) --main io-caf-lower

fast-to-ocaf : fast-to-ocaf.str
	$(ST_PATH)/strc -i fast-to-ocaf.str $(CFLAGS) $(LDFLAGS) --main io-fast-to-ocaf

check : fast-to-ocaf
	@$(ST_PATH)/sglri -p $(TBL) -i $(SOURCE)| $(TO_FAST) | ./fast-to-ocaf | $(TO_PP) | $(ST_PATH)/ast2text -p $(PP)

check-orig : fast-to-ocaf
	@$(ST_PATH)/sglri -p $(TBL) -i $(SOURCE).orig | $(TO_FAST) | ./fast-to-ocaf | $(TO_PP) | $(ST_PATH)/ast2text -p $(PP)

check-fast : fast-to-ocaf
	$(ST_PATH)/sglri -p $(TBL) -i $(SOURCE) | $(TO_FAST) | $(ST_PATH)/pp-aterm

check-last : fast-to-ocaf
	$(ST_PATH)/sglri -p $(TBL) -i $(SOURCE) | $(TO_FAST) | ./fast-to-ocaf | $(ST_PATH)/pp-aterm

check-lower : caf-lower
	$(ST_PATH)/sglri -p $(TBL) -i $(SOURCE) | $(TO_FAST) | ./caf-lower | $(TO_PP) | $(ST_PATH)/ast2text -p $(PP)

clean :
	rm -f caf-lower caf-lower.c
	rm -f fast-to-ocaf fast-to-ocaf.c
	rm -f *.o *.lo *.dep .libs/*.o
