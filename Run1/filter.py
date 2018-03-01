

dtnfile = open("dtn.txt","r")
outfile = open("out.csv","w")
for line in dtnfile:
	if line[0]=="p":
		outfile.write(line[1:])

outfile.close()
dtnfile.close()