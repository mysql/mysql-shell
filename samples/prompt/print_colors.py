
for j in range(32):
    for i in range(8):
        print "\033[48;5;%im"%(i+j*8),str(i+j*8).center(4)+"\033[0m",
        print "\033[38;5;%im"%(i+j*8),str(i+j*8).center(4)+"\033[0m",
    print

