import os
import gzip


# This path may not always be right
src_html_path = "lib/comms_handler/src/webpage/websrc.html"
dest_html_path = "lib/comms_handler/src/webpage/webpage.h"


def auto_create_webpagecpp():
    if os.path.exists(src_html_path):
        # Get the last modified time of the file
        if os.path.exists(dest_html_path):
            src_time = os.path.getmtime(src_html_path)
            dest_time = os.path.getmtime(dest_html_path)
            if src_time < dest_time:
                print("No changes to webpage detected")
                #return
            else:
                print("Modifying webpage.cpp")
        else:
            print("Making new webpage.cpp")

        html_to_progmem(src_html_path, dest_html_path, "HTML_GZ_PROGMEM")
    else:
        print("Source webpage not found")


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
#ifndef WEBPAGE_GZIP_H
#define WEBPAGE_GZIP_H

const size_t {variable_name}_LEN = {len(compressed_content)};

const uint8_t {variable_name}[] PROGMEM = {{
{hex_array}
}};



#endif
"""

    # Write the C++ code to a file
    with open(output_cpp_file, "w") as f:
        f.write(cpp_code)

    print(f"C++ file '{output_cpp_file}' has been created successfully!")
    print(f"Compressed size: {len(compressed_content)} bytes")

auto_create_webpagecpp()
