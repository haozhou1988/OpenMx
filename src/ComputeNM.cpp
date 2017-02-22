/*
 *  Copyright 2007-2017 The OpenMx Project
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
 */

#include "omxDefines.h"
#include "omxState.h"
#include "omxFitFunction.h"
#include "omxExportBackendState.h"
#include "nloptcpp.h"
#include "Compute.h"
#include "glue.h"
#include "ComputeGD.h"
#include "ComputeNM.h"
#include <Eigen/Core>
#include <Eigen/Cholesky>
#include <Eigen/Dense>

#include <Rmath.h>
#include <Rinternals.h>

#include "EnableWarnings.h"


class omxCompute *newComputeNelderMead()
{
	return new omxComputeNM();
}


omxComputeNM::omxComputeNM()
{

}


void omxComputeNM::initFromFrontend(omxState *globalState, SEXP rObj){
	super::initFromFrontend(globalState, rObj);
	
	SEXP slotValue;
	fitMatrix = omxNewMatrixFromSlot(rObj, globalState, "fitfunction");
	setFreeVarGroup(fitMatrix->fitFunction, varGroup);
	omxCompleteFitFunction(fitMatrix);
	
	ScopedProtect p1(slotValue, R_do_slot(rObj, Rf_install("verbose")));
	verbose = Rf_asInteger(slotValue);
	
	ScopedProtect p2(slotValue, R_do_slot(rObj, Rf_install("nudgeZeroStarts")));
	nudge = Rf_asLogical(slotValue);
	
	ScopedProtect p3(slotValue, R_do_slot(rObj, Rf_install("defaultMaxIter")));
	defaultMaxIter = Rf_asLogical(slotValue);
	
	ScopedProtect p4(slotValue, R_do_slot(rObj, Rf_install("maxIter")));
	if(Rf_isNull(slotValue)){maxIter = Global->majorIterations * 5;}
	else{maxIter = Rf_asInteger(slotValue);}
	
	ScopedProtect p5(slotValue, R_do_slot(rObj, Rf_install("alpha")));
	alpha = Rf_asReal(slotValue);
	
	ScopedProtect p6(slotValue, R_do_slot(rObj, Rf_install("betao")));
	betao = Rf_asReal(slotValue);
	
	ScopedProtect p7(slotValue, R_do_slot(rObj, Rf_install("betai")));
	betai = Rf_asReal(slotValue);
	
	ScopedProtect p8(slotValue, R_do_slot(rObj, Rf_install("gamma")));
	gamma = Rf_asReal(slotValue);
	
	ScopedProtect p9(slotValue, R_do_slot(rObj, Rf_install("sigma")));
	sigma = Rf_asReal(slotValue);
	
	ScopedProtect p10(slotValue, R_do_slot(rObj, Rf_install("bignum")));
	bignum = Rf_asReal(slotValue);
	
	ScopedProtect p11(slotValue, R_do_slot(rObj, Rf_install("iniSimplexType")));
	if(strEQ(CHAR(Rf_asChar(slotValue)),"regular")){iniSimplexType = 1;}
	else if(strEQ(CHAR(Rf_asChar(slotValue)),"right")){iniSimplexType = 2;}
	else if(strEQ(CHAR(Rf_asChar(slotValue)),"smartRight")){iniSimplexType = 3;}
	else if(strEQ(CHAR(Rf_asChar(slotValue)),"random")){iniSimplexType = 4;}
	else{Rf_error("unrecognized character string provided for Nelder-Mead 'iniSimplexType'");}
	
	ScopedProtect p12(slotValue, R_do_slot(rObj, Rf_install("iniSimplexEdge")));
	iniSimplexEdge = Rf_asReal(slotValue);
	
	ScopedProtect p13(slotValue, R_do_slot(rObj, Rf_install("iniSimplexMtx")));
	if (Rf_length(slotValue)) {
		SEXP matrixDims;
		ScopedProtect pipm(matrixDims, Rf_getAttrib(slotValue, R_DimSymbol));
		int *dimList = INTEGER(matrixDims);
		int rows = dimList[0];
		int cols = dimList[1];
		//iniSimplexMtx(REAL(slotValue), rows, cols);
		iniSimplexMtx = Eigen::Map< Eigen::MatrixXd >(REAL(slotValue), rows, cols);
	}
	
	ScopedProtect p14(slotValue, R_do_slot(rObj, Rf_install("greedyMinimize")));
	greedyMinimize = Rf_asLogical(slotValue);
	
	ScopedProtect p15(slotValue, R_do_slot(rObj, Rf_install("altContraction")));
	altContraction = Rf_asLogical(slotValue);
	
	ScopedProtect p16(slotValue, R_do_slot(rObj, Rf_install("degenLimit")));
	degenLimit = Rf_asLogical(slotValue);
	
	ScopedProtect p17(slotValue, R_do_slot(rObj, Rf_install("stagnationCtrl")));
	stagnationCtrl[0] = INTEGER(slotValue)[0];
	stagnationCtrl[1] = INTEGER(slotValue)[1];
	
	ScopedProtect p18(slotValue, R_do_slot(rObj, Rf_install("validationRestart")));
	validationRestart = Rf_asLogical(slotValue);
	
	ScopedProtect p19(slotValue, R_do_slot(rObj, Rf_install("xTolProx")));
	xTolProx = Rf_asReal(slotValue);
	
	ScopedProtect p20(slotValue, R_do_slot(rObj, Rf_install("fTolProx")));
	fTolProx = Rf_asReal(slotValue);
	
	ScopedProtect p21(slotValue, R_do_slot(rObj, Rf_install("xTolRelChange")));
	xTolRelChange = Rf_asReal(slotValue);
	
	ScopedProtect p22(slotValue, R_do_slot(rObj, Rf_install("fTolRelChange")));
	fTolRelChange = Rf_asReal(slotValue);
	
	ScopedProtect p23(slotValue, R_do_slot(rObj, Rf_install("pseudoHessian")));
	doPseudoHessian = Rf_asLogical(slotValue);
	
	ScopedProtect p24(slotValue, R_do_slot(rObj, Rf_install("ineqConstraintMthd")));
	if(strEQ(CHAR(Rf_asChar(slotValue)),"soft")){ineqConstraintMthd = 0;}
	else if(strEQ(CHAR(Rf_asChar(slotValue)),"eqMthd")){ineqConstraintMthd = 1;}
	else{Rf_error("unrecognized character string provided for Nelder-Mead 'ineqConstraintMthd'");}
	
	ScopedProtect p25(slotValue, R_do_slot(rObj, Rf_install("eqConstraintMthd")));
	if(strEQ(CHAR(Rf_asChar(slotValue)),"soft")){eqConstraintMthd = 1;}
	else if(strEQ(CHAR(Rf_asChar(slotValue)),"backtrack")){eqConstraintMthd = 2;}
	else if(strEQ(CHAR(Rf_asChar(slotValue)),"GDsearch")){eqConstraintMthd = 3;}
	else if(strEQ(CHAR(Rf_asChar(slotValue)),"augLag")){eqConstraintMthd = 4;}
	else{Rf_error("unrecognized character string provided for Nelder-Mead 'eqConstraintMthd'");}
	
	feasTol = Global->feasibilityTolerance;
	
	ProtectedSEXP Rexclude(R_do_slot(rObj, Rf_install(".excludeVars")));
	excludeVars.reserve(Rf_length(Rexclude));
	for (int ex=0; ex < Rf_length(Rexclude); ++ex) {
		int got = varGroup->lookupVar(CHAR(STRING_ELT(Rexclude, ex)));
		if (got < 0) continue;
		excludeVars.push_back(got);
	}
	
	//Rf_error("successful importation of Nelder-Mead compute object from frontend");
}


void omxComputeNM::computeImpl(FitContext *fc){
	
	omxAlgebraPreeval(fitMatrix, fc);
	if (isErrorRaised()) return;
	
	size_t numParam = fc->varGroup->vars.size();
	if (excludeVars.size()) {
		fc->profiledOut.assign(fc->numParam, false);
		for (auto vx : excludeVars) fc->profiledOut[vx] = true;
	}
	if (fc->profiledOut.size()) {
		if (fc->profiledOut.size() != fc->numParam) Rf_error("Fail");
		for (size_t vx=0; vx < fc->varGroup->vars.size(); ++vx) {
			if (fc->profiledOut[vx]) --numParam;
		}
	}
	
	if (numParam <= 0) {
		omxRaiseErrorf("%s: model has no free parameters", name);
		return;
	}
	
	fc->ensureParamWithinBox(nudge);
	fc->createChildren(fitMatrix);
	
	
	//int beforeEval = fc->getLocalComputeCount();
	
	if (verbose >= 1){
		//mxLog something here
	}

	//Rf_error("omxComputeNM::computeImpl() : so far, so good");
	
	NelderMeadOptimizerContext nmoc(fc, this);
	if(eqConstraintMthd==4){Rf_error("'augLag' Not Yet Implemented");}
	nmoc.countConstraintsAndSetupBounds();
	nmoc.invokeNelderMead();
}

//-------------------------------------------------------

NelderMeadOptimizerContext::NelderMeadOptimizerContext(FitContext* _fc, omxComputeNM* _nmo)
	: fc(_fc), NMobj(_nmo), numFree(countNumFree())
{
	est.resize(numFree);
	copyParamsFromFitContext(est.data());
	backtrackSteps=10; //<--Eventually should be made user-settable
}

void NelderMeadOptimizerContext::copyBounds()
{
	FreeVarGroup *varGroup = fc->varGroup;
	int px=0;
	for (size_t vx=0; vx < fc->profiledOut.size(); ++vx) {
		if (fc->profiledOut[vx]) continue;
		solLB[px] = varGroup->vars[vx]->lbound;
		if (!std::isfinite(solLB[px])) solLB[px] = NEG_INF;
		solUB[px] = varGroup->vars[vx]->ubound;
		if (!std::isfinite(solUB[px])) solUB[px] = INF;
		++px;
	}
}

void NelderMeadOptimizerContext::countConstraintsAndSetupBounds()
{
	solLB.resize(numFree);
	solUB.resize(numFree);
	copyBounds();
	
	omxState *globalState = fc->state;
	globalState->countNonlinearConstraints(numEqC, numIneqC, false);
	//If there aren't any of one of the two constraint types, then the
	//method for handling them shouldn't matter.  But, switching the
	//method to the simplest setting helps simplify programming logic:
	if(!numIneqC){NMobj->ineqConstraintMthd = 0;}
	if(!numEqC){NMobj->eqConstraintMthd = 1;}
	equality.resize(numEqC);
	inequality.resize(numIneqC);
}

int NelderMeadOptimizerContext::countNumFree()
{
	int nf = 0;
	for (size_t vx=0; vx < fc->profiledOut.size(); ++vx) {
		if (fc->profiledOut[vx]) continue;
		++nf;
	}
	return nf;
}

void NelderMeadOptimizerContext::copyParamsFromFitContext(double *ocpars)
{
	int px=0;
	for (size_t vx=0; vx < fc->profiledOut.size(); ++vx) {
		if (fc->profiledOut[vx]) continue;
		ocpars[px] = fc->est[vx];
		++px;
	}
}

void NelderMeadOptimizerContext::copyParamsFromOptimizer(Eigen::VectorXd &x, FitContext* fc2)
{
	int px=0;
	for (size_t vx=0; vx < fc2->profiledOut.size(); ++vx) {
		if (fc2->profiledOut[vx]) continue;
		fc2->est[vx] = x[px];
		++px;
	}
	fc2->copyParamToModel();
}

//----------------------------------------------------------------------

void NelderMeadOptimizerContext::enforceBounds(Eigen::VectorXd &x){
	int i=0;
	for(i=0; i < x.size(); i++){
		if(x[i] < solLB[i]){x[i] = solLB[i];}
		if(x[i] > solUB[i]){x[i] = solUB[i];}
	}
}

bool NelderMeadOptimizerContext::checkBounds(Eigen::VectorXd &x){
	bool retval=true;
	int i=0;
	for(i=0; i < x.size(); i++){
		if(x[i] < solLB[i] && x[i] > solUB[i]){
			retval=false;
			break;
		}
	}
	return(retval);
}

void NelderMeadOptimizerContext::evalIneqC()
{
	if(!numIneqC){return;}
	
	omxState *st = fc->state;
	
	int cur=0, j=0;
	for (j=0; j < int(st->conListX.size()); j++) {
		omxConstraint &con = *st->conListX[j];
		if (con.opCode == omxConstraint::EQUALITY) continue;
		//con.refreshAndGrab(fc, (omxConstraint::Type) ineqType, &inequality(cur));
		con.refreshAndGrab(fc, (omxConstraint::Type) con.opCode, &inequality(cur));
		//Nelder-Mead, of course, does not use constraint Jacobians...
		cur += con.size;
	}
	//Nelder-Mead will not care about the function values of inactive inequality constraints:
	inequality = inequality.array().max(0.0);
	
	if (NMobj->verbose >= 3) {
		mxPrintMat("inequality", inequality);
	}
	
}

void NelderMeadOptimizerContext::evalEqC()
{
	if(!numEqC){return;}
	
	omxState *st = fc->state;
	
	int cur=0, j=0;
	for(j = 0; j < int(st->conListX.size()); j++) {
		omxConstraint &con = *st->conListX[j];
		if (con.opCode != omxConstraint::EQUALITY) continue;
		con.refreshAndGrab(fc, &equality(cur));
		//Nelder-Mead, of course, does not use constraint Jacobians...
		cur += con.size;
	}
	
	if (NMobj->verbose >= 3) {
		mxPrintMat("equality", equality);
	}
}

double NelderMeadOptimizerContext::evalFit(Eigen::VectorXd &x)
{
	copyParamsFromOptimizer(x,fc);
	if(!std::isfinite(fc->fit)){return(NMobj->bignum);}
	else{
		double fv = fc->fit;
		if(NMobj->eqConstraintMthd==4){
			//TODO: add terms from augmented Lagrangian to fv
		}
		return(fv);
	}
}

void NelderMeadOptimizerContext::checkNewPointInfeas(Eigen::VectorXd &x, Eigen::Vector2i &ifcr)
{
	int i=0;
	double feasTol = NMobj->feasTol;
	ifcr.setZero(2);
	if(!numIneqC && !numEqC){return;}
	copyParamsFromOptimizer(x,fc);
	evalIneqC();
	evalEqC();
	if(numIneqC){
		for(i=0; i < inequality.size(); i++){
			if(inequality[i] > feasTol){
				ifcr[0] = 1;
				break;
			}
		}
	}
	if(numEqC){
		for(i=0; i < equality.size(); i++){
			if(fabs(equality[i]) > feasTol){
				ifcr[1] = 1;
				break;
			}
		}
	}
}

//TODO: rewrite this and evalNewPoint() using templates for Eigen-library classes, 
//to avoid copying rows of 'vertices' to temporary Eigen::Vectors:
void NelderMeadOptimizerContext::evalFirstPoint(Eigen::VectorXd &x, double fv, int infeas)
{
	Eigen::Vector2i ifcr;
	int ineqConstraintMthd = NMobj->ineqConstraintMthd;
	int eqConstraintMthd = NMobj->eqConstraintMthd;
	double bignum = NMobj->bignum;
	enforceBounds(x);
	checkNewPointInfeas(x, ifcr);
	if(!ifcr.sum()){
		infeas = 0L;
		fv = (evalFit(x));
		return;
	}
	else if(ifcr[1] || (ifcr[0] && ineqConstraintMthd)){
		switch(eqConstraintMthd){
		case 1:
			infeas = 1L;
			fv = bignum;
			return;
		case 2:
			//Can't backtrack to someplace else if it's the very first point.
			Rf_error("starting values not feasible; re-specify the initial simplex, or consider mxTryHard()");
			break;
		case 3:
			Rf_error("'GDsearch' Not Yet Implemented");
		case 4:
			if(ifcr[0]){
				fv = bignum;
				infeas = 1L;
			}
			else{
				fv = evalFit(x);
				infeas = 0L;
			}
			return;
		}
	}
	else if(ifcr[0]){
		fv = bignum;
		infeas = 1L;
		return;
	}
}

//oldpt is used for backtracking:
void NelderMeadOptimizerContext::evalNewPoint(Eigen::VectorXd &newpt, Eigen::VectorXd &oldpt, double fv, int infeas)
{
	Eigen::Vector2i ifcr;
	int ineqConstraintMthd = NMobj->ineqConstraintMthd;
	int eqConstraintMthd = NMobj->eqConstraintMthd;
	double bignum = NMobj->bignum;
	enforceBounds(newpt);
	checkNewPointInfeas(newpt, ifcr);
	if(!ifcr.sum()){
		infeas = 0L;
		fv = (evalFit(newpt));
		return;
	}
	else if(ifcr[1] || (ifcr[0] && ineqConstraintMthd)){
		switch(eqConstraintMthd){
		case 1:
			infeas = 1L;
			fv = bignum;
			return;
		case 2:
			Rf_error("'backtrack' Not Yet Implemented");
		case 3:
			Rf_error("'GDsearch' Not Yet Implemented");
		case 4:
			if(ifcr[0]){
				fv = bignum;
				infeas = 1L;
			}
			else{
				fv = evalFit(newpt);
				infeas = 0L;
			}
			return;
		}
	}
	else if(ifcr[0]){
		fv = bignum;
		infeas = 1L;
		return;
	}
}

void NelderMeadOptimizerContext::jiggleCoord(Eigen::VectorXd &xin, Eigen::VectorXd &xout){
	double a,b;
	int i;
	GetRNGstate();
	for(i=0; i < xin.size(); i++){
		b = Rf_runif(0.25,1.75);
		a = Rf_runif(-0.25,0.25);
		xout[i] = b*xin[i] + a;
	}
	PutRNGstate();
}

void NelderMeadOptimizerContext::initializeSimplex(Eigen::VectorXd startpt, double edgeLength)
{
	//vertices.resize(n,numFree);
	int i=0;
	Eigen::VectorXd xin, xout;
	if(NMobj->iniSimplexMtx.rows() && NMobj->iniSimplexMtx.cols()){
		Eigen::MatrixXd SiniSupp;
		if(NMobj->iniSimplexMtx.cols() != numFree){
			Rf_error("'iniSimplexMtx' has %d columns, but %d columns expected",NMobj->iniSimplexMtx.cols(), numFree);
		}
		if(NMobj->iniSimplexMtx.rows()>n){
			Rf_warning("'iniSimplexMtx' has %d rows, but %d rows expected; extraneous rows will be ignored",NMobj->iniSimplexMtx.rows(), n);
			NMobj->iniSimplexMtx.conservativeResize(n,numFree);
		}
		if(NMobj->iniSimplexMtx.rows()<n){
			Rf_warning("'iniSimplexMtx' has %d rows, but %d rows expected; omitted rows will be generated randomly",NMobj->iniSimplexMtx.rows(),n);
			SiniSupp.resize(n - NMobj->iniSimplexMtx.rows(), numFree);
			xin=NMobj->iniSimplexMtx.row(0);
			for(i=0; i<SiniSupp.rows(); i++){
				xout=SiniSupp.row(i);
				jiggleCoord(xin, xout);
				SiniSupp.row(i) = xout;
			}
		}
		for(i=0; i < NMobj->iniSimplexMtx.rows(); i++){
			vertices.row(i) = NMobj->iniSimplexMtx.row(i);
		}
		if(SiniSupp.rows()){
			for(i=0; i<SiniSupp.rows(); i++){
				vertices.row(i+NMobj->iniSimplexMtx.rows()) = SiniSupp.row(i);
			}
		}
	}
	else{
		//TODO: regular simplex for edge length other than 1.0
		double shhp = (1/n/sqrt(2))*(-1.0 + n + sqrt(1.0+n));
		double shhq = (1/n/sqrt(2))*(sqrt(1.0+n)-1);
		Eigen::VectorXd xin, xout;
		switch(NMobj->iniSimplexType){
		case 1:
			vertices.setZero(n,numFree);
			for(i=1; i<n+1; i++){
				vertices.row(i).setConstant(shhq);
				vertices(i,i-1) = shhp;
			}
			for(i=0; i<n+1; i++){
				vertices.row(i) += est;
			}
			break;
		case 3:
			//TODO
		case 2:
			vertices.row(0) = est;
			for(i=1; i<n+1; i++){
				vertices.row(i) = vertices.row(0);
				vertices(i,i-1) = vertices(0,i-1)+edgeLength;
			}
			break;
		case 4:
			vertices.row(0) = est;
			xin=vertices.row(0);
			for(i=1; i<n+1; i++){
				xout=vertices.row(i);
				jiggleCoord(xin,xout);
				vertices.row(i)=xout;
			}
			break;
		}
	}
	//Now evaluate each vertex (if not done already):
	Eigen::VectorXd newpt, oldpt;
	oldpt=vertices.row(0); //<--oldpt
	evalFirstPoint(oldpt, fvals[0], vertexInfeas[0]);
	vertices.row(0)=oldpt;
	for(i=1; i<n+1; i++){
		newpt = vertices.row(i); //<--newpt
		evalNewPoint(newpt, oldpt, fvals[i], vertexInfeas[i]);
		vertices.row(i) = newpt;
	}
}


void NelderMeadOptimizerContext::invokeNelderMead(){
	n = numFree - numEqC;
	vertices.resize(n+1,numFree);
	fvals.resize(n+1);
	vertexInfeas.resize(n+1);
	initializeSimplex(est, NMobj->iniSimplexEdge);
	Rf_error("NelderMeadOptimizerContext::invokeNelderMead() : so far, so good");
}


