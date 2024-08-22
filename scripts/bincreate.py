import os, re, shutil, json

# This script is to be used for debug purposes only
# It will create a JSON file with the appropriate information and save it along with one bin file


def copy_bin_to_loc(binpath, destpath):
    print(f"Copying '{binpath}' to '{destpath}'")
    shutil.copy(binpath, destpath)

def extract_definitions(header_file):
    definitions = {}
    with open(header_file, 'r') as file:
        for line in file:
            match = re.match(r'#define\s+(\w+)\s+"?([^"\s]+)"?', line)

            if match:
                key, value = match.groups()
                if(value.isdigit()):
                    definitions[key] = int(value)
                else:
                    definitions[key] = value
    return definitions


def after_build(source, target, env):
    # Get the path to the binary
    binpath = env.subst("$BUILD_DIR/${PROGNAME}.bin") # type: ignore

    # Get the destination file name
    version_header = env.subst("$PROJECT_DIR/include/version.h")    # type: ignore

    version_defs = extract_definitions(version_header)

    # Get the destination path
    dest_name = version_defs["VERSION_ENVIRONMENT"] + ".bin"
    newbin_dir = os.path.join(env["PROJECT_DIR"],"binfiles", version_defs["VERSION_ENVIRONMENT"]) # type: ignore
    
    if not os.path.exists(newbin_dir):
        os.makedirs(newbin_dir)
    else:
        # Delete folder
        shutil.rmtree(newbin_dir)
        os.makedirs(newbin_dir)

    copy_bin_to_loc(binpath, os.path.join(newbin_dir,dest_name)) # type: ignore

    "https://github.com/Hamza-Anver/esp32-mqtt-handler-pio-0/raw/main/binfiles/nb-iot-nodemcu-32s-4mb/nb-iot-nodemcu-32s-4mb.bin"
    binurl = "https://github.com/Hamza-Anver/esp32-mqtt-handler-pio-0/raw/"+version_defs["VERSION_GIT_BRANCH"]+"/binfiles/" + version_defs["VERSION_ENVIRONMENT"] + '/' + dest_name

    jsondata = {
        "name": version_defs["VERSION_STRING"],
        "major": version_defs["VERSION_MAJOR"],
        "minor": version_defs["VERSION_MINOR"],
        "patch": version_defs["VERSION_PATCH"],
        "env": version_defs["VERSION_ENVIRONMENT"],
        "binurl": binurl
    }

    newjson_loc = os.path.join(env["PROJECT_DIR"],"binfiles", version_defs["VERSION_ENVIRONMENT"], "ota.json") # type: ignore
    
    with open(newjson_loc, 'w') as jsonfile:
        json.dump(jsondata, jsonfile, indent=4)

    json_formatted_str = json.dumps(jsondata, indent=4)
    print("OTA JSON FIlE: ")
    print(json_formatted_str)


Import("env") # type: ignore
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", after_build) # type: ignore