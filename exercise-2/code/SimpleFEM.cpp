#include "SimpleFEMDefs.h"
#include "SimpleFEM.h"
#include "MeshViewer.h"
#include <numeric>

// size of grid
static const size_t GRIDSIZE = 32;
// use a graded mesh, or a regular mesh
static const bool gradedMesh = false;
// laplace or poisson problem?
static const bool laplaceProblem = false;
// plot solution or error?
static bool vizSolution = true;
// display debug information?
static const bool debugOut = false;


float eval_u(float x, float y)
{
	if(laplaceProblem)
		//return x*x - y*y;
		return exp(x)*sin(y);
	else 
		return 	3.f*x*x + 2.f*y*y*y*x;
}

float eval_f(float x, float y)
{
	if(laplaceProblem)
		return 0;
	else
		return 	-6.f - 12.f*y*x;
}


FEMMesh SimpleFEM::CreateUniformGridMesh(size_t nodesX, size_t nodesY)
{
	assert(nodesX >= 2);
	assert(nodesY >= 2);

	FEMMesh mesh;

	// setup positions of nodes
	int nodecnt =0;
	for(size_t y=0; y<nodesY; y++)
	{
		for(size_t x=0; x<nodesX; x++)
		{
			Vector2 pos = Vector2((float)x / (float)(nodesX-1), (float)y / (float)(nodesY-1));

			//shift mesh positions for graded mesh
			if(gradedMesh)
			{
				pos[0] *= pos[0]; pos[1] *= pos[1];
				//pos[0] = sqrt(pos[0]); pos[1] = sqrt(pos[1]);
			}
			mesh.AddNode(pos);
			nodecnt++;
		}
	}
	std::cout<<"Added "<<nodecnt<<" nodes to mesh.\n";

	// create elements
	int cnt=0;
	for(size_t y=0; y<nodesY-1; y++)
	{
		for(size_t x=0; x<nodesX-1; x++)
		{
			// bottom-left:
			size_t node00 = y*nodesX + x;

			// bottom-right:
			size_t node10 = node00 + 1;

			// top-left:
			size_t node01 = node00 + nodesX;

			// top-right:
			size_t node11 = node00 + nodesX + 1;

			// add two element for this quad
			mesh.AddElement( FEMElementTri(node00, node10, node11) );
			mesh.AddElement( FEMElementTri(node00, node11, node01) );
			cnt+=2;
		}
	}
	std::cout<<"Added "<<cnt<<" elements to mesh.\n";

	return mesh;
}

void SimpleFEM::ComputeBoundaryConditions(const FEMMesh &mesh, vector<BoundaryCondition> &boundaryConditions)
{
	boundaryConditions.clear();

	for(size_t nodeID=0; nodeID<mesh.GetNumNodes(); nodeID++)
	{
		const Vector2 &pos = mesh.GetNodePosition(nodeID);

		if(isOnBoundary(pos)) {
			float x = pos[0];
			float y = pos[1]; 

			// compute reference solution on boundary
			//float val = 3.*x*x + 2.*y*y*y*x;
			float val = eval_u(x,y);

			// this fixes the solution for node "nodeID" to "val" when
			// solving the system later on
			boundaryConditions.push_back( BoundaryCondition(nodeID, val) );
		}
	}
}


// perform a simple boundary check
// is either of the components 0 or 1?
bool SimpleFEM::isOnBoundary(const Vector2 &pos)
{
	return pos[0] <= 0. || pos[0] >= 1. || pos[1] <= 0. || pos[1] >= 1.;
}


void SimpleFEM::ComputeRHS(const FEMMesh &mesh,  vector<float> &rhs)
{
	for(size_t ie=0; ie<mesh.GetNumElements(); ie++) {
		const FEMElementTri& elem = mesh.GetElement(ie);

		Vector2 node_1 = mesh.GetNodePosition(elem.GetGlobalNodeForElementNode(0));
		Vector2 node_2 = mesh.GetNodePosition(elem.GetGlobalNodeForElementNode(1));
		Vector2 node_3 = mesh.GetNodePosition(elem.GetGlobalNodeForElementNode(2));

		Vector2 middlePt((node_1[0] + node_2[0]) / 2, (node_1[1] + node_2[1]) / 2); double h = Vector2(node_3 - middlePt).length();
		double area = (node_1 - node_2).length() * (h / 2);

		Vector2 barycenter = (node_1 + node_2 + node_3) / 3;

		// approximate Ni (i=1,2,3) at the barycenters
		Vector2 n2_n3_mid((node_2 + node_3) / 2); float n1_h = (node_1 - n2_n3_mid).length(); float n1_grad_h = (barycenter - n2_n3_mid).length();
		float n1_scale = n1_grad_h / n1_h;

		Vector2 n1_n3_mid((node_1 + node_3) / 2); float n2_h = (node_2 - n1_n3_mid).length(); float n2_grad_h = (barycenter - n1_n3_mid).length();
		float n2_scale = n2_grad_h / n2_h;

		Vector2 n1_n2_mid((node_1 + node_2) / 2); float n3_h = (node_3 - n1_n2_mid).length(); float n3_grad_h = (barycenter - n1_n2_mid).length();
		float n3_scale = n3_grad_h / n3_h;

		double barycenter_x = barycenter[0]; double barycenter_y = barycenter[1]; double f_val = eval_f(barycenter_x, barycenter_y);

		// use the quadrature rule for each node in an element
		rhs[elem.GetGlobalNodeForElementNode(0)] += area * (f_val * n1_scale);
		rhs[elem.GetGlobalNodeForElementNode(1)] += area * (f_val * n2_scale);
		rhs[elem.GetGlobalNodeForElementNode(2)] += area * (f_val * n3_scale);
	}
}

void SimpleFEM::computeError(FEMMesh &mesh,  const vector<float> &sol_num, vector<float> &verror, float& err_nrm )
{
	unsigned int nodes_num = mesh.GetNumNodes();
	for (unsigned int i = 0; i < nodes_num; i++) {
		Vector2 node = mesh.GetNodePosition(i);
		verror[i] = fabs(sol_num[i] - eval_u(node[0], node[1]));
	}
	
	vector<float> mat_times_v(nodes_num); mesh.getMat().MultVector(verror, mat_times_v);
	err_nrm = sqrt(inner_product(mat_times_v.begin(), mat_times_v.end(), verror.begin(), 0.f));
}

int main(int argc, char *argv[])
{
	// Create a uniform mesh:
	FEMMesh mesh = SimpleFEM::CreateUniformGridMesh(GRIDSIZE, GRIDSIZE);
	int nNodes = mesh.GetNumNodes();
	// Build its stiffness matrix:
	// loop over all elements, and compute their contributions
	// for the equations of their respective nodes
	mesh.getMat().ClearResize( mesh.GetNumNodes() );
	for(size_t i=0; i<mesh.GetNumElements(); i++) {
		if(debugOut) cout<<"Assembling "<<i<<"\n";
		mesh.GetElement(i).Assemble( &mesh );
	}

	// Compute boundary conditions and right-hand side:
	vector<BoundaryCondition> boundaryConditions;

	SimpleFEM::ComputeBoundaryConditions(mesh, boundaryConditions);

	// Apply right-hand side:
	vector<float> rhs(nNodes);
	SimpleFEM::ComputeRHS(mesh, rhs);
	mesh.SetRHS(rhs);

	// Solve the problem, this calls a preconditioned CG solver
	// for the sparse matrix with right hand side rhs
	// all nodes stored in "boundaryConditions" are fixed to certain values
	vector<float> solution;
	bool isSolved = mesh.Solve(solution, boundaryConditions);
	assert(isSolved);

	// print matrix for boundary nodes
	if(debugOut) 
		for(size_t i=0; i<mesh.GetNumNodes(); i++) {
			const Vector2 & pi = mesh.GetNodePosition(i);
			if(SimpleFEM::isOnBoundary(pi))
				continue;
			for(size_t j=0; j<mesh.GetNumNodes(); j++) {
				const Vector2 & pj = mesh.GetNodePosition(j);
				if(SimpleFEM::isOnBoundary(pj))
					continue;
				if(j>i) cout<<mesh.getMat()(j,i)<<"\t";
				else    cout<<mesh.getMat()(i,j)<<"\t";
			}
			std::cout<<std::endl;
		//cout<<" rhs="<<rhs[i]<<" \n";
	} // debug output

	float err_nrm = 0;
	std::vector<float> verr(nNodes);
	SimpleFEM::computeError(mesh,solution,verr,err_nrm);
	printf("Error norm is %f\n",err_nrm);
	// Visualize the solution:
	// draw the triangles with colors according to solution
	// blue means zero, red means maxValue.
	// the default problem goes from 0-5 , for other problems, 
	// adjust the maxValue parameter below (values <0, or >maxValue
	// are clamped for the display)
	MeshViewer viewer(argc, argv);

	if(vizSolution)
		viewer.VisualizeSolution(mesh, solution);
	else 
		viewer.VisualizeError(mesh, verr);


	return 0;
}


