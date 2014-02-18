/*
 *  Copyright 2007-2014 The OpenMx Project
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */


/***********************************************************
*
*  omxStateSpaceExpectation.c
*
*  Created: Michael D. Hunter 	Date: 2012-10-28 20:07:36
*
*  Contains code to calculate the objective function for a
*   state space model.  Currently, this is done with a 
*   Kalman filter in separate Predict and Update steps.
*   Later this could be done with one of several Kalman 
*   filter-smoothers (a forward-backward algorithm).
*
**********************************************************/

#include "omxExpectation.h"
#include "omxBLAS.h"
#include "omxFIMLFitFunction.h"
#include "omxStateSpaceExpectation.h"


void omxCallStateSpaceExpectation(omxExpectation* ox, const char *) {
    if(OMX_DEBUG) { mxLog("State Space Expectation Called."); }
	omxStateSpaceExpectation* ose = (omxStateSpaceExpectation*)(ox->argStruct);
	
	omxRecompute(ose->A);
	omxRecompute(ose->B);
	omxRecompute(ose->C);
	omxRecompute(ose->D);
	omxRecompute(ose->Q);
	omxRecompute(ose->R);
	
	// Probably should loop through all the data here!!!
	omxKalmanPredict(ose);
	omxKalmanUpdate(ose);
}



void omxDestroyStateSpaceExpectation(omxExpectation* ox) {
	
	if(OMX_DEBUG) { mxLog("Destroying State Space Expectation."); }
	
	omxStateSpaceExpectation* argStruct = (omxStateSpaceExpectation*)(ox->argStruct);
	
	/* We allocated 'em, so we destroy 'em. */
	omxFreeMatrixData(argStruct->r);
	omxFreeMatrixData(argStruct->s);
	omxFreeMatrixData(argStruct->z);
	//omxFreeMatrixData(argStruct->u); // This is data, destroy it?
	//omxFreeMatrixData(argStruct->x); // This is latent data, destroy it?
	omxFreeMatrixData(argStruct->y); // This is data, destroy it?
	omxFreeMatrixData(argStruct->K); // This is the Kalman gain, destroy it?
	//omxFreeMatrixData(argStruct->P); // This is latent cov, destroy it?
	omxFreeMatrixData(argStruct->S); // This is data error cov, destroy it?
	omxFreeMatrixData(argStruct->Y);
	omxFreeMatrixData(argStruct->Z);
}


void omxPopulateSSMAttributes(omxExpectation *ox, SEXP algebra) {
    if(OMX_DEBUG) { mxLog("Populating State Space Attributes.  Currently this does very little!"); }
	
}




void omxKalmanPredict(omxStateSpaceExpectation* ose) {
	if(OMX_DEBUG) { mxLog("Kalman Predict Called."); }
	/* Creat local copies of State Space Matrices */
	omxMatrix* A = ose->A;
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(A, "....State Space: A"); }
	omxMatrix* B = ose->B;
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(B, "....State Space: B"); }
	//omxMatrix* C = ose->C;
	//omxMatrix* D = ose->D;
	omxMatrix* Q = ose->Q;
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(Q, "....State Space: Q"); }
	//omxMatrix* R = ose->R;
	//omxMatrix* r = ose->r;
	//omxMatrix* s = ose->s;
	omxMatrix* u = ose->u;
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(u, "....State Space: u"); }
	omxMatrix* x = ose->x;
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(x, "....State Space: x"); }
	//omxMatrix* y = ose->y;
	omxMatrix* z = ose->z;
	//omxMatrix* K = ose->K;
	omxMatrix* P = ose->P;
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(P, "....State Space: P"); }
	//omxMatrix* S = ose->S;
	//omxMatrix* Y = ose->Y;
	omxMatrix* Z = ose->Z;

	/* x = A x + B u */
	omxDGEMV(FALSE, 1.0, A, x, 0.0, z); // x = A x
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(z, "....State Space: z = A x"); }
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(A, "....State Space: A"); }
	omxDGEMV(FALSE, 1.0, B, u, 1.0, z); // x = B u + x THAT IS x = A x + B u
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(z, "....State Space: z = A x + B u"); }
	omxCopyMatrix(x, z); // x = z THAT IS x = A x + B u
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(x, "....State Space: x = A x + B u"); }
	
	/* P = A P A^T + Q */
	omxDSYMM(FALSE, 1.0, P, A, 0.0, Z); // Z = A P
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(Z, "....State Space: Z = A P"); }
	omxCopyMatrix(P, Q); // P = Q
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(P, "....State Space: P = Q"); }
	omxDGEMM(FALSE, TRUE, 1.0, Z, A, 1.0, P); // P = Z A^T + P THAT IS P = A P A^T + Q
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(P, "....State Space: P = A P A^T + Q"); }
}


void omxKalmanUpdate(omxStateSpaceExpectation* ose) {
	//TODO: Clean up this hack of function.
	if(OMX_DEBUG) { mxLog("Kalman Update Called."); }
	/* Creat local copies of State Space Matrices */
	omxMatrix* C = ose->C;
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(C, "....State Space: C"); }
	omxMatrix* D = ose->D;
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(D, "....State Space: D"); }
	// omxMatrix* R = ose->R; //unused
	omxMatrix* r = ose->r;
	omxMatrix* s = ose->s;
	omxMatrix* u = ose->u;
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(u, "....State Space: u"); }
	omxMatrix* x = ose->x;
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(x, "....State Space: x"); }
	omxMatrix* y = ose->y;
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(y, "....State Space: y"); }
	// omxMatrix* K = ose->K; //unused
	omxMatrix* P = ose->P;
	omxMatrix* S = ose->S;
	// omxMatrix* Y = ose->Y; //unused
	// omxMatrix* Cov = ose->cov; //unused
	omxMatrix* Means = ose->means;
	omxMatrix* Det = ose->det;
	*Det->data = 0.0; // the value pointed to by Det->data is assigned to be zero
	omxMatrix* smallC = ose->smallC;
	omxMatrix* smallD = ose->smallD;
	omxMatrix* smallr = ose->smallr;
	omxMatrix* smallR = ose->smallR;
	omxMatrix* smallK = ose->smallK;
	omxMatrix* smallS = ose->smallS;
	omxMatrix* smallY = ose->smallY;
	int ny = y->rows;
	int nx = x->rows;
	int toRemoveSS[ny];
	int numRemovesSS = 0;
	int toRemoveNoneLat[nx];
	int toRemoveNoneOne[1];
	memset(toRemoveNoneLat, 0, sizeof(int) * nx);
	memset(toRemoveNoneOne, 0, sizeof(int) * 1);
	
	int info = 0; // Used for computing inverse for Kalman gain
	
	omxAliasMatrix(smallS, S); //re-alias every time expectation is called, at end delete the alias.
	
	/* Reset/Resample aliased matrices */
	omxResetAliasedMatrix(smallC);
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(smallC, "....State Space: C (Reset)"); }
	omxResetAliasedMatrix(smallD);
	omxResetAliasedMatrix(smallR);
	omxResetAliasedMatrix(smallr);
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(smallr, "....State Space: r (Reset)"); }
	omxResetAliasedMatrix(smallK);
	omxResetAliasedMatrix(smallS);
	omxResetAliasedMatrix(smallY);
	
	/* r = r - C x - D u */
	/* Alternatively, create just the expected value for the data row, x. */
	omxDGEMV(FALSE, 1.0, smallC, x, 0.0, s); // s = C x
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(s, "....State Space: s = C x"); }
	omxDGEMV(FALSE, 1.0, D, u, 1.0, s); // s = D u + s THAT IS s = C x + D u
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(s, "....State Space: s = C x + D u"); }
	omxCopyMatrix(Means, s); // Means = s THAT IS Means = C x + D u
	//if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(Means, "....State Space: Means"); }
	omxTransposeMatrix(Means);
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(Means, "....State Space: Means"); }
	
	//If entire data vector, y, is missing, then set residual, r, to zero.
	//otherwise, compute residual.
	memset(toRemoveSS, 0, sizeof(int) * ny);
	for(int j = 0; j < y->rows; j++) {
		double dataValue = omxMatrixElement(y, j, 0);
		int dataValuefpclass = fpclassify(dataValue);
		if(dataValuefpclass == FP_NAN || dataValuefpclass == FP_INFINITE) {
			numRemovesSS++;
			toRemoveSS[j] = 1;
			omxSetMatrixElement(r, j, 0, 0.0);
		} else {
			omxSetMatrixElement(r, j, 0, (dataValue -  omxMatrixElement(s, j, 0)));
		}
	}
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(r, "....State Space: Residual (Loop)"); }
	/* Now compute the residual */
	//omxCopyMatrix(r, y); // r = y
	//omxDAXPY(-1.0, s, r); // r = r - s THAT IS r = y - (C x + D u)
	omxResetAliasedMatrix(smallr);
	
	/* Filter S Here */
	// N.B. if y is completely missing or completely present, leave S alone.
	// Otherwise, filter S.
	if(numRemovesSS < ny && numRemovesSS > 0) {
		//if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(smallS, "....State Space: S"); }
		if(OMX_DEBUG) { mxLog("Filtering S, R, C, r, K, and Y."); }
		omxRemoveRowsAndColumns(smallS, numRemovesSS, numRemovesSS, toRemoveSS, toRemoveSS);
		//if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(smallS, "....State Space: S (Filtered)"); }
		
		omxRemoveRowsAndColumns(smallR, numRemovesSS, numRemovesSS, toRemoveSS, toRemoveSS);
		
		if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(smallC, "....State Space: C"); }
		omxRemoveRowsAndColumns(smallC, numRemovesSS, 0, toRemoveSS, toRemoveNoneLat);
		if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(smallC, "....State Space: C (Filtered)"); }
		
		if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(r, "....State Space: r"); }
		omxRemoveRowsAndColumns(smallr, numRemovesSS, 0, toRemoveSS, toRemoveNoneOne);
		if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(smallr, "....State Space: r (Filtered)"); }
		
		//if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(smallK, "....State Space: K"); }
		omxRemoveRowsAndColumns(smallK, numRemovesSS, 0, toRemoveSS, toRemoveNoneLat);
		//if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(smallK, "....State Space: K (Filtered)"); }
		
		omxRemoveRowsAndColumns(smallY, numRemovesSS, 0, toRemoveSS, toRemoveNoneLat);
	}
	if(numRemovesSS == ny) {
		if(OMX_DEBUG_ALGEBRA) { mxLog("Completely missing row of data found."); }
		if(OMX_DEBUG_ALGEBRA) { mxLog("Skipping much of Kalman Update."); }
		smallS->aliasedPtr = NULL;//delete alias from smallS
		return ;
	}
	
	
	/* S = C P C^T + R */
	omxDSYMM(FALSE, 1.0, P, smallC, 0.0, smallY); // Y = C P
	//omxCopyMatrix(S, smallR); // S = R
	memcpy(smallS->data, smallR->data, smallR->rows * smallR->cols * sizeof(double)); // Less safe omxCopyMatrix that keeps smallS aliased to S.
	omxDGEMM(FALSE, TRUE, 1.0, smallY, smallC, 1.0, smallS); // S = Y C^T + S THAT IS C P C^T + R
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(smallS, "....State Space: S = C P C^T + R"); }
	
	
	/* Now compute the Kalman Gain and update the error covariance matrix */
	/* S = S^-1 */
	omxDPOTRF(smallS, &info); // S replaced by the lower triangular matrix of the Cholesky factorization
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(smallS, "....State Space: Cholesky of S"); }
	if(info > 0) {
		omxRaiseErrorf(smallS->currentState, "Expected covariance matrix is non-positive-definite (info %d)", info);
		return;  // Leave output untouched
	}
	for(int i = 0; i < smallS->cols; i++) {
		*Det->data += log(fabs(omxMatrixElement(smallS, i, i)));
	}
	//det *= 2.0; //sum( log( abs( diag( chol(S) ) ) ) )*2
	omxDPOTRI(smallS, &info); // S = S^-1 via Cholesky factorization
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(smallS, "....State Space: Inverse of S"); }
	
	
	/* K = P C^T S^-1 */
	/* Computed as K^T = S^-1 C P */
	omxDSYMM(TRUE, 1.0, smallS, smallY, 0.0, smallK); // K = Y^T S THAT IS K = P C^T S^-1
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(smallK, "....State Space: K^T = S^-1 C P"); }
	
	/* x = x + K r */
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(smallr, "....State Space Check Residual: r"); }
	omxDGEMV(TRUE, 1.0, smallK, smallr, 1.0, x); // x = K r + x
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(x, "....State Space: x = K r + x"); }
	
	/* P = (I - K C) P */
	/* P = P - K C P */
	omxDGEMM(TRUE, FALSE, -1.0, smallK, smallY, 1.0, P); // P = -K Y + P THAT IS P = P - K C P
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(P, "....State Space: P = P - K C P"); }
	
	smallS->aliasedPtr = NULL;//delete alias from smallS
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(smallS, "....State Space: Inverse of S"); }
	
	
	/*m2ll = y^T S y */ // n.b. y originally is the data row but becomes the data residual!
	//omxDSYMV(1.0, S, y, 0.0, s); // s = S y
	//m2ll = omxDDOT(y, s); // m2ll = y s THAT IS y^T S y
	//m2ll += det; // m2ll = m2ll + det THAT IS m2ll = log(det(S)) + y^T S y
	// Note: this leaves off the S->cols * log(2*pi) THAT IS k*log(2*pi)
}



void omxInitStateSpaceExpectation(omxExpectation* ox) {
	
	SEXP rObj = ox->rObj;
	if(OMX_DEBUG) { mxLog("Initializing State Space Expectation."); }
		
	int nx, ny, nu;
	
	//SEXP slotValue;   //Used by PPML
	
	/* Create and fill expectation */
	omxStateSpaceExpectation *SSMexp = (omxStateSpaceExpectation*) R_alloc(1, sizeof(omxStateSpaceExpectation));
	omxState* currentState = ox->currentState;
	
	/* Set Expectation Calls and Structures */
	ox->computeFun = omxCallStateSpaceExpectation;
	ox->destructFun = omxDestroyStateSpaceExpectation;
	ox->componentFun = omxGetStateSpaceExpectationComponent;
	ox->mutateFun = omxSetStateSpaceExpectationComponent;
	ox->populateAttrFun = omxPopulateSSMAttributes;
	ox->argStruct = (void*) SSMexp;
	
	/* Set up expectation structures */
	if(OMX_DEBUG) { mxLog("Initializing State Space Meta Data for expectation."); }
	
	if(OMX_DEBUG) { mxLog("Processing A."); }
	SSMexp->A = omxNewMatrixFromSlot(rObj, currentState, "A");
	
	if(OMX_DEBUG) { mxLog("Processing B."); }
	SSMexp->B = omxNewMatrixFromSlot(rObj, currentState, "B");
	
	if(OMX_DEBUG) { mxLog("Processing C."); }
	SSMexp->C = omxNewMatrixFromSlot(rObj, currentState, "C");
	
	if(OMX_DEBUG) { mxLog("Processing D."); }
	SSMexp->D = omxNewMatrixFromSlot(rObj, currentState, "D");
	
	if(OMX_DEBUG) { mxLog("Processing Q."); }
	SSMexp->Q = omxNewMatrixFromSlot(rObj, currentState, "Q");
	
	if(OMX_DEBUG) { mxLog("Processing R."); }
	SSMexp->R = omxNewMatrixFromSlot(rObj, currentState, "R");
	
	if(OMX_DEBUG) { mxLog("Processing initial x."); }
	SSMexp->x0 = omxNewMatrixFromSlot(rObj, currentState, "x0");
	
	if(OMX_DEBUG) { mxLog("Processing initial P."); }
	SSMexp->P0 = omxNewMatrixFromSlot(rObj, currentState, "P0");
	
	if(OMX_DEBUG) { mxLog("Processing u."); }
	SSMexp->u = omxNewMatrixFromSlot(rObj, currentState, "u");

	
	
	/* Initialize the place holder matrices used in calculations */
	nx = SSMexp->C->cols;
	ny = SSMexp->C->rows;
	nu = SSMexp->D->cols;
	
	if(OMX_DEBUG) { mxLog("Processing first data row for y."); }
	SSMexp->y = omxInitMatrix(NULL, ny, 1, TRUE, currentState);
	for(int i = 0; i < ny; i++) {
		omxSetMatrixElement(SSMexp->y, i, 0, omxDoubleDataElement(ox->data, 0, i));
	}
	if(OMX_DEBUG_ALGEBRA) {omxPrintMatrix(SSMexp->y, "....State Space: y"); }
	
	// TODO Make x0 and P0 static (if possible) to save memory
	// TODO Look into omxMatrix.c/h for a possible new matrix from omxMatrix function
	if(OMX_DEBUG) { mxLog("Generating static internals for resetting initial values."); }
	SSMexp->x = 	omxInitMatrix(NULL, nx, 1, TRUE, currentState);
	SSMexp->P = 	omxInitMatrix(NULL, nx, nx, TRUE, currentState);
	omxCopyMatrix(SSMexp->x, SSMexp->x0);
	omxCopyMatrix(SSMexp->P, SSMexp->P0);
	
	if(OMX_DEBUG) { mxLog("Generating internals for computation."); }
	
	SSMexp->det = 	omxInitMatrix(NULL, 1, 1, TRUE, currentState);
	SSMexp->r = 	omxInitMatrix(NULL, ny, 1, TRUE, currentState);
	SSMexp->s = 	omxInitMatrix(NULL, ny, 1, TRUE, currentState);
	SSMexp->z = 	omxInitMatrix(NULL, nx, 1, TRUE, currentState);
	SSMexp->K = 	omxInitMatrix(NULL, ny, nx, TRUE, currentState); // Actually the tranpose of the Kalman gain
	SSMexp->S = 	omxInitMatrix(NULL, ny, ny, TRUE, currentState);
	SSMexp->Y = 	omxInitMatrix(NULL, ny, nx, TRUE, currentState);
	SSMexp->Z = 	omxInitMatrix(NULL, nx, nx, TRUE, currentState);
	
	SSMexp->cov = 		omxInitMatrix(NULL, ny, ny, TRUE, currentState);
	SSMexp->means = 	omxInitMatrix(NULL, 1, ny, TRUE, currentState);
	
	/* Create alias matrices for missing data filtering */
	SSMexp->smallC = 	omxInitMatrix(NULL, ny, nx, TRUE, currentState);
	SSMexp->smallD = 	omxInitMatrix(NULL, ny, nu, TRUE, currentState);
	SSMexp->smallR = 	omxInitMatrix(NULL, ny, ny, TRUE, currentState);
	SSMexp->smallr = 	omxInitMatrix(NULL, ny, 1, TRUE, currentState);
	SSMexp->smallK = 	omxInitMatrix(NULL, ny, nx, TRUE, currentState);
	SSMexp->smallS = 	omxInitMatrix(NULL, ny, ny, TRUE, currentState);
	SSMexp->smallY = 	omxInitMatrix(NULL, ny, nx, TRUE, currentState);
	
	/* Alias the small matrices so their alias pointers refer to the memory location of their respective matrices */
	omxAliasMatrix(SSMexp->smallC, SSMexp->C);
	omxAliasMatrix(SSMexp->smallD, SSMexp->D);
	omxAliasMatrix(SSMexp->smallR, SSMexp->R);
	omxAliasMatrix(SSMexp->smallr, SSMexp->r);
	omxAliasMatrix(SSMexp->smallK, SSMexp->K);
	omxAliasMatrix(SSMexp->smallS, SSMexp->S);
	omxAliasMatrix(SSMexp->smallY, SSMexp->Y);

}


omxMatrix* omxGetStateSpaceExpectationComponent(omxExpectation* ox, omxFitFunction* off, const char* component) {
	omxStateSpaceExpectation* ose = (omxStateSpaceExpectation*)(ox->argStruct);
	omxMatrix* retval = NULL;

	if(!strncmp("cov", component, 3)) {
		retval = ose->cov;
	} else if(!strncmp("mean", component, 4)) {
		retval = ose->means;
	} else if(!strncmp("pvec", component, 4)) {
		// Once implemented, change compute function and return pvec
	} else if(!strncmp("inverse", component, 7)) {
		retval = ose->smallS;
	} else if(!strncmp("determinant", component, 11)) {
		retval = ose->det;
	} else if(!strncmp("r", component, 1)) {
		retval = ose->r;
	}
	
	if(OMX_DEBUG) { mxLog("Returning %p.", retval); }

	return retval;
}

void omxSetStateSpaceExpectationComponent(omxExpectation* ox, omxFitFunction* off, const char* component, omxMatrix* om) {
	omxStateSpaceExpectation* ose = (omxStateSpaceExpectation*)(ox->argStruct);
	
	if(!strcmp("y", component)) {
		for(int i = 0; i < ose->y->rows; i++) {
			omxSetMatrixElement(ose->y, i, 0, omxVectorElement(om, i));
		}
		//ose->y = om;
	}
	if(!strcmp("Reset", component)) {
		omxCopyMatrix(ose->x, ose->x0);
		omxCopyMatrix(ose->P, ose->P0);
	}
}



