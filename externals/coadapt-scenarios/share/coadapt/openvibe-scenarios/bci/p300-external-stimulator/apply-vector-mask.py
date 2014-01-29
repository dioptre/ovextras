# we use numpy to compute the mean of an array of values
import numpy
from collections import deque

# let's define a new box class that inherits from OVBox
class MyOVBox(OVBox):
	def __init__(self):
		OVBox.__init__(self)
# we add a new member to save the signal header information we will receive
		self.matrixHeader = None
		self.input2Buffer = deque([])
	
	def initialize(self):
		 self.resetTrigger = self.setting['ResetTrigger'];

# The process method will be called by openvibe on every clock tick
# The first input of this box receives is the probability whether it is a target flash or not.
# The second input is a matrix with lines containing zeros and ones. The ones indicating
# the symbols that have been flashed. Each line represents a flash and flashes have been
# generated for the whole trial (6 groups and 20 repetitions means 120 flashes)
# The third input is the feedback cue and removes the rest of the flashes that have not
# been used from the queue.
# The script multiplies the probability of each flash with the next line of the second input
# matrix. This is send to the output
	def process(self):
		#if the feedback cue arrives it will flush all the remaiming lines in the
		#input2Buffer matrix
		for chunkIndex in range( len(self.input[2]) ):
			if(type(self.input[2][chunkIndex])==OVStimulationHeader):
				self.input[2].pop()
			elif(type(self.input[2][chunkIndex])==OVStimulationSet):
				stimSet = self.input[2].pop()
				for stimIndex in range( len(stimSet) ):
					if (stimSet[stimIndex].identifier==int(self.resetTrigger)):
						for bufferIndex in range( len(self.input2Buffer) ):
							mask = self.input2Buffer.popleft()
			elif(type(self.input[2][chunkIndex])==OVStimulationEnd):
				self.input[2].pop()
		
		#The first input of this box receives is the probability whether it is a target flash or not and multiplies
		#it with the next line in the matrix input2Buffer of the second input
		for chunkIndex in range( len(self.input[0]) ):
			if(type(self.input[0][chunkIndex]) == OVStreamedMatrixHeader):
				chunk = self.input[0].pop()
			elif(type(self.input[0][chunkIndex]) == OVStreamedMatrixBuffer):
				chunk = self.input[0].pop()
				if (len(self.input2Buffer)>0):
					mask = self.input2Buffer.popleft()
					numpyBuffer = numpy.array(mask).reshape(tuple(self.matrixHeader.dimensionSizes))
					numpyBuffer = float(chunk[0])*numpyBuffer
					numpyBuffer = numpyBuffer.squeeze()
					chunk = OVStreamedMatrixBuffer(chunk.startTime, chunk.endTime, numpyBuffer.tolist())
					self.output[0].append(chunk)
		#The second input is a matrix with lines containing zeros and ones. The ones indicating
		#the symbols that have been flashed. Each line represents a flash and flashes have been
		#generated for the whole trial (6 groups and 20 repetitions means 120 flashes). This matrix
		#is received in the beginning of the trial and has to be buffered
		for chunkIndex in range( len(self.input[1]) ):
			if(type(self.input[1][chunkIndex]) == OVStreamedMatrixHeader):
				chunk = self.input[1].pop()
				self.matrixHeader = chunk
				outputHeader = OVStreamedMatrixHeader(
				self.matrixHeader.startTime,
				self.matrixHeader.endTime,
				[self.matrixHeader.dimensionSizes[0], 1],
				['']*self.matrixHeader.dimensionSizes[0]+[''])
				self.output[0].append(outputHeader)	
			elif(type(self.input[1][chunkIndex]) == OVStreamedMatrixBuffer):
				chunk = self.input[1].pop()
				self.input2Buffer.append(chunk)
			elif(type(self.input[1][chunkIndex]) == OVStreamedMatrixEnd):
				chunk = self.input[1].pop()
				self.output[0].append(chunk)
										
# Finally, we notify openvibe that the box instance 'box' is now an instance of MyOVBox.
# Don't forget that step !!
box = MyOVBox()