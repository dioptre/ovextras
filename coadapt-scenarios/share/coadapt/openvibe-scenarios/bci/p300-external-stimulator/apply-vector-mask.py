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
	def process(self):
		for chunkIndex in range( len(self.input[2]) ):
			if(type(self.input[2][chunkIndex])==OVStimulationHeader):
				self.input[2].pop()
			elif(type(self.input[2][chunkIndex])==OVStimulationSet):
				stimSet = self.input[2].pop()
				for stimIndex in range( len(stimSet) ):
					#print "Stim " + str(stimSet[stimIndex].identifier) + " received "
					if (stimSet[stimIndex].identifier==int(self.resetTrigger)):
						#print "Reset trigger " + str(self.resetTrigger) + " received "
						for bufferIndex in range( len(self.input2Buffer) ):
							mask = self.input2Buffer.popleft()
							#print "deleting input2buffer "
							#print mask
			elif(type(self.input[2][chunkIndex])==OVStimulationEnd):
				self.input[2].pop()
				
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
					#print numpyBuffer
					chunk = OVStreamedMatrixBuffer(chunk.startTime, chunk.endTime, numpyBuffer.tolist())
					self.output[0].append(chunk)
				#else:
				#	print "No masks in the queue anymore";
				
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