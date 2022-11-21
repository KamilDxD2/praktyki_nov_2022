with open('additional.txt', 'r') as e:
	contents = e.read().split("\n")

allFonts = []
thisChar = []
for line in contents:
	line = line.replace(" ", "0")
	while len(line) < 8:
		line += "0"
	c = 0
	if line.startswith("-"):
		if len(thisChar) != 8:
			print("Unexpected additional data")
			print(len(thisChar))
		allFonts += thisChar
		thisChar = []
		continue
	for chr in line.strip():
		c <<= 1
		c |= int(chr)
	thisChar += [ c ]
    
print(', '.join(f"0x{'0' if x < 16 else ''}{hex(x)[2:]}" for x in allFonts))
