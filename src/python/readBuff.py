#circular buffer class
class RingBuffer: 
	def __init__(self, size):
        	self.data = [None for i in xrange(size)]

	def append(self, x):
        	self.data.pop(0)
        	self.data.append(x)

	def get(self):
        	return self.data


#define circular buffer size and begin reading
buffSize = 22000
buf = RingBuffer(buffSize)
i = 0
while 1:
	li = input()
 	if not li: 
        	break
    	else:
        	buf.append(li)
    	#print buf.get()
	if(i==buffSize):
		print  float(sum(buf.get())) / len(buf.get())
		i=0
	i+=1
        

