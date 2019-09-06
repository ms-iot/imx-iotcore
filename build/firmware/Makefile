# This Makefile inspired by http://lackof.org/taggart/hacking/make-example/

IMX67_BOARDS=\
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
	VAB820_iMX6Q_1GB \
	EVK_iMX6ULL_512MB \

IMX8_BOARDS=\
	NXPEVK_iMX8M_4GB \
	NXPEVK_iMX8M_Mini_2GB \

UPDATEDIRS=$(IMX67_BOARDS:%=update-%)
COMMITDIRS=$(IMX67_BOARDS:%=commit-%)
CLEANDIRS=$(IMX67_BOARDS:%=clean-%)
INITDIRS=$(IMX67_BOARDS:%=init-%)

#Serialize since some builds are mutually exclusive (imx8, EKD2)
# serialize_builds(command, common arguments, serialized list)
define serialize_builds
	$(foreach PARAM, $3, \
	$1 $2 $(PARAM) && \
	) true
endef

all: imx67_boards_serialized
imx8_all: imx8_boards_serialized

#Allows for a call to specific boards ie: "make NXPEVK_iMX8M_4GB", but will not enforce serialization of the builds
$(IMX67_BOARDS):
	$(MAKE) -C $@

$(IMX8_BOARDS):
	$(MAKE) imx8_build IMX8_TARGET=$@

imx67_boards_serialized: #init-dirs
	$(call serialize_builds,$(MAKE),-C,$(foreach BOARD,$(IMX67_BOARDS),$(BOARD)))

imx8_boards_serialized:
	$(call serialize_builds,$(MAKE) -f imx8.mk, imx8_build, $(foreach BOARD,$(IMX8_BOARDS),IMX8_TARGET=$(BOARD)))

#Checks for the case sensitivity of all output folders all at once.
#Force serial build using $(foreach to avoid too many pop-ups which can cause some to fail.
init-dirs:
	$(foreach BOARD, $(INITDIRS), \
	$(MAKE) -C $(BOARD:init-%=%) verify_case_sensitivity && \
	sleep 1 && \
	) true

update-ffu: $(UPDATEDIRS)
$(UPDATEDIRS):
	$(MAKE) -C $(@:update-%=%) update-ffu

#IMX8 requires a re-build to do this.
imx8_update-ffu:
	$(call serialize_builds,$(MAKE) -f imx8.mk, imx8_update-ffu, $(foreach BOARD,$(IMX8_BOARDS),IMX8_TARGET=$(BOARD)))

commit-firmware: $(COMMITDIRS)
$(COMMITDIRS):
	$(MAKE) -C $(@:commit-%=%) commit-firmware

#IMX8 requires a re-build to do this.
imx8_commit-firmware:
	$(call serialize_builds, $(MAKE) -f imx8.mk, imx8_commit-firmware, $(foreach BOARD,$(IMX8_BOARDS),IMX8_TARGET=$(BOARD)))

clean: $(CLEANDIRS)
$(CLEANDIRS):
	$(MAKE) -C $(@:clean-%=%) clean

include CGManifests.mk

# Print off a list of boards to display
.PHONY: print_board_list
print_board_list:
	@echo $(BOARDS) | tr " " "\n"

.PHONY: $(IMX67_BOARDS) $(IMX8_BOARDS) $(INITDIRS) $(UPDATEDIRS) $(CLEANDIRS)
.PHONY: all update-ffu commit-ffu clean init-dirs imx67_boards_serialized
.PHONY: imx8_all imx8_update-ffu imx8_commit-firmware imx8_boards_serialized
