f = open("typescript" , "rb");
win = 0;
draw = 0
loss = 0
match = 0
lossframes = []
wins = []
drawframes = []
toframes = []
for line in f:
	words = line.split();
	if len(words) == 2 and words[0] == "match":
		match = int(words[1])
	elif len(words)>1 and words[0] == "Match=":
		for j in range(1,len(words)):
			print words[j],
		print
		print
	elif len(words)>0 and words[0] == "Draw!":
		draw = draw +1;
		drawframes.append(match)
	elif len(words) >2 and words[0] == "Player" and words[2] == "Wins!":
		if words[1] == "1":
			win = win+1
			wins.append(match)
		else:
			loss= loss+1
			lossframes.append(match)
	elif len(words)>0 and words[0] == "WARNING:":
		# problem = problem +1;
		toframes.append(match);

print "Total Matches:", match-1, "Wins:", win, "Losses:",loss, "Lost games are:"
for i in range(0,len(lossframes)):
	print lossframes[i],
print 
print "Draws:", draw, "Timed out:", len(toframes), "Problematic frames:"
for i in range(0,len(toframes)):
	print toframes[i],
