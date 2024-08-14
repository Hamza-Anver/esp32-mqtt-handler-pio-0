import os
import gzip
import re

# ---------------------------------------------------------------------------- #
#                            PREPROCESSING FUNCTIONS                           #
# ---------------------------------------------------------------------------- #

def extract_definitions(header_file):
    definitions = {}
    with open(header_file, 'r') as file:
        for line in file:
            match = re.match(r'#define\s+(\w+)\s+"(.+)"', line)
            if match:
                key, value = match.groups()
                definitions[key] = value
    return definitions

def replace_placeholders(html_file, definitions, dest_file):
    with open(html_file, 'r') as file:
        html = file.read()
        for key, value in definitions.items():
            html = html.replace(f"{{{{{key}}}}}", value)
    
    with open(dest_file, 'w') as file:
        file.write(html)

def compress_page_as_header(src_path, dest_path):
    if os.path.exists(src_path):
        # Get the last modified time of the file
        if os.path.exists(dest_path):
            src_time = os.path.getmtime(src_path)
            dest_time = os.path.getmtime(dest_path)
            if src_time < dest_time:
                print("No changes to webpage detected")
                return
            else:
                print("Modifying webpage.h")
        else:
            print("Making new webpage.h")

        html_to_progmem(src_path, dest_path, "HTML_GZ_PROGMEM")
    else:
        print("ERROR: Source webpage not found!")


def html_to_progmem(html_file_path, output_cpp_file, variable_name="html_gz"):
    # Read the HTML file
    with open(html_file_path, "rb") as f:
        html_content = f.read()

    # Compress the HTML content using gzip
    compressed_content = gzip.compress(html_content)

    # Convert the compressed content to a C++ byte array
    hex_array = ", ".join(f"0x{byte:02X}" for byte in compressed_content)

    # Create the C++ code
    cpp_code = f"""
#include <Arduino.h>
#include <pgmspace.h>
#include "configwebpage/webpage/webpage.h"

const size_t {variable_name}_LEN = {len(compressed_content)};

const uint8_t {variable_name}[] PROGMEM = {{
{hex_array}
}};

"""

    # Write the C++ code to a file
    with open(output_cpp_file, "w") as f:
        f.write(cpp_code)

    print(f"C++ header file '{output_cpp_file}' has been created successfully!")
    print(f"Compressed size: {len(compressed_content)} bytes")



# ---------------------------------------------------------------------------- #
#                                 PREPROCESSING                                #
# ---------------------------------------------------------------------------- #

script_dir = "lib/comms_handler/src/configwebpage/webpage"

src_html_path = os.path.join(script_dir, "websrc.html")
temp_html_path = os.path.join(script_dir, "prepped.html")
dest_html_path = os.path.join(script_dir, "webpage.cpp")

src_header_endpoints = os.path.join(script_dir, "endpoints.h")

config_dir = "lib/comms_handler/src/config"
src_header_keys = os.path.join(config_dir, "configkeys.h")

src_dir = "src/"
src_metadata = os.path.join(src_dir, "metadata.h")


def preprocess_webpage(env):
    metaheader_path = os.path.join(env["PROJECT_INCLUDE_DIR"], "metaheaders.h")
    lib_dir = os.path.join(env["PROJECT_DIR"], "lib/comms_handler/src")
    src_dir = env["PROJECT_DIR"]
    if(os.path.exists(metaheader_path)):
        
        metaheaders_dirs = extract_definitions(metaheader_path)
        header_defs = []
        for key, value in metaheaders_dirs.items():
            # Find the appropriate header file from value
            lib_path = os.path.join(lib_dir, value)
            src_path = os.path.join(src_dir, value)
            if os.path.exists(lib_path):
                print(f"Found metaheader [{key}] in directory '{lib_path}'")
                header_defs.append(extract_definitions(lib_path))
            elif os.path.exists(src_path):
                print(f"Found metaheader [{key}] in directory '{src_path}'")
                header_defs.append(extract_definitions(src_path))
            else:
                print(f"WARNING: Metaheader directory '{lib_path}' and '{src_path}' not found!")

        # Output all the header definitions
        print("Found the following header definitions:")
        for header in header_defs:
            for key, value in header.items():
                print(f"  {key} : {value}")

        # Replace placeholders in the source HTML file
        replace_placeholders(src_html_path, header_defs[0], temp_html_path)
        for header in header_defs:
            replace_placeholders(temp_html_path, header, temp_html_path)
        
        # Compress the HTML file as a header
        compress_page_as_header(temp_html_path, dest_html_path)
        
    else:
        raise Exception("ERROR: metaheaders.h not found!")

Import("env") # type: ignore
preprocess_webpage(env) # type: ignore

