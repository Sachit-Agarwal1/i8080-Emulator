bin = b'\x26\xAA\x2E\x00\x36\xF0\xB6\x30\x76'


with open("program1", "wb") as f:
	f.write(bytes(bin))