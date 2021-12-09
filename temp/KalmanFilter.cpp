/*
	KalmanFilter.cpp

	Wednesday, May 30, 2007

	From: http://www.gamasutra.com/features/20070620/KalmanFilter.cpp
*/

#include "KalmanFilter.h"


namespace Soma
{
	namespace Math
	{

		KalmanFilter::KalmanFilter()
		{
			pX = &Xa;
			pXprev = &Xb;
			pP = &Pa;
			pPprev = &Pb;

			Xa = {0, 0, 0};
			Xb = {0, 0, 0};

			Z = 0;

			F[0][1] = 1.0f;
			F[1][2] = -1.0f;

			Phi = linalg::identity;// Depends on time
			PhiTranspose = linalg::identity;// Depends on time

			G[1][0] = 1.0f;
			G[2][1] = 1.0f;
			GTranspose[0][1] = 1.0f;
			GTranspose[1][2] = 1.0f;

			H[0][0] = 1.0f;
			HTranspose[0][0] = 1.0f;

// The above will not change for our filter. For a general filter they will.

// These are the intial covariance estimates, feel free to muck with them.
			Pa[0][0] =  0.05f * 0.05f;
			Pa[1][1] =  0.5f * 0.5f;
			Pa[2][2] =  10.0f * 10.0f;
			Pb = Pa;

// The following will change to appropriate values for noise.
			timePropNoiseVector[0] = 0.01f ;// m / s^2
			timePropNoiseVector[1] = 0.10f;// m / s^2 / s
			measurmentNoiseVector[0] = 0.005f;// =1/2 cm;
		}

		float KalmanFilter::getCurrentStateBestEstimate( const int stateIndex )
		{
			if( stateIndex < 0 || stateIndex > pX->numElements )
			{
				return pX->getElement(0);
			}

			return pX->getElement(stateIndex);
		}

		void KalmanFilter::setCurrentStateBestEstimate( const int stateIndex, const float value )
		{
			pX->setElement(stateIndex,value);
		}

		float KalmanFilter::getCurrentStateUncertainty( const int stateIndex )
		{
			if( stateIndex < 0 || stateIndex * stateIndex >= pP->getNumElements() )
			{
				return pP->getElement(0,0);
			}

			return sqrtf( pP->getElement(stateIndex,stateIndex) );
		}


		const float KalmanFilter::getTimePropUncertainty( const int index )
		{
			if( index > timePropNoiseVector.numDimensions )
			{
				return 0.0f;
			}

			return timePropNoiseVector[index];
		}

		const float KalmanFilter::getMeasurementPropUncertainty( const int index )
		{
			if( index > measurmentNoiseVector.numDimensions )
			{
				return 0.0f;
			}

			return measurmentNoiseVector[index];
		}

		void KalmanFilter::updateTime( const float t, const float measuredAccel )// t is the time to propagate
		{// Variable time propigation
	// Basic equations to simulate
	// xnew = Phi * xold
	// Pnew = Phi * Pold * PhiT + Int [Phi*G*Q*GT*PhiT] * dt
			this->timeUpdateSetPrevious();
// Setup the Phi and PhiTranspose
//			Phi.identity();// Depends on time, overkill for this example. Only need to rewrite the same elements
			Phi[0][1] = t;
			Phi[0][2] = -t * t * 0.5f;
			Phi[1][2] = -t;

//			PhiTranspose.identity();// Depends on time, overkill for this example. Only need to rewrite the same elements
			PhiTranspose[1][0] = t;
			PhiTranspose[2][0] = -t * t * 0.5f;
			PhiTranspose[2][1] = -t;

			TVectorN<3,float>& X = *pX;
			TVectorN<3,float>& Xprev = *pXprev;

	// Update the state vector.
			Phi.transform(Xprev,X);// xnew = Phi * xold

			X[0] += measuredAccel * t * t * 0.5f;// These will not be here for an eerror state Kalman Filter
			X[1] += measuredAccel * t;// These will not be here for an eerror state Kalman Filter

			TMatrixMxN<3,3,float>& P = *pP;// Points to Current Covariance matrix
			TMatrixMxN<3,3,float>& Pprev = *pPprev;// Points to Previous Covariance matrix

	// Now update teh covariance matrix. Yay lots of temps.
			TMatrixMxN<3,3,float> temp33;
			temp33.mul(Phi,Pprev);
			P.mul(temp33,PhiTranspose);

// Since Q and G are independent of time can make into one easy vector the is integrated by hand
			const float Q11 = timePropNoiseVector[0] * timePropNoiseVector[0];
			const float Q22 = timePropNoiseVector[1] * timePropNoiseVector[1];
//			const float Q12 = 0.0f * timePropNoiseVector[0] * timePropNoiseVector[1]; 0.0f means tehy are uncorrelated
			temp33[0][0] = ( Q11 / 3.0f + Q22 * t * t / 20.0f )* t * t * t;
			temp33[0][1] = ( Q11 * 0.5f + Q22 * t * t / 8.0f )* t * t;
			temp33[0][2] = -Q22* t * t * t / 6.0f;
			temp33[1][0] = temp33[0][1];
			temp33[1][1] = ( Q11 + Q22 * t * t  / 3.0f )* t;
			temp33[1][2] = -Q22 * t * t * 0.5f;
			temp33[2][0] = temp33[0][2];
			temp33[2][1] = temp33[1][2];
			temp33[2][2] = Q22 * t;

			P += temp33;
		}

		void KalmanFilter::updateMeasurement( const float* measurement )
		{
			this->measurementUpdateSetPrevious();
		// K = P * HT * [H * P * HT + noiseMeasurement]^-1 (Involves matrix inverse in general, for us not)
		// x+ = x + K * [ measurement - H * x ]
		// P+ = ( 1 - K * H ) * P = P - K*H*P
			TVectorN<3,float>& X = *pX;
			TVectorN<3,float>& Xprev = *pXprev;

			TMatrixMxN<3,3,float>& P = *pP;// Points to Current Covariance matrix
			TMatrixMxN<3,3,float>& Pprev = *pPprev;// Points to Previous Covariance matrix

			TMatrixMxN<3,3,float> temp33;
			TMatrixMxN<3,1,float> temp31;
			TMatrixMxN<1,3,float> temp13;
			TMatrixMxN<1,1,float> temp11;

			temp13.mul(H,Pprev);
			temp11.mul(temp13,HTranspose);

			const float temp = temp11[0][0] + measurmentNoiseVector[0];

			temp31.mul(Pprev,HTranspose);
			temp31 /= temp;// This is K.
			temp31.getColumn(0,K);

			temp13.mul(H,Pprev);
//			temp31.setColumn(0,K);
			temp33.mul(temp31,temp13);
			P = Pprev - temp33;

			temp31.setColumn(0,Xprev);
			temp11.mul(H,temp31);
			X = K * ( *measurement - temp11[0][0] );
			X += Xprev;
		}

		void KalmanFilter::timeUpdateSetPrevious()
		{
			if( pX == &Xa )
			{
				pX = &Xb;
				pXprev = &Xa;
			}
			else
			{
				pX = &Xa;
				pXprev = &Xb;
			}

			if( pP == &Pa )
			{
				pP = &Pb;
				pPprev = &Pa;
			}
			else
			{
				pP = &Pa;
				pPprev = &Pb;
			}
		}

		void KalmanFilter::measurementUpdateSetPrevious()
		{
			if( pX == &Xa )
			{
				pX = &Xb;
				pXprev = &Xa;
			}
			else
			{
				pX = &Xa;
				pXprev = &Xb;
			}

			if( pP == &Pa )
			{
				pP = &Pb;
				pPprev = &Pa;
			}
			else
			{
				pP = &Pa;
				pPprev = &Pb;
			}
		}

	}
}
