
dtnfile = open("dtn.txt","r")
seqfile = open("seq.csv","w")
bndlfile = open("bndl.csv","w")

seqfile.write("seqno,timeGenerated,timeReceived,sensorID,mobileID\n")
bndlfile.write("bndlseqno,timerecvd,delay,sensorID,mobileID\n")

for line in dtnfile:

	if line[0]=="x":
		line=line.replace("10.0.0.","")
		# print str(int(line[-2:])-1)
		line=line[:len(line)-2] +str(int(line[-2:])-1)
		seqfile.write(line[1:]+"\n")
	elif line[0] =="y":
		# print line
		line=line.replace("10.0.0.","")
		# print "YES",line[-2:]
		line=line[:len(line)-2] +str(int(line[-2:])-1)
		bndlfile.write(line[1:]+"\n")
seqfile.close()
bndlfile.close()
dtnfile.close()

seqfile = open("seq.csv","r")
bndlfile = open("bndl.csv","r")
seq=[]
bndl=[]

for line in seqfile:
	if line != "seqno,timeGenerated,timeReceived,sensorID,mobileID\n":
		line=line.strip("\n").split(",")
		line=[float(x) for x in line]
		seq.append(line)

for line in bndlfile:
	if line != "bndlseqno,timerecvd,delay,sensorID,mobileID\n":
		line=line.strip("\n").split(",")
		line=[float(x) for x in line]
		if line[3]==1:
			line[0]=line[0]-1000
		if line[3]==3:
			line[0]=line[0]-3000
		if line[3]==4:
			line[0]=line[0]-4000
		bndl.append(line)

seqfile.close()
bndlfile.close()


# graphcoor = open("graphcoor.txt","w")
plots = open("plots.txt","w")

legend="LEGEND"

# Data Sequence Number x Delay
out10=""
out13=""
out14=""

for coor in seq:
	if coor[4]==0:
		out10=out10 + "(" + str(coor[0]) + "," + str(coor[2]-coor[1]) + ")"
	if coor[4]==3:
		out13=out13 + "(" + str(coor[0]) + "," + str(coor[2]-coor[1]) + ")"
	if coor[4]==4:
		out14=out14 + "(" + str(coor[0]) + "," + str(coor[2]-coor[1]) + ")"

# Bundle Sequence Number x Delay
out20=""
out23=""
out24=""

for coor in bndl:
	if coor[4]==0:
		out20=out20 + "(" + str(coor[0]) + "," + str(coor[2]) + ")"
	if coor[4]==3:
		out23=out23 + "(" + str(coor[0]) + "," + str(coor[2]) + ")"
	if coor[4]==4:
		out24=out24 + "(" + str(coor[0]) + "," + str(coor[2]) + ")"

# Time Received at Base x Data Sequence Number
out30=""
out33=""
out34=""

for coor in seq:
	if coor[4]==0:
		out30=out30 + "(" + str(coor[2]) + "," + str(coor[0]) + ")"
	if coor[4]==3:
		out33=out33 + "(" + str(coor[2]) + "," + str(coor[0]) + ")"
	if coor[4]==4:
		out34=out34 + "(" + str(coor[2]) + "," + str(coor[0]) + ")"

# Time Received at Base x Bundle Sequence Number
out40=""
out43=""
out44=""

for coor in bndl:
	if coor[4]==0:
		out40=out40 + "(" + str(coor[1]) + "," + str(coor[0]) + ")"
	if coor[4]==3:
		out43=out43 + "(" + str(coor[1]) + "," + str(coor[0]) + ")"
	if coor[4]==4:
		out44=out44 + "(" + str(coor[1]) + "," + str(coor[0]) + ")"

plots.write("\documentclass{article}\n\usepackage[paperwidth=8.5in,paperheight=2.5in,left=0.4in, right=0.4in,top=0in,bottom=0in]{geometry}\n\\thispagestyle{empty}\n\usepackage[utf8]{inputenc}\n\usepackage{textcomp}\n\usepackage{subcaption}\n\usepackage{pgfplots}\n\pgfplotsset{compat=1.15}\n\n\definecolor{L4}{RGB}{189,189,189}\n\definecolor{L3}{RGB}{150,150,150}\n\definecolor{L2}{RGB}{99,99,99}\n\definecolor{L1}{RGB}{0,0,0}\n\n\\begin{document}\n\n\n\\begin{figure}\n\\begin{center}\n%\caption{A figure with two subfigures}\n\\ref{legendName}\\\n\end{center}\n\\begin{subfigure}{0.25\linewidth}\n\\begin{tikzpicture}[xscale=0.56, yscale=0.56]\n\\begin{axis}[\n    title={Data Sequence Number x Delay},\n    xlabel={Data Sequence Number},\n    ylabel={Delay},\n    mark=*,\n    legend to name=legendName,\n]\n\legend{" + legend + "}\n")
plots.write("\\addplot [L1, only marks, mark=o]\ncoordinates{\n" + out10 + "};\n")
plots.write("\\addplot [L2, only marks, mark=square]\ncoordinates{\n" + out13 + "};\n")
plots.write("\\addplot [L3, only marks, mark=diamond]\ncoordinates{\n" + out14 + "};\n")

plots.write("\end{axis}\n\end{tikzpicture}\n\end{subfigure}%\n\hfill%\n\\begin{subfigure}{0.25\linewidth}\n\\begin{tikzpicture}[xscale=0.56, yscale=0.56]\n\\begin{axis}[\n    title={Bundle Sequence Number x Delay},\n    xlabel={Bundle Sequence Number},\n    ylabel={Delay},\n    mark=*,\n]\n")
plots.write("\\addplot [L1, only marks, mark=o]\ncoordinates{\n" + out20 + "};\n")
plots.write("\\addplot [L2, only marks, mark=square]\ncoordinates{\n" + out23 + "};\n")
plots.write("\\addplot [L3, only marks, mark=diamond]\ncoordinates{\n" + out24 + "};\n")

plots.write("\end{axis}\n\end{tikzpicture}\n\end{subfigure}%\n\hfill%\n\\begin{subfigure}{0.25\linewidth}\n\\begin{tikzpicture}[xscale=0.56, yscale=0.56]\n\\begin{axis}[\n    title={Time Received at Base x Data Sequence Number},\n    xlabel={Time Received at Base},\n    ylabel={Data Sequence Number},\n    mark=*,\n]\n")
plots.write("\\addplot [L1, only marks, mark=o]\ncoordinates{\n" + out30 + "};\n")
plots.write("\\addplot [L2, only marks, mark=square]\ncoordinates{\n" + out33 + "};\n")
plots.write("\\addplot [L3, only marks, mark=diamond]\ncoordinates{\n" + out34 + "};\n")

plots.write("\end{axis}\n\end{tikzpicture}\n\end{subfigure}%\n\hfill%\n\\begin{subfigure}{0.25\linewidth}\n\\begin{tikzpicture}[xscale=0.56, yscale=0.56]\n\\begin{axis}[\n    title={Time Received at Base x Bundle Sequence Number},\n    xlabel={Time Received at Base},\n    ylabel={Bundle Sequence Number},\n    mark=*,\n]\n")
plots.write("\\addplot [L1, only marks, mark=o]\ncoordinates{\n" + out40 + "};\n")
plots.write("\\addplot [L2, only marks, mark=square]\ncoordinates{\n" + out43 + "};\n")
plots.write("\\addplot [L3, only marks, mark=diamond]\ncoordinates{\n" + out44 + "};\n")

plots.write("\end{axis}\n\end{tikzpicture}\n\end{subfigure}\n\end{figure}\n\n\n\end{document}\n")
plots.close()