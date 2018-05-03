
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
		bndl.append(line)

seqfile.close()
bndlfile.close()


# graphcoor = open("graphcoor.txt","w")
plots = open("plots.txt","w")

legend="Test 0: Main Test"
# legend="Test II: Mobile Speed Test"

# Data Sequence Number x Delay
out1=""
xmax1=0
ymax1=0

for coor in seq:
	out1=out1 + "(" + str(coor[0]) + "," + str(coor[2]-coor[1]) + ")"

# Bundle Sequence Number x Delay
out2=""
xmax2=0
ymax2=0
for coor in bndl:
	out2=out2 + "(" + str(coor[0]) + "," + str(coor[2]) + ")"

# Time Received at Base x Data Sequence Number
out3=""
xmax3=0
ymax3=0
for coor in seq:
	out3=out3 + "(" + str(coor[2]) + "," + str(coor[0]) + ")"

# Time Received at Base x Bundle Sequence Number
out4=""
xmax4=0
ymax4=0
for coor in bndl:
	out4=out4 + "(" + str(coor[1]) + "," + str(coor[0]) + ")"



plots.write("\documentclass{article}\n\usepackage[paperwidth=8.5in,paperheight=9in,left=0.4in, right=0.4in,top=0in,bottom=0in]{geometry}\n\\thispagestyle{empty}\n\usepackage[utf8]{inputenc}\n\usepackage{textcomp}\n\usepackage{subcaption}\n\usepackage{pgfplots}\n\pgfplotsset{compat=1.15}\n\n\definecolor{L4}{RGB}{189,189,189}\n\definecolor{L3}{RGB}{150,150,150}\n\definecolor{L2}{RGB}{99,99,99}\n\definecolor{L1}{RGB}{0,0,0}\n\n\n\\begin{document}\n\n\\begin{figure}\n\\begin{center}\n\\ref{legendName}\\\n\end{center}\n\\begin{subfigure}{1\\textwidth}\n\\begin{tikzpicture}\n\\begin{axis}[\n    scale only axis,\n    height=1.45in,\n    width=7in,\n    % title={Data Sequence Number x Delay},\n    xlabel={Data Sequence Number},\n    ylabel={Delay},\n    mark=*,\n    xmin=0,\n    ymin=0,\n    legend to name=legendName,\n    legend style={legend columns=-1},\n]\n")
plots.write("\legend{" + legend + "}\n")
plots.write("\\addplot [L1, only marks, mark=o]\ncoordinates{\n" + out1 + "};\n")

plots.write("\end{axis}\n\end{tikzpicture}\n\caption{Data Sequence Number x Delay}\n\end{subfigure}\\\n\n\\begin{subfigure}{1\\textwidth}\n\\begin{tikzpicture}\n\\begin{axis}[\n    scale only axis,\n    height=1.45in,\n    width=7in,\n    % title={Bundle Sequence Number x Delay},\n    xlabel={Bundle Sequence Number},\n    ylabel={Delay},\n    mark=*,\n    xmin=0,\n    ymin=0,\n]\n")
plots.write("\\addplot [L1, only marks, mark=o]\ncoordinates{\n" + out2 + "};\n")

plots.write("\end{axis}\n\end{tikzpicture}\n\caption{Bundle Sequence Number x Delay}\n\end{subfigure}\\\n\n\\begin{subfigure}{1\\textwidth}\n\\begin{tikzpicture}\n\\begin{axis}[\n    scale only axis,\n    height=1.45in,\n    width=7in,\n    % title={Time Received at Base x Data Sequence Number},\n    xlabel={Time Received at Base},\n    ylabel={Data Sequence Number},\n    mark=*,\n    xmin=0,\n    ymin=0,\n]\n")
plots.write("\\addplot [L1, only marks, mark=o]\ncoordinates{\n" + out3 + "};\n")

plots.write("\end{axis}\n\end{tikzpicture}\n\caption{Time Received at Base x Data Sequence Number}\n\end{subfigure}\\\n\n\\begin{subfigure}{1\\textwidth}\n\\begin{tikzpicture}\n\\begin{axis}[\n    scale only axis,\n    height=1.45in,\n    width=7in,\n    % title={Time Received at Base x Bundle Sequence Number},\n    xlabel={Time Received at Base},\n    ylabel={Bundle Sequence Number},\n    mark=*,\n    xmin=0,\n    ymin=0,\n]\n")
plots.write("\\addplot [L1, only marks, mark=o]\ncoordinates{\n" + out4 + "};\n")

plots.write("\end{axis}\n\end{tikzpicture}\n\caption{Time Received at Base x Bundle Sequence Number}\n\end{subfigure}\n\n% \\begin{center}\n% \caption{FIGURE CAPTION}\n% \end{center}\n\end{figure}\n\n\n\end{document}\n")
plots.close()
