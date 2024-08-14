import os
import re
import shutil

def copy_bin_to_loc(binpath, destpath):
    print(f"Copying '{binpath}' to '{destpath}'")
    shutil.copy(binpath, destpath)

def extract_definitions(header_file):
    definitions = {}
    with open(header_file, 'r') as file:
        for line in file:
            match = re.match(r'#define\s+(\w+)\s+"(.+)"', line)
            if match:
                key, value = match.groups()
                definitions[key] = value
    return definitions


def after_build(source, target, env):
    # Get the path to the binary
    binpath = env.subst("$BUILD_DIR/${PROGNAME}.bin") # type: ignore

    # Get the destination file name
    version_header = env.subst("$PROJECT_DIR/include/version.h")    # type: ignore

    version_defs = extract_definitions(version_header)

    # Get the destination path
    dest_name = version_defs["VERSION_STRING_NO_SPACES"] + ".bin"
    newbin_dir = os.path.join(env["PROJECT_DIR"],"binfiles") # type: ignore
    if not os.path.exists(newbin_dir):
        os.makedirs(newbin_dir)
    copy_bin_to_loc(binpath, os.path.join(env["PROJECT_DIR"],"binfiles",dest_name)) # type: ignore

Import("env") # type: ignore
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", after_build) # type: ignore