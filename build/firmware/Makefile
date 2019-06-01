# This Makefile inspired by http://lackof.org/taggart/hacking/make-example/

BOARDS=\
	ClSomImx7_iMX7D_1GB \
	HummingBoardEdge_iMX6DL_1GB \
	HummingBoardEdge_iMX6Q_2GB \
	HummingBoardEdge_iMX6S_512MB \
	Sabre_iMX6Q_1GB \
	Sabre_iMX6QP_1GB \
	Sabre_iMX6SX_1GB \
	Sabre_iMX7D_1GB \
	SabreLite_iMX6Q_1GB \
	UdooNeo_iMX6SX_1GB \
	NXPEVK_iMX8M_4GB \
	VAB820_iMX6Q_1GB \
	EVK_iMX6ULL_512MB \

UPDATEDIRS=$(BOARDS:%=update-%)
CLEANDIRS=$(BOARDS:%=clean-%)

all: $(BOARDS)
$(BOARDS):
	$(MAKE) -C $@

update-ffu: $(UPDATEDIRS)
$(UPDATEDIRS):
	$(MAKE) -C $(@:update-%=%) update-ffu

clean: $(CLEANDIRS)
$(CLEANDIRS):
	$(MAKE) -C $(@:clean-%=%) clean

include CGManifests.mk

# Print off a list of boards to display
.PHONY: print_board_list
print_board_list:
	@echo $(BOARDS) | tr " " "\n"

.PHONY: subdirs $(BOARDS)
.PHONY: subdirs $(UPDATEDIRS)
.PHONY: subdirs $(CLEANDIRS)
.PHONY: all update-ffu clean
