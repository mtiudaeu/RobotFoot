/**
******************************************************************************^M
* @file    EigenUtils.h
* @authors  Camille Hébert & Antoine Rioux
* @date    2013-11-19
* @brief   Utility class to calculate some trajectories
******************************************************************************^M
*/

#ifndef EIGENUTILS_H
#define EIGENUTILS_H

#include "../../ThirdParty/Eigen/Dense"

namespace EigenUtils
{

	Eigen::MatrixXf MXB(Eigen::Vector2f pointA, Eigen::Vector2f pointB, float increment, int offset = 0);
	Eigen::MatrixXf CreateCombinedMXBMatrix(Eigen::MatrixXf matrixA, Eigen::MatrixXf matrixB, float increment, int i, int j, int offset = 0);
	Eigen::MatrixXf PseudoInverse(Eigen::MatrixXf matrix);
	Eigen::Matrix3f BaseChange(float angle);
};

#endif  //EIGENUTILS_H
