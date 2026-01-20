files = ["index.html", "htmx.js", "pico.css"]

c_src_template = """#ifndef EMBEDDED_FILES_H
#define EMBEDDED_FILES_H
%CONTENTS%
#endif
"""
text_embed_template = "static const char* EMBED_%NAME%_DATA = \"%CONTENTS%\";\n" #static const unsigned long EMBED_%NAME%_LEN = %LEN%;\n"

contents = ""

for file in files:
    with open(file) as f:
        data = f.read()
        escaped = data.replace("\\", "\\\\")
        escaped = escaped.replace("\"", "\\\"")
        escaped = escaped.replace("\n", "\\n\\\n")
        name = file.replace(".", "_").upper()
        part = text_embed_template.replace("%NAME%", name)
        #part = part.replace("%LEN%", str(len(data)))
        part = part.replace("%CONTENTS%", escaped)

        contents += part

src = c_src_template.replace("%CONTENTS%", contents)

print(src)
