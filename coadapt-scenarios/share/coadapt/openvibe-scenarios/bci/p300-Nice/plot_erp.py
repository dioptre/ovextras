import numpy
import matplotlib.figure as fig
import matplotlib.pyplot as pyplot

class MyOVBox(OVBox):
	def __init__(self):
		OVBox.__init__(self)
		self.signalHeader = None

	def initialize(self):
		self.canDraw = False
		self.stimCode = OpenViBE_stimulation[self.setting['stimulus']]
		self.plotFileName = self.setting['file_name']
		#pyplot.ion() #nasty trick, if not in interactive mode python blocks when drawing (although it shouldn't)

	def uninitialize(self):
		#pyplot.close() #close the figure
		pass
	
	def process(self):
		for inputIndex in range( len(self.input) ):
			for chunkIndex in range( len(self.input[inputIndex]) ):
				
				if(type(self.input[inputIndex][chunkIndex]) == OVStimulationHeader):
					self.input[inputIndex].pop();

				elif(type(self.input[inputIndex][chunkIndex]) == OVStimulationSet):
					inputStimulus = self.input[inputIndex].pop();
					if (len(inputStimulus)>0):
						if(inputStimulus[0].identifier==self.stimCode):
							self.canDraw = True;

				elif(type(self.input[inputIndex][chunkIndex]) == OVStimulationEnd):
					self.input[inputIndex].pop();

				elif(type(self.input[inputIndex][chunkIndex]) == OVStreamedMatrixHeader):
					self.signalHeader = self.input[inputIndex].pop()
					self.data = numpy.zeros((2,self.signalHeader.dimensionSizes[1]))
				
				elif(type(self.input[inputIndex][chunkIndex]) == OVStreamedMatrixBuffer):
					chunk = self.input[inputIndex].pop()
					self.data[inputIndex,:] = numpy.array(chunk).reshape(tuple(self.signalHeader.dimensionSizes))

				elif(type(self.input[inputIndex][chunkIndex]) == OVStreamedMatrixEnd):	 			
					self.input[inputIndex].pop()

		if (self.canDraw):
			self.canDraw = False
			x = numpy.arange(0,self.signalHeader.dimensionSizes[1],1)
			pyplot.cla()
			pyplot.plot(x, self.data.transpose().tolist(), linewidth=1.0) #update line plots
			pyplot.legend( ('Target ERP', 'Non-target ERP') )
			pyplot.savefig(self.plotFileName)

box = MyOVBox()
