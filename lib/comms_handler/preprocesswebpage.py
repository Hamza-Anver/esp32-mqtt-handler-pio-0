import os
import time

# This path may not always be right
src_html_path = "lib/comms_handler/src/webpage/websrc.html"
dest_html_path = "lib/comms_handler/src/webpage/webpage.cpp"

def auto_create_webpagecpp():
    if os.path.exists(src_html_path):
        # Get the last modified time of the file
        if(os.path.exists(dest_html_path)):
            src_time = os.path.getmtime(src_html_path)
            dest_time = os.path.getmtime(dest_html_path)
            if(src_time < dest_time):
                print("No changes to webpage detected")
                return
            else:
                print("Modifying webpage.cpp")
        else:
            print("Making new webpage.cpp")

        # Open the source file
        src_html = open(src_html_path, "r")
        cpp_html = open(dest_html_path, "w")   
        
        cpp_html.write("#include \"webpage.h\"\n")
        cpp_html.write("#include <Arduino.h>\n")
        cpp_html.write("const char FULL_CONFIG_PAGE[] PROGMEM = R\"rawliteral(\n")

        # Write the source file to the destination file
        for line in src_html:
            cpp_html.write(line)
        
        cpp_html.write(")rawliteral\";\n")

        # Close the files
        src_html.close()
        cpp_html.close()
    else:
        print("Source webpage not found")


auto_create_webpagecpp()