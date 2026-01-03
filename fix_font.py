import re

file_path = "chumini/IngameCanvas.cpp"

with open(file_path, "rb") as f:
    content = f.read()

# Pattern: L"851..."
# We search for L"851 and then non-quote characters until "
# L is 0x4C, " is 0x22, 8 is 0x38, 5 is 0x35, 1 is 0x31
# b'L"851[^"]*"'

pattern = re.compile(b'L"851[^"]*"')
replacement = b'L"851\\x30B4\\x30C1\\x30AB\\x30AF\\x30C3\\x30C8"'

new_content, count = pattern.subn(replacement, content)

print(f"Replaced {count} occurrences.")

with open(file_path, "wb") as f:
    f.write(new_content)
