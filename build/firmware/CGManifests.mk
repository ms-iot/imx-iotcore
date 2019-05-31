# Update the cgmanifest.json files located in each repo based on the current upstream values.
# Each cgmanifest file will include the contents of each cgmanifest file from the dependent
# repos.

# To add/modify a dependency:
#	Submodules:
#		Submodules are handled automatically, just run the update
#	Adding a new repo:
#		1) Add the repo name to the CG_MANIFEST_REPOS variable
#		3) Add an entry in the parent repo's ci/cgmanifest_template.json file
#		2) List the dependencies in a <REPO_NAME>_cgmanfiest_deps variable
#
# How to generate new cgmanifest files:
# Update all manifest files:
#		make cg_manifests
#
# Update a specific repo's manifest, AND update all its dependencies:
#		make <REPO_NAME>_cgmanifest
#		ex: make u-boot_cgmanifest
#
# Update a manifest WITHOUT updating any dependencies first:
#		make <REPO_NAME>_cgmanifest_nodep

# List of all repos to generate dependencies for:
CG_MANIFEST_REPOS= imx-iotcore u-boot optee_os RIoT imx-edk2-platforms edk2 edk2-libc MSRSec

# CG Manifest dependency Graph. List all implicit dependencies here. Submodules will be detected
# automatically.
imx-iotcore_cgmanifest_deps=u-boot optee_os imx-edk2-platforms
u-boot_cgmanifest_deps=RIoT
optee_os_cgmanifest_deps=
imx-edk2-platforms_cgmanifest_deps=edk2 edk2-libc MSRSec
RIoT_cgmanifest_deps=
edk2_cgmanifest_deps=
edk2-libc_cgmanifest_deps=
MSRSec_cgmanifest_deps=

#
# Rules and Variables
#

# Convert a rule to the corresponding repo name (ie. imx-iotcore_cgmanifest -> imx-iotcore)
rule_to_name=$(subst _cgmanifest,,$(subst _nodep,,$(1)))
# MSRSec does not have a ci/ folder, instead it uses external/
name_to_ci_path=$(abspath $(REPO_ROOT)/$(1)/$(if $(findstring MSRSec, $(1)),external/,ci/))

# Each repo listed in $(CG_MANIFEST_REPOS) will generate a set of rules, prerequisites and variables:
#	example: imx-iotcore gives:
#
#	Variables:
#		CURRENT_REPO_NAME = imx-iotcore
#		CURRENT_REPO_PATH = ../../../imx-iotcore/
#		CURRENT_TEMPLATE_PATH = ../../../imx-iotcore/ci/cgmanifest_template.json
#		CURRENT_DEPENDENT_REPO_NAMES = u-boot optee_os   etc...
#		CURRENT_DEPENDENT_RULES = u-boot_cgmanifest optee_os_cgmanifest   etc...
#		CURRENT_DEPENDENT_PATHS = ../../../u-boot/ci/cgmanifest.json ../../../optee_os/ci/cgmanifest.json  etc...
#		FINAL_MANIFEST_PATH = ../../../imx-iotcore/ci/cgmanifest.json
#
#	Rules:
#		imx-iotcore_cgmanifest: u-boot_cgmanifest optee_os_cgmanifest imx-edk2-platforms_cgmanifest
#		imx-iotcore_cgmanifest:
#			$(info ...
#		imx-iotcore_cgmanifest_nodep:
#			$(info ...

# Global variables
REPO_ROOT=../../..
CG_MANIFEST_RULES=$(foreach repo,$(CG_MANIFEST_REPOS),$(repo)_cgmanifest)
CG_MANIFEST_RULES_NODEP=$(foreach rule,$(CG_MANIFEST_RULES),$(rule)_nodep)
CG_MANIFEST_TARGETS=$(foreach repo,$(CG_MANIFEST_REPOS),$(REPO_ROOT)/$(repo)/ci/cgmanifest.json)
MANIFEST_SCRIPT=$(abspath $(REPO_ROOT)/imx-iotcore/ci/build_cgmanifest.py)

# Rule specific variables
CURRENT_REPO_NAME=$(call rule_to_name,$@)
CURRENT_REPO_PATH=$(abspath $(REPO_ROOT)/$(CURRENT_REPO_NAME))
CURRENT_TEMPLATE_PATH=$(abspath $(call name_to_ci_path,$(CURRENT_REPO_NAME))/cgmanifest_template.json)
CURRENT_DEPENDENT_REPO_NAMES=$(foreach name,$($(CURRENT_REPO_NAME)_cgmanifest_deps),$(name))
CURRENT_DEPENDENT_RULES=$(foreach name,$(CURRENT_DEPENDENT_REPO_NAMES),$(name)_cgmanifest)
CURRENT_DEPENDENT_PATHS=$(foreach name,$(CURRENT_DEPENDENT_REPO_NAMES),$(abspath $(call name_to_ci_path,$(name))/cgmanifest.json))
FINAL_MANIFEST_PATH=$(abspath $(call name_to_ci_path,$(CURRENT_REPO_NAME))/cgmanifest.json)

.PHONY: cg_manifests $(CG_MANIFEST_RULES) $(CG_MANIFEST_RULES_NODEP)
cg_manifests: $(CG_MANIFEST_RULES)

# Generate prerequesites of the form: <REPO_NAME>_cgmanifest: <REPO_DEPENDENCY1>_cgmanifest <REPO_DEPENDENCY2>_cgmanifest ... <REPO_NAME>_cgmanifest_nodep
# This will udpate all dependent repos first, then collect the registrations into the current manifest file.
#	The inner foreach loop takes its list from the dependency graph variables above using a computed variable name ( $(rule)_deps )
$(foreach rule,$(CG_MANIFEST_RULES),$(eval $(rule):$(foreach dep,$($(rule)_deps),$(dep)_cgmanifest) $(rule)_nodep))
$(CG_MANIFEST_RULES):
	$(info Recursive update of $@ complete)
	$(info Regenerated manifests for: $^)

# Copies contents of dependent *.json files and collects them into the current cgmanifest
# Does not update the dependent files first.
$(CG_MANIFEST_RULES_NODEP): 
	$(info Regenerating $(CURRENT_REPO_NAME) cgmanifest.json)
	$(info Template path: $(CURRENT_TEMPLATE_PATH))
	$(info Dependencies: $(CURRENT_DEPENDENT_REPO_NAMES))
	$(info Dependency paths: $(CURRENT_DEPENDENT_PATHS))
	rm -rf $(FINAL_MANIFEST_PATH)
	cd $(CURRENT_REPO_PATH) && python3 $(MANIFEST_SCRIPT) $(CURRENT_TEMPLATE_PATH) $(CURRENT_DEPENDENT_PATHS) $(FINAL_MANIFEST_PATH)