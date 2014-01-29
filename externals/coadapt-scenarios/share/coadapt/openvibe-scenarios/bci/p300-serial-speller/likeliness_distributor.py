import numpy as numpy

class MyOVBox(OVBox):
	def __init__(self):
		OVBox.__init__(self)
		self.signalHeader = None

	def initialize(self):
		#print 'Initializer called in python box'
		self.n = int(self.setting['number'])
		self.index = int(self.setting['index'])
		self.scale = float(self.setting['scale'])
		#print 'Initializer called in python box, number {}, index {}'.format(self.n,self.index)

	def uninitialize(self):
		print 'Uninitializer called in python box'
	
	def process(self):
		chunkIndex = 0
		#print 'Signal fdsfsadf sdfdsafsaaf f asdfsda fsda {},{}'.format(chunkIndex,len(self.input[0]))
		for chunkIndex in range( len(self.input[0]) ):
			if(type(self.input[0][chunkIndex]) == OVStreamedMatrixHeader):
				#print 'Signal header received in python box'
				self.signalHeader = self.input[0].pop()
				outHeader = OVStreamedMatrixHeader(self.signalHeader.startTime, self.signalHeader.endTime, [1, self.n], ['pvalue-of-target']+self.n*[''])
				self.output[0].append(outHeader)
				#print 'Size of matrix {},{}'.format(self.signalHeader.dimensionSizes[0],self.signalHeader.dimensionSizes[1])
			
			elif(type(self.input[0][chunkIndex]) == OVStreamedMatrixBuffer):
				chunk = self.input[0].pop()
				inputBuffer = numpy.array(chunk).reshape(tuple(self.signalHeader.dimensionSizes))
				#print 'Number of elements in matrix {}, element value {}'.format(self.signalHeader.getBufferElementCount(),inputBuffer[0])
				outputBuffer = numpy.zeros(self.n)
				outputBuffer[self.index] = max(0,1-self.scale*inputBuffer[0])
				chunk = OVStreamedMatrixBuffer(chunk.startTime, chunk.endTime, outputBuffer.tolist())
				#print 'elements list {}'.format(outputBuffer.tolist())
				self.output[0].append(chunk)
				
			elif(type(self.input[0][chunkIndex]) == OVStreamedMatrixEnd):	 			
				self.output[0].append(self.input[0].pop())

			#print 'Signal fdsfsadf sdaf {},{}'.format(chunkIndex,len(self.input[0]))

box = MyOVBox()
