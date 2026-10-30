from ispa_deps import dump_inc

if __name__ == "__main__":
    # Reading the json file as a string
    with open("ispaext_deps.json", "r", encoding="utf-8") as f:
        json_str = f.read()

    # Pass string to dump_inc and write output to .inc
    inc_content = dump_inc(json_str)

    with open("ispaext_deps.inc", "w", encoding="utf-8") as f:
        f.write(inc_content)