/*
	KalmanFilter.hpp

	Wednesday, May 30, 2007

	Copyright(c) 2007, Smarter Than You Software.  All rights reserved.
*/

#ifndef __KalmanFilter__hpp__
#define __KalmanFilter__hpp__

#include "linalg.h"
using namespace linalg::aliases;

namespace Soma
{
	namespace Math
	{
		class KalmanFilter
		{
			public:
				KalmanFilter();

				float getCurrentStateBestEstimate( const int stateIndex );// Returns the current stateEst of stateIndex
				void setCurrentStateBestEstimate( const int stateIndex, const float value );

				float getCurrentStateUncertainty( const int stateIndex );// Returns the current uncertainty of stateIndex

				void updateTime( const float deltaTime, const float measuredAccel );
				void updateMeasurement( const float* measurement );

				const float getTimePropUncertainty( const int index );
				const float getMeasurementPropUncertainty( const int index );

		private:
// Hoepfully you can tell the difference between vectors and matrices and that all are made of floats.
// The few pointers are so that entire matrices are not copied druing updates then refilled.
				float3* pX;//Points to Current stateVector
				float3* pXprev;//Points to Previous stateVector

				float3 Xa;//stateVector
				float3 Xb;//stateVector

				float3x3* pP;// Points to Current Covariance matrix
				float3x3* pPprev;// Points to Previous Covariance matrix
				float3x3 Pa;// Covariance matrix
				float3x3 Pb;// Covariance matrix

				float3x3 F;
				float3x3 Phi;// Depends on time
				float3x3 PhiTranspose;// Depends on time
				float3x2 G;// noise time to state space matrix
				float2x3 GTranspose;
				float2 timePropNoiseVector;

				float1 Z;//measurementVector;

				float1x3 H;
				float3x1 HTranspose;
				float1 measurmentNoiseVector;

				float3 K;// Gain

				void timeUpdateSetPrevious();
				void measurementUpdateSetPrevious();
		};
	}
}



#endif // #ifndef __Soma__Particles__KalmanFilter__h__
