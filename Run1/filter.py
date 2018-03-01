

dtnfile = open("dtn.txt","r")
seqfile = open("seq.csv","w")
bndlfile = open("bndl.csv","w")

seqfile.write("seqno,timeGenerated,timeReceived\n")
bndlfile.write("bndlseqno,timerecvd,delay\n")

for line in dtnfile:
	if line[0]=="x":
		seqfile.write(line[1:])
	elif line[0] =="y":
		bndlfile.write(line[1:])
seqfile.close()
bndlfile.close()
dtnfile.close()