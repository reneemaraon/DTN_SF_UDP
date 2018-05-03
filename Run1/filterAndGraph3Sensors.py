
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

roundNo=0
for line in seqfile:
	if line != "seqno,timeGenerated,timeReceived,sensorID,mobileID\n":
		line=line.strip("\n").split(",")
		line=[float(x) for x in line]
		line[0]=line[0]+(1000*roundNo)
		if line[0]==999:
			roundNo=roundNo+1
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

legend="Test I: Sensor Number Test:Sensor 1,Test I: Sensor Number Test:Sensor 2,Test I: Sensor Number Test:Sensor 3"

# Data Sequence Number x Delay
out11=""
out13=""
out14=""

for coor in seq:
	if coor[3]==1:
		out11=out11 + "(" + str(coor[0]) + "," + str(coor[2]-coor[1]) + ")"
	if coor[3]==3:
		out13=out13 + "(" + str(coor[0]) + "," + str(coor[2]-coor[1]) + ")"
	if coor[3]==4:
		out14=out14 + "(" + str(coor[0]) + "," + str(coor[2]-coor[1]) + ")"

# Bundle Sequence Number x Delay
out21=""
out23=""
out24=""

for coor in bndl:
	if coor[3]==1:
		out21=out21 + "(" + str(coor[0]) + "," + str(coor[2]) + ")"
	if coor[3]==3:
		out23=out23 + "(" + str(coor[0]) + "," + str(coor[2]) + ")"
	if coor[3]==4:
		out24=out24 + "(" + str(coor[0]) + "," + str(coor[2]) + ")"

# Time Received at Base x Data Sequence Number
out31=""
out33=""
out34=""

for coor in seq:
	if coor[3]==1:
		out31=out31 + "(" + str(coor[2]) + "," + str(coor[0]) + ")"
	if coor[3]==3:
		out33=out33 + "(" + str(coor[2]) + "," + str(coor[0]) + ")"
	if coor[3]==4:
		out34=out34 + "(" + str(coor[2]) + "," + str(coor[0]) + ")"

# Time Received at Base x Bundle Sequence Number
out41=""
out43=""
out44=""

for coor in bndl:
	if coor[3]==1:
		out41=out41 + "(" + str(coor[1]) + "," + str(coor[0]) + ")"
	if coor[3]==3:
		out43=out43 + "(" + str(coor[1]) + "," + str(coor[0]) + ")"
	if coor[3]==4:
		out44=out44 + "(" + str(coor[1]) + "," + str(coor[0]) + ")"

plots.write("\documentclass{article}\n\usepackage[paperwidth=8.5in,paperheight=9in,left=0.4in, right=0.4in,top=0in,bottom=0in]{geometry}\n\\thispagestyle{empty}\n\usepackage[utf8]{inputenc}\n\usepackage{textcomp}\n\usepackage{subcaption}\n\usepackage{pgfplots}\n\pgfplotsset{compat=1.15}\n\n\definecolor{L4}{RGB}{189,189,189}\n\definecolor{L3}{RGB}{150,150,150}\n\definecolor{L2}{RGB}{99,99,99}\n\definecolor{L1}{RGB}{0,0,0}\n\n\n\\begin{document}\n\n\\begin{figure}\n\\begin{center}\n\\ref{legendName}\\\n\end{center}\n\\begin{subfigure}{1\\textwidth}\n\\begin{tikzpicture}\n\\begin{axis}[\n    scale only axis,\n    height=1.45in,\n    width=7in,\n    % title={Data Sequence Number x Delay},\n    xlabel={Data Sequence Number},\n    ylabel={Delay},\n    mark=*,\n    xmin=0,\n    ymin=0,\n    legend to name=legendName,\n    legend style={legend columns=-1},\n]\n")
plots.write("\legend{" + legend + "}\n")
plots.write("\\addplot [L1, only marks, mark=o]\ncoordinates{\n" + out11 + "};\n")
plots.write("\\addplot [L2, only marks, mark=square]\ncoordinates{\n" + out13 + "};\n")
plots.write("\\addplot [L3, only marks, mark=diamond]\ncoordinates{\n" + out14 + "};\n")

plots.write("\end{axis}\n\end{tikzpicture}\n\caption{Data Sequence Number x Delay}\n\end{subfigure}\\\n\n\\begin{subfigure}{1\\textwidth}\n\\begin{tikzpicture}\n\\begin{axis}[\n    scale only axis,\n    height=1.45in,\n    width=7in,\n    % title={Bundle Sequence Number x Delay},\n    xlabel={Bundle Sequence Number},\n    ylabel={Delay},\n    mark=*,\n    xmin=0,\n    ymin=0,\n]\n")
plots.write("\\addplot [L1, only marks, mark=o]\ncoordinates{\n" + out21 + "};\n")
plots.write("\\addplot [L2, only marks, mark=square]\ncoordinates{\n" + out23 + "};\n")
plots.write("\\addplot [L3, only marks, mark=diamond]\ncoordinates{\n" + out24 + "};\n")

plots.write("\end{axis}\n\end{tikzpicture}\n\caption{Bundle Sequence Number x Delay}\n\end{subfigure}\\\n\n\\begin{subfigure}{1\\textwidth}\n\\begin{tikzpicture}\n\\begin{axis}[\n    scale only axis,\n    height=1.45in,\n    width=7in,\n    % title={Time Received at Base x Data Sequence Number},\n    xlabel={Time Received at Base},\n    ylabel={Data Sequence Number},\n    mark=*,\n    xmin=0,\n    ymin=0,\n]\n")
plots.write("\\addplot [L1, only marks, mark=o]\ncoordinates{\n" + out31 + "};\n")
plots.write("\\addplot [L2, only marks, mark=square]\ncoordinates{\n" + out33 + "};\n")
plots.write("\\addplot [L3, only marks, mark=diamond]\ncoordinates{\n" + out34 + "};\n")

plots.write("\end{axis}\n\end{tikzpicture}\n\caption{Time Received at Base x Data Sequence Number}\n\end{subfigure}\\\n\n\\begin{subfigure}{1\\textwidth}\n\\begin{tikzpicture}\n\\begin{axis}[\n    scale only axis,\n    height=1.45in,\n    width=7in,\n    % title={Time Received at Base x Bundle Sequence Number},\n    xlabel={Time Received at Base},\n    ylabel={Bundle Sequence Number},\n    mark=*,\n    xmin=0,\n    ymin=0,\n]\n")
plots.write("\\addplot [L1, only marks, mark=o]\ncoordinates{\n" + out41 + "};\n")
plots.write("\\addplot [L2, only marks, mark=square]\ncoordinates{\n" + out43 + "};\n")
plots.write("\\addplot [L3, only marks, mark=diamond]\ncoordinates{\n" + out44 + "};\n")

plots.write("\end{axis}\n\end{tikzpicture}\n\caption{Time Received at Base x Bundle Sequence Number}\n\end{subfigure}\n\n% \\begin{center}\n% \caption{FIGURE CAPTION}\n% \end{center}\n\end{figure}\n\n\n\end{document}\n")
plots.close()