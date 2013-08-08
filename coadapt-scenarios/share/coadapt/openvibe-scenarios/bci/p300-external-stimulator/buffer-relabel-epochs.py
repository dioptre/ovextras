# we use numpy to compute the mean of an array of values
import numpy
from collections import deque

# let's define a new box class that inherits from OVBox
class MyOVBox(OVBox):
	def __init__(self):
		OVBox.__init__(self)
# we add a new member to save the signal header information we will receive
		#self.signalHeader = None
		self.signalBuffer = deque([])
		self.flashedSymbolsHeader = None
		self.flashedSymbols = deque([])
		self.Label = -1
	
	def initialize(self):
		 self.Trigger = self.setting['Trigger']

# The process method will be called by openvibe on every clock tick
	def process(self):
		for chunkIndex in range( len(self.input[0]) ):
			if(type(self.input[0][chunkIndex])==OVStimulationHeader):
				stimSet = self.input[0].pop()
			elif(type(self.input[0][chunkIndex])==OVStimulationSet):
				stimSet = self.input[0].pop()
				for  stimIndex in range(len(stimSet)):
					if stimSet[stimIndex].identifier==int(self.Trigger):
						for  stimIndex in range(len(stimSet)):
							if stimSet[stimIndex].identifier!=int(self.Trigger):
								self.Label = stimSet[stimIndex].identifier
				if self.Label != -1:
					print "Label received, flushing target and non-target epochs"
					while self.signalBuffer:
						flashGroup = self.flashedSymbols.popleft()
						pyFlashGroup = numpy.array(flashGroup).reshape(tuple(self.flashedSymbolsHeader.dimensionSizes))
						if pyFlashGroup[self.Label-1,0]==1 :
							chunk = self.signalBuffer.popleft()
							#print "Target epoch flushed, length " + str(len(chunk))
							self.output[0].append(chunk)
						else :
							self.output[1].append(self.signalBuffer.popleft())
					self.flashedSymbols.clear()
					self.signalBuffer.clear()
					self.Label = -1	
					
			elif(type(self.input[0][chunkIndex])==OVStimulationEnd):
				stimSet = self.input[0].pop()

		for chunkIndex in range( len(self.input[1]) ):
			if(type(self.input[1][chunkIndex]) == OVStreamedMatrixHeader):
				self.flashedSymbolsHeader = self.input[1].pop()
			elif(type(self.input[1][chunkIndex]) == OVStreamedMatrixBuffer):
				chunk = self.input[1].pop()
				self.flashedSymbols.append(chunk)
			elif(type(self.input[1][chunkIndex]) == OVSreamedMatrixEnd):
				chunk = self.input[1].pop()

		for chunkIndex in range( len(self.input[2]) ):
			if(type(self.input[2][chunkIndex]) == OVSignalHeader):
				chunk = self.input[2].pop()
				self.output[0].append(chunk)
				self.output[1].append(chunk)
			elif(type(self.input[2][chunkIndex]) == OVSignalBuffer):
				chunk = self.input[2].pop()
				self.signalBuffer.append(chunk)
			elif(type(self.input[2][chunkIndex]) == OVSignalEnd):
				stimSet = self.input[2].pop()
										
# Finally, we notify openvibe that the box instance 'box' is now an instance of MyOVBox.
# Don't forget that step !!
box = MyOVBox()