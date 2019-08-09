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
COMMITDIRS=$(BOARDS:%=commit-%)
CLEANDIRS=$(BOARDS:%=clean-%)
INITDIRS=$(BOARDS:%=init-%)

all: $(BOARDS)
$(BOARDS): init-dirs
	$(MAKE) -C $@

#Checks for the case sensitivity of all output folders all at once.
init-dirs: $(INITDIRS)
$(INITDIRS):
	$(MAKE) -C $(@:init-%=%) verify_case_sensitivity

update-ffu: $(UPDATEDIRS)
$(UPDATEDIRS):
	$(MAKE) -C $(@:update-%=%) update-ffu

commit-firmware: $(COMMITDIRS)
$(COMMITDIRS):
	$(MAKE) -C $(@:commit-%=%) commit-firmware

clean: $(CLEANDIRS)
$(CLEANDIRS):
	$(MAKE) -C $(@:clean-%=%) clean

include CGManifests.mk

# Print off a list of boards to display
.PHONY: print_board_list
print_board_list:
	@echo $(BOARDS) | tr " " "\n"

.PHONY: $(BOARDS) $(INITDIRS) $(UPDATEDIRS) $(CLEANDIRS)
.PHONY: all update-ffu commit-ffu clean init-dirs
