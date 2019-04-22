# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import subprocess, os, sys, json

# This python script will dynamically generate a cgmanifest file based on a template (or generate from
# scratch if needed). It will also automatically populate the the cgmanifest file with the first level
# of submodules included in the current repo.
#
# The template file can include:
#   - explicit registrations (as defined in the Component Governance docs)
#   - git registrations which omit the 'commitHash' tag and replace it with a 'branch' tag
#   - no registrations at all (you can simpmly exclude the cgmanifest file completely in this case)
#
# The script will:
#   - Leave explicit registrations as is
#   - Convert all 'branch' tags to 'commitHash' tags based on a call to the upstream repo via 'git ls-remote'
#   - Add an entry for each git submodule not already added by either of the above steps
#
# Use: python3 build_cgmanifest.py [input.json [output.json]]
#   input.json: A template file which may include 0 or more registrations following the standard cgmanifest format
#                   Git registrations may omit the required 'commitHash' tag and replace it with a 'branch' tag.
#                   If the file is not found a default blank template is used.
#                   default: cgmanifest_template.json
#
#   output.json: The location to place the final json (must be named cgmanifest.json to be picked up by the tooling)
#                   Path doesn't matter, the tooling will search the entire repo for the file.
#                   default: cgmanifest.json

# Returns a list of tupples (URL, Commit hash) of registered first level submodules
def get_submodules():
    submodules = []
    git_process = subprocess.run("git rev-parse --show-toplevel",shell=True, stdout=subprocess.PIPE,  universal_newlines=True, timeout=10)
    git_process.check_returncode()
    repo_root = git_process.stdout.splitlines()[0]
    print("Checking for submodules in ", repo_root)

    if not os.path.isfile(os.path.join(repo_root, ".gitmodules")):
        print("No .gitmodules file found")
        return []

    # Submodules can have custom names, need to look up the names of submodules by path so we can look up the URL given a path
    cmd = "git config --file .gitmodules --get-regexp submodule.*path"
    git_process = subprocess.run(cmd, cwd=repo_root,shell=True, stdout=subprocess.PIPE,  universal_newlines=True, timeout=10)
    git_process.check_returncode()
    submodule_config_data = git_process.stdout
    name_lookup = dict()
    for line in submodule_config_data.splitlines():
        config_name = line.split()[0][:-5] # Remove '.path' from end of name
        config_path = line.split()[1]
        name_lookup[config_path] = config_name

    # Parse list of active submodules to get commit hash and relative path
    git_process = subprocess.run("git submodule", cwd=repo_root,shell=True, stdout=subprocess.PIPE,  universal_newlines=True, timeout=10)
    git_process.check_returncode()
    for line in git_process.stdout.splitlines():
        print (line)
        # Uninitialized repos will have a '-' infront of the hash
        hash = line.split()[0].strip('-')
        path = line.split()[1]

        # Given the path of the repo, find the custom name of the submodule and search for the URL
        name = name_lookup[path] + ".url"
        cmd = "git config --file .gitmodules --get-regexp " + name
        submodule_process = subprocess.run(cmd, cwd=repo_root, shell=True, stdout=subprocess.PIPE,  universal_newlines=True, timeout=10)
        submodule_process.check_returncode()
        url = submodule_process.stdout.splitlines()[0].split()[1]

        # Create tuple to be added to json
        repo = (url, hash)
        submodules.append(repo)

    return submodules

# Replaces branch tags in a repo object with the latest commit hash for that branch
def update_git_repo(repo):
    branch = repo.pop('branch')
    arg = "ls-remote " + repo['repositoryUrl'] + " " + branch

    git_process = subprocess.run("git " + arg, shell=True, stdout=subprocess.PIPE,  universal_newlines=True, timeout=10)
    git_process.check_returncode()

    hash = git_process.stdout.splitlines()[0].split()[0]
    repo['commitHash'] = hash

    print("\t"+repo['repositoryUrl'], branch)
    print("\tUpstream is on commit", hash)

# Searches for and updates all git repos which have branches instead of commit hashes
def update_registration(registration):
    component = registration['component']
    if component['type'] == 'git':
        git = component['git']
        if 'commitHash' not in git:
            print("Found registration with no commitHash tag")
            update_git_repo(git)

# Adds a new git component to the registrations
def add_new_registration(registrations, url, hash):
    new_registration = dict()    
    new_component = dict()
    new_git = dict()

    new_git['repositoryUrl'] = url
    new_git['commitHash'] = hash

    new_component['type'] = 'git'
    new_component['git'] = new_git

    new_registration['component'] = new_component

    registrations.append(new_registration)

# Return true if a (url,hash) pair already exists in the json
def check_existing_registration(registrations, url, hash):
    # Strip .git from URLs
    if url.endswith('.git'):
        url = url[:-4]

    print("Checking submodule", url, "with hash:", hash)
    for registration in registrations:
            component = registration['component']
            if component['type'] == 'git':
                git = component['git']

                # Strip .git from URLs
                component_url = git['repositoryUrl']
                if component_url.endswith('.git'):
                    component_url = component_url[:-4]

                if (('commitHash' in git) and 
                        (git['commitHash'] == hash) and
                        (component_url == url)):
                    return True
    return False

def default_cgmanifest():
    new_manifest = dict()
    new_manifest['Registrations'] = list()
    new_manifest['Version'] = 1

    return new_manifest

def update_json(json_file_in, json_file_out):
    #Load json, then search for any repos with branches.
    if json_file_in is not None:
        print("Opening file ", os.path.abspath(os.path.join(os.getcwd(), json_file_in)))
        with open(json_file_in, 'r') as f_in:
            cgmanifest = json.load(f_in)
    else:
        print("Starting with blank cgmanifest")
        cgmanifest = default_cgmanifest()

    registrations = cgmanifest["Registrations"]

    # Update existing registrations to replace 'branch' with 'commitHash'
    for registration in registrations:
        update_registration(registration)

    # Add any missing submodules
    for submodule in get_submodules():
        if not check_existing_registration(registrations, submodule[0], submodule[1]):
            print("\tRegistering new submodule")
            add_new_registration(registrations, submodule[0], submodule[1])
        else:
            print("\tSubmodule already registered")

    print("Writing back to file ", os.path.abspath(os.path.join(os.getcwd(), json_file_out)))
    with open(json_file_out, 'w') as f_out:
        json.dump(cgmanifest, f_out, indent=4)


json_file_in = "cgmanifest_template.json"
json_file_out = "cgmanifest.json"

num_args = len(sys.argv)
if num_args == 1:
    print("Using default filenames")
if num_args >= 2:
    json_file_in = sys.argv[1]
if num_args == 3:
    json_file_out = sys.argv[2]
if num_args > 3:
    print("\n\n\t\tUse: build_cgmanifest.py [input.json [output.json]]\n\n")
    raise ValueError("Too many arguments")

if not os.path.isfile(json_file_in):
    json_file_in = None

update_json(json_file_in, json_file_out)