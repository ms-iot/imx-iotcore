# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import subprocess, os, sys, json, getopt

# This python script will dynamically generate a cgmanifest file based on a set of templates
# (or generate from scratch if needed). It will also automatically populate the the cgmanifest
# file with the first level of submodules included in the current repo.
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
# Use: python3 build_cgmanifest.py [-g graph_file] [-i input1.json -i input2.json ...] [-o output.json]
#   -g:     A file to print dependency information (formated for future use in graphviz), defaults to 'None'
#
#   -i: A template file which may include 0 or more registrations following the standard 
#           cgmanifest format. Git registrations may omit the required 'commitHash' tag and replace
#           it with a 'branch' tag. If the file is not found a default blank template is used.
#           If no arguments are passed a default is used: cgmanifest_template.json. May be passed multiple
#           times, the templates will be unioned together.
#
#   -o: The location to place the final json (must be named cgmanifest.json to be picked up by the tooling)
#           Path doesn't matter, the tooling will search the entire repo for the file.
#           If no arguments are passed a default is used: cgmanifest.json

def get_repo_name():
    git_process = subprocess.run("git rev-parse --show-toplevel",shell=True, stdout=subprocess.PIPE,  universal_newlines=True, timeout=10)
    git_process.check_returncode()
    repo_root = git_process.stdout.splitlines()[0]
    repo_name = repo_root.rsplit(r'/',1)[1]
    return repo_name

def get_repo_url():
    git_process = subprocess.run("git config --get remote.upstream.url",shell=True, stdout=subprocess.PIPE,  universal_newlines=True, timeout=10)
    try:
        git_process.check_returncode()
    except:
        # Try again using origin instead of upstream
        git_process = subprocess.run("git config --get remote.origin.url",shell=True, stdout=subprocess.PIPE,  universal_newlines=True, timeout=10)
    git_process.check_returncode()
    repo_url = git_process.stdout.splitlines()[0]
    if repo_url.endswith('.git'):
        repo_url = repo_url[:-4]
    return repo_url

def get_repo_commit():
    git_process = subprocess.run("git rev-parse HEAD",shell=True, stdout=subprocess.PIPE,  universal_newlines=True, timeout=10)
    git_process.check_returncode()
    repo_commit = git_process.stdout.splitlines()[0]
    return repo_commit

def add_to_graph(dependency, commit, graph_edges):
    if dependency.endswith('.git'):
        dependency = dependency[:-4]
    # TODO: Figure out a better way to get commits, nothing matches currently and the graph fails
    #graph_edges.append('"%s\n(%s)" -> "%s\n(%s)";' % (dependency, commit, get_repo_url(), get_repo_commit()))
    graph_edges.append('"%s" -> "%s";' % (dependency.lower(), get_repo_url().lower()))

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
def add_new_registration(registrations, url, hash, graph_edges):
    new_registration = dict()
    new_component = dict()
    new_git = dict()

    new_git['repositoryUrl'] = url
    new_git['commitHash'] = hash

    new_component['type'] = 'git'
    new_component['git'] = new_git

    new_registration['component'] = new_component

    add_to_graph(url, hash, graph_edges)
    registrations.append(new_registration)

# Return true if a (url,hash) pair already exists in the json
# Add any existing edges to the graph
def check_existing_registration(registrations, url, hash, graph_edges):
    # Strip .git from URLs
    if url.endswith('.git'):
        url = url[:-4]

    print("Checking if", url, "with hash:", hash, "is already registered")
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
                    add_to_graph(component_url, hash, graph_edges)
                    return True
    return False

def default_cgmanifest():
    new_manifest = dict()
    new_manifest['Registrations'] = list()
    new_manifest['Version'] = 1

    return new_manifest

def update_json(json_input_list, json_file_out, graph_file_out):
    print("Starting with blank cgmanifest")
    cgmanifest = default_cgmanifest()
    existing_registrations = cgmanifest["Registrations"]
    graph_edges = []

    for json_file_in in json_input_list:
        #Load json, then search for any repos with branches.
        if os.path.isfile(json_file_in):
            print("Opening file ", os.path.abspath(os.path.join(os.getcwd(), json_file_in)))
            with open(json_file_in, 'r') as f_in:
                new_cgmanifest = json.load(f_in)

            new_registrations = new_cgmanifest["Registrations"]

            # Update existing registrations to replace 'branch' with 'commitHash', then place into
            # final manifest if they are not already present
            for new_registration in new_registrations:
                update_registration(new_registration)
                new_url = new_registration['component']['git']['repositoryUrl']
                new_hash = new_registration['component']['git']['commitHash']
                if not check_existing_registration(existing_registrations, new_url, new_hash, graph_edges):
                    print("\tRegistering new dependency")
                    add_new_registration(existing_registrations, new_url, new_hash, graph_edges)
                else:
                    print("\t Skipping")

        else:
            print("Can't find", json_file_in, ", skipping it")

    # Add any missing submodules
    for submodule in get_submodules():
        if not check_existing_registration(existing_registrations, submodule[0], submodule[1], graph_edges):
            print("\tRegistering new submodule")
            add_new_registration(existing_registrations, submodule[0], submodule[1], graph_edges)
        else:
            print("\tSubmodule already registered")

    print("New dependencies for graph: %s" % str(graph_edges))
    if graph_file_out is not None:
        full_graph_path = os.path.abspath(graph_file_out)
        print("Appending graph data to %s" % full_graph_path)
        with open(full_graph_path, "a") as graph_file:
            for edge in graph_edges:
                graph_file.write(edge + "\n")

    full_output_path = os.path.abspath(os.path.join(os.getcwd(), json_file_out))
    output_directory = os.path.dirname(full_output_path)
    if len(existing_registrations) > 0:
        print("Found", len(existing_registrations), "registrations!")
        print("Writing back to file ", full_output_path)

        if not os.path.exists(output_directory):
            print("Creating directory", output_directory)
            os.makedirs(output_directory)

        with open(full_output_path, 'w') as f_out:
            json.dump(cgmanifest, f_out, indent=4)
    else:
        print("Found no registrations to add!")
        if os.path.isfile(full_output_path):
            print("Clearing old cgmanifest file", full_output_path)
            os.remove(full_output_path)

try:
    opts,args = getopt.getopt(sys.argv[1:],'g:i:o:')
except getopt.GetoptError:
    print("\n\n\t\tUse: build_cgmanifest.py [-g graph_file] [ -i input1.json -i ... ] [-o output.json]\n\n")
    sys.exit(-1)

if len(args) > 0:
    print("\n\n\t\tUse: build_cgmanifest.py [-g graph_file] [ -i input1.json -i ... ] [-o output.json]\n\n")
    sys.exit(-1)

json_input_list = []
# Set default output
json_file_out = "cgmanifest.json"
graph_file = None

for opt, arg in opts:
    if opt == '-g':
        graph_file = arg
        print("Graph file is %s" % graph_file)
    elif opt == '-i':
        json_input_list.append(arg)
        print("Input files are %s" % str(json_input_list))
    elif opt == '-o':
        json_file_out = arg
        print("Output file is %s" % json_file_out)

# Set default input
if len(json_input_list) == 0:
    json_input_list = ["cgmanifest_template.json",]

print(json_input_list)
update_json(json_input_list, json_file_out, graph_file)