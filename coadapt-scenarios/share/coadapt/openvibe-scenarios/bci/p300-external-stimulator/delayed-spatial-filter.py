import numpy

class MyOVBox(OVBox):
    # the constructor creates the box and initializes object variables
	def __init__(self):
		OVBox.__init__(self)
		self.filters = None
		self.matrix_header = None
		self.ReadyToProcess = False
		self.chunkBuffer = []

	# the initialize method reads settings and outputs the first header
	def initialize(self):
		# all settings
		print 'initialize'
		
	def process(self):

		# process spatial filter input
		for chunk_index in range( len(self.input[1]) ):
			
			if(type(self.input[1][chunk_index]) == OVStreamedMatrixHeader):
				self.matrix_header = self.input[1].pop()
				print "Delayed spatial filter: Matrix header received"
			
			elif(type(self.input[1][chunk_index]) == OVStreamedMatrixBuffer):
				chunk = self.input[1].pop()
				print "received spatial filters, dimensionSizes " + str(tuple(self.matrix_header.dimensionSizes))
				self.filters = numpy.matrix(chunk).reshape(tuple(self.matrix_header.dimensionSizes))
				
				outputHeader = OVSignalHeader(
				self.signal_header.startTime,
				self.signal_header.endTime,
				[self.matrix_header.dimensionSizes[0], self.signal_header.dimensionSizes[1]],
				self.matrix_header.dimensionSizes[0]*['Filter']+self.signal_header.dimensionSizes[1]*[''],
				self.signal_header.samplingRate)
				self.output[0].append(outputHeader)
				
				self.ReadyToProcess = True			
			
			elif(type(self.input[1][chunk_index]) == OVStreamedMatrixEnd):
				self.input[1].pop()
											
			# process first signal input
		for chunk_index in range( len(self.input[0]) ):

			if(type(self.input[0][chunk_index]) == OVSignalHeader):
				self.signal_header = self.input[0].pop()
									
			elif(type(self.input[0][chunk_index]) == OVSignalBuffer):
				chunk = self.input[0].pop()
				self.chunkBuffer.append(chunk)
						
			elif(type(self.input[0][chunk_index]) == OVSignalEnd):
				self.input[0].pop()
		
		if(self.ReadyToProcess):			
			self.ReadyToProcess = False	
			print "Delayed filter will process " + str(len(self.chunkBuffer)) + " chunks"
			for i in range( len(self.chunkBuffer) ):
				
				chunk = self.chunkBuffer.pop(0)
				X = numpy.matrix(chunk).reshape(tuple(self.signal_header.dimensionSizes))
				X = (self.filters)*X
				#print "Filter signal shape " + str(X.shape)
				chunk = OVSignalBuffer(chunk.startTime, chunk.endTime, X.flatten().tolist()[0])
				self.output[0].append(chunk)			

if __name__ == '__main__':
	box = MyOVBox()
