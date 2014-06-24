#ifndef __IEvidenceAccumulator_H__
#define __IEvidenceAccumulator_H__
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <map>
#include <queue>
#include <vector>

#include <cstring>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cmath>

#include "../ovexP300SharedMemoryReader.h"
#include "../properties/ovexP300StimulatorPropertyReader.h"
#include "../sequence/ovexP300SequenceGenerator.h"


namespace OpenViBEApplications
{		

		/**
		  * The EvidenceAccumulator class takes evidence from OpenVibe (via shared memory) and determines which symbol is selected.
		 *	Each time data is received from the openvibe scenario, the probabilities are updated
		 *
		 */
		class ExternalP300IEvidenceAccumulator
		{
			public:
				
				/**
				 * @param propertyObject the object containing the properties for the EvidenceAccumulator such as flash duration, interflash duration, intertrial...
				 * @param l_pSequenceGenerator the sequence generator that defines which letters are flashed at one single point in time (does that for the whole trial)
				 */
				ExternalP300IEvidenceAccumulator(P300StimulatorPropertyReader* propertyObject, P300SequenceGenerator* l_pSequenceGenerator):m_opropertyObject(propertyObject),m_pSequenceGenerator(l_pSequenceGenerator),m_bIsReadyToPredict(false)
				{
					m_pAccumulatedEvidence = new OpenViBE::CMatrix();
					m_pAccumulatedEvidence->setDimensionCount(1);
					m_pAccumulatedEvidence->setDimensionSize(0, l_pSequenceGenerator->getNumberOfSymbols());
					m_bStopCondition = propertyObject->getStopCondition();
					earlyStoppingEnabled = propertyObject->getEarlyStopping();
					maxRepetition = propertyObject->getNumberOfRepetitions();
					m_oSharedMemoryReader = new ExternalP300SharedMemoryReader();
					OpenViBE::CString l_sSharedMemoryName = propertyObject->getSharedMemoryName();
					m_oSharedMemoryReader->openSharedMemory(l_sSharedMemoryName);
					m_ui64Prediction=0;

				}
				
				//ExternalP300IEvidenceAccumulator(){}

				virtual ~ExternalP300IEvidenceAccumulator()
				{
					m_opropertyObject=NULL;
					m_pSequenceGenerator=NULL;
					m_oSharedMemoryReader->closeSharedMemory();
				}

				
				/**
				 *
				 */ 
				virtual void generateNewSequence()=0;

				//a prediction equal to 0 is used to indicate an error
				virtual OpenViBE::uint64 getPrediction()
				{
					if(m_bIsReadyToPredict)
					{
						OpenViBE::float32 max;
						findMaximum(m_pAccumulatedEvidence->getBuffer(), &m_ui64Prediction, &max);
					}
					//if we are not ready, the prediction will be 0 and ignored
					return m_ui64Prediction;
				}

				//reset all accumulated evidence, used when a new trial start
				virtual void flushEvidence()
				{
					std::cout <<"flush\n";
					m_ui64Prediction=0;
					m_ui32CurrentFlashIndex=0;
					m_bIsReadyToPredict=false;
					//clear buffer
					OpenViBE::float64* buffer = m_pAccumulatedEvidence->getBuffer();
					for(unsigned int i=0; i<m_pAccumulatedEvidence->getBufferElementCount(); i++)
					{
						buffer[i]=0;
					}

				}

				//function called at each loop of the stimulator
				virtual void update()
				{
					//std::cout << "Evidence Acc update " << std::endl;
					//get the evidence from shared memory
					OpenViBE::IMatrix* currentEvidence = m_oSharedMemoryReader->readNextSymbolProbabilities();
					
					if(currentEvidence!=NULL)
					{
						//the matrix should only contain a single value, the proba of the gorup containing a p300
						OpenViBE::float64* proba = currentEvidence->getBuffer();
						//match to the current flash group
						std::cout << "Evidence Acc update with proba " << proba[0] << std::endl;
						OpenViBE::IMatrix* evidenceToAdd = matchProbaLetters(proba[0]);
						accumulate(evidenceToAdd);
						if(earlyStoppingEnabled)//get the token wich says if early stopping is enabled or not
						{
							m_bIsReadyToPredict = stopEarly();
						}
						//there is NumberOfGroup flashes by repetition so if we have more than NumberOfGroup*maxRepetition flashes, we force a predicition
						if(m_ui32CurrentFlashIndex*m_pSequenceGenerator->getNumberOfGroups()>=maxRepetition)
						{
							m_bIsReadyToPredict=true;
						}
						m_oSharedMemoryReader->clearSymbolProbabilities();//clear to avoid reading it again
					}
				}
				
				/**
				 * @return return vector of zeros and ones defining which letters will be flashed next
				 */
				virtual std::vector<OpenViBE::uint32>* getNextFlashGroup()=0;
				
				/**
				 * @return The shared memory reader that is created during construction of the EvidenceAccumulator
				 */
				virtual ExternalP300SharedMemoryReader* getSharedMemoryReader() { return m_oSharedMemoryReader; }

		protected:

				virtual void accumulate(OpenViBE::IMatrix* mEvidenceToAdd)=0;

				//find maximum
				void findMaximum(OpenViBE::float64* vector, OpenViBE::uint32* l_ui32MaximumIndex, OpenViBE::float32* l_f32Maximum)
				{
					*l_f32Maximum = std::numeric_limits<int>::min();
					*l_ui32MaximumIndex = 0;
					for (unsigned int j=0; j<m_pAccumulatedEvidence->getBufferElementCount(); j++)
						if (*(vector+j)>(*l_f32Maximum))
						{
							*l_f32Maximum = *(vector+j);
							*l_ui32MaximumIndex = j;
						}
					//if an index of 0 is a mistake, we must start to count at 1
					(*l_ui32MaximumIndex)++;

					//return  l_ui32MaximumIndex;
				}

				//update the proba of the letters based on the current flash
				OpenViBE::IMatrix* matchProbaLetters(OpenViBE::float64 proba)
				{
					//get letter in the current flash
					std::vector<OpenViBE::uint32>* currentFlashGroup = this->getNextFlashGroup();
					m_ui32CurrentFlashIndex++;
					std::cout << " getting group  " << m_ui32CurrentFlashIndex << std::endl;

					OpenViBE::CMatrix* fproba = new OpenViBE::CMatrix();
					fproba->setDimensionCount(2);
					fproba->setDimensionSize(0,1);
					fproba->setDimensionSize(1,m_pSequenceGenerator->getNumberOfSymbols());
					OpenViBE::float64* buffer = fproba->getBuffer();

					//check
					if(!(fproba->getBufferElementCount()==currentFlashGroup->size()))
						return NULL;

					for(unsigned int i=0; i<currentFlashGroup->size(); i++)
					{
						(*(buffer+i))=proba*currentFlashGroup->at(i);
					}

					return fproba;
				}

				//check against early stopping criteria
				OpenViBE::boolean stopEarly()
				{
					bool rValue = false;
					OpenViBE::uint32 argmax;
					OpenViBE::float32 max;
					unsigned int j=0;
					OpenViBE::float64* buffer = m_pAccumulatedEvidence->getBuffer();
					findMaximum(buffer, &argmax, &max);


					bool m_bEarlyStoppingConditionMet = true;
					while((m_bEarlyStoppingConditionMet))
					{
						if((buffer[j]>max-m_bStopCondition)&&(j!=argmax))
							m_bEarlyStoppingConditionMet=false;
						j++;
					}
					rValue = m_bEarlyStoppingConditionMet;
					if(rValue)
					{
						m_ui64Prediction = argmax;
					}
					return rValue;
				}

				ExternalP300SharedMemoryReader* m_oSharedMemoryReader;
				OpenViBE::IMatrix* m_pAccumulatedEvidence;
				OpenViBE::uint32 m_ui64Prediction;
				OpenViBE::uint32 m_ui32CurrentFlashIndex;
				P300StimulatorPropertyReader* m_opropertyObject;
				P300SequenceGenerator* m_pSequenceGenerator;
				bool m_bIsReadyToPredict;
				bool earlyStoppingEnabled;
				OpenViBE::uint32 maxRepetition;//number of repetition before we force the accumulator to make a prediction
				OpenViBE::float64 m_bStopCondition;
				
		};

};
#endif
