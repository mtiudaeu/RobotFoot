#include "Trajectory.h"
#include "MotionControl.h"
#include "EigenUtils.h"
#include "DenavitHartenberg.h"

#include "../../ThirdParty/Eigen/Dense"

#include <iostream>
#include <fstream>
#include <cstdlib>
using namespace std;

int main(int argc, char* argv[]) {
	Eigen::Vector2f pointA(0, 0);
	Eigen::Vector2f pointD(3, 0.5);
	Eigen::Vector2f startAngle(0, 0);
	Eigen::Vector2f endAngle(0, 0);


	Trajectory* traj = new Trajectory();
	/*Eigen::MatrixXf matrix = traj->GenerateWalk(pointA, pointD, startAngle,
			endAngle);
*/

	Eigen::Vector4f rightInit(0.037, 0, 0, 0);
	Eigen::Vector4f rightFinal(0.037, 0, 0, 0);

	Eigen::Vector4f leftInit(-0.037, 0, 0, 0);
	Eigen::Vector4f leftFinal(-0.037, 0.04, 0, 0);

	Eigen::Vector4f pelvisInit(0, 0, 0.29672, 0);
	Eigen::Vector4f pelvisFinal(0, 0, 0.29672, 0);

	Eigen::MatrixXf matrix = traj->GenerateMovement(rightInit, rightFinal, leftInit, leftFinal, pelvisInit, pelvisFinal, 1);

	MotionControl* motion = new MotionControl();
	motion->Walk(matrix);


	ofstream myfiletraj;
	myfiletraj.open ("matrixTraj.txt");

	for(int i = 0; i < matrix.rows(); i++)
	{
		//*****************write to a file for tests************************************//
		myfiletraj << matrix.row(i) << endl;
		//*****************************************************************************//
	}

	myfiletraj.close();

	Eigen::MatrixXf matrice(3, 6);
	matrice << -0.1747, -0.1747, -0.0874, 0, 0,0,
			0, 0, -0.0319, 0, 0, 0,
			0, 0, 0, 0, 0, 0;


	Eigen::MatrixXf testPseudoInv = EigenUtils::PseudoInverse(matrice);

	return 0;
}
