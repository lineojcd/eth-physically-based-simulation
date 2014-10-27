#ifndef SOLVER_H
#define SOLVER_H

#include <iostream>

using namespace std;

class SolverBase  
{
	public:
		// constructor
		// iterations   maximum number of iterations to run the solver
		// dt           timestep being used by the fluid simulator
		SolverBase(int iterations, double dt, double accuracy) : 
			_iterations(iterations), _dt(dt), _accuracy(accuracy) {};

		// destructor
		virtual ~SolverBase() {};

		// Poisson solver
		// xRes     x resolution of pressure grid
		// yRes     y resolution of pressure grid
		// field    the pressure grid
		// b        right hand side of the Poisson problem
		virtual void solvePoisson(int xRes, int yRes, double* field, double* b) {};

	protected:
		int _iterations;  // maximum iterations to run the solver
		double _dt;        // timestep being used by the simulation
		double _accuracy; // how small should the residual be?
};

#endif
