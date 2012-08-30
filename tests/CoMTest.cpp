// This file is part of RBDyn.
//
// RBDyn is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RBDyn is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with RBDyn.  If not, see <http://www.gnu.org/licenses/>.

// includes
// std
#include <iostream>

// boost
#define BOOST_TEST_MODULE BodyTest
#include <boost/test/unit_test.hpp>
#include <boost/math/constants/constants.hpp>

// SpaceVecAlg
#include <SpaceVecAlg>

// RBDyn
#include "EulerIntegration.h"
#include "FK.h"
#include "FV.h"
#include "MultiBody.h"
#include "MultiBodyConfig.h"
#include "MultiBodyGraph.h"
#include "CoM.h"

const double TOL = 0.000001;

BOOST_AUTO_TEST_CASE(computeCoMTest)
{
	using namespace Eigen;
	using namespace sva;
	using namespace rbd;
	namespace cst = boost::math::constants;

	MultiBodyGraph mbg;

	double mass = 1.;
	Matrix3d I = Matrix3d::Identity();
	Vector3d h = Vector3d::Zero();

	RBInertia rbi(mass, h, I);

	Body b0(rbi, 0, "b0");
	Body b1(rbi, 1, "b1");
	Body b2(RBInertia(2., h, I), 2, "b2");
	Body b3(rbi, 3, "b3");

	mbg.addBody(b0);
	mbg.addBody(b1);
	mbg.addBody(b2);
	mbg.addBody(b3);

	Joint j0(Joint::RevX, true, 0, "j0");
	Joint j1(Joint::RevY, true, 1, "j1");
	Joint j2(Joint::RevZ, true, 2, "j2");

	mbg.addJoint(j0);
	mbg.addJoint(j1);
	mbg.addJoint(j2);


	//  Root     j0      j1     j2
	//  ---- b0 ---- b1 ---- b2 ----b3
	//  Fixed    RevX   RevY    RevZ

	PTransform to(Vector3d(0., 0.5, 0.));
	PTransform from(Vector3d(0., 0., 0.));

	mbg.linkBodies(0, to, 1, from, 0);
	mbg.linkBodies(1, to, 2, from, 1);
	mbg.linkBodies(2, to, 3, from, 2);


	MultiBody mb = mbg.makeMultiBody(0, true);
	MultiBodyConfig mbc(mb);

	mbc.q = {{}, {0.}, {0.}, {0.}};

	forwardKinematics(mb, mbc);

	Vector3d CoM = computeCoM(mb, mbc);

	double handCoMX = 0.;
	double handCoMY = (0.5*1. + 1.*2. + 1.5*1.)/4.;
	double handCoMZ = 0.;
	BOOST_CHECK_EQUAL(CoM, Vector3d(handCoMX, handCoMY, handCoMZ));



	mbc.q = {{}, {cst::pi<double>()/2.}, {0.}, {0.}};
	forwardKinematics(mb, mbc);

	CoM = sComputeCoM(mb, mbc);

	handCoMX = 0.;
	handCoMY = (0.5*1. + 0.5*2 + 0.5*1.)/4.;
	handCoMZ = (0.5*2. + 1.*1.)/4.;

	BOOST_CHECK_EQUAL(CoM, Vector3d(handCoMX, handCoMY, handCoMZ));


	// test safe version
	mbc.bodyPosW = {I, I, I};
	BOOST_CHECK_THROW(sComputeCoM(mb, mbc), std::domain_error);
}


Eigen::Vector3d makeCoMDotFormStep(const rbd::MultiBody& mb,
	const rbd::MultiBodyConfig& mbc)
{
	using namespace Eigen;
	using namespace rbd;

	double step = 1e-8;

	MultiBodyConfig mbcTmp(mbc);

	Vector3d oC = computeCoM(mb, mbcTmp);
	eulerIntegration(mb, mbcTmp, step);
	forwardKinematics(mb, mbcTmp);
	forwardVelocity(mb, mbcTmp);
	Vector3d nC = computeCoM(mb, mbcTmp);

	return (nC - oC)/step;
}


BOOST_AUTO_TEST_CASE(CoMJacobianDummyTest)
{
	using namespace Eigen;
	using namespace sva;
	using namespace rbd;
	namespace cst = boost::math::constants;

	typedef Eigen::Matrix<double, 1, 1> EScalar;

	MultiBodyGraph mbg;

	double mass = 1.;
	Matrix3d I = Matrix3d::Identity();
	Vector3d h = Vector3d::Zero();

	RBInertia rbi(mass, h, I);

	Body b0(RBInertia(EScalar::Random()(0)*10., h, I), 0, "b0");
	Body b1(RBInertia(EScalar::Random()(0)*10., h, I), 1, "b1");
	Body b2(RBInertia(EScalar::Random()(0)*10., h, I), 2, "b2");
	Body b3(RBInertia(EScalar::Random()(0)*10., h, I), 3, "b3");
	Body b4(RBInertia(EScalar::Random()(0)*10., h, I), 4, "b4");

	mbg.addBody(b0);
	mbg.addBody(b1);
	mbg.addBody(b2);
	mbg.addBody(b3);
	mbg.addBody(b4);

	Joint j0(Joint::RevX, true, 0, "j0");
	Joint j1(Joint::RevY, true, 1, "j1");
	Joint j2(Joint::RevZ, true, 2, "j2");
	Joint j3(Joint::Spherical, true, 3, "j3");

	mbg.addJoint(j0);
	mbg.addJoint(j1);
	mbg.addJoint(j2);
	mbg.addJoint(j3);

	//                b4
	//             j3 | Spherical
	//  Root     j0   |   j1     j2
	//  ---- b0 ---- b1 ---- b2 ----b3
	//  Fixed    RevX   RevY    RevZ


	PTransform to(Vector3d(0., 0.5, 0.));
	PTransform from(Vector3d(0., -0.5, 0.));


	mbg.linkBodies(0, to, 1, from, 0);
	mbg.linkBodies(1, to, 2, from, 1);
	mbg.linkBodies(2, to, 3, from, 2);
	mbg.linkBodies(1, PTransform(Vector3d(0.5, 0., 0.)),
								 4, PTransform(Vector3d(-0.5, 0., 0.)), 3);

	MultiBody mb = mbg.makeMultiBody(0, true);
	CoMJacobianDummy comJac(mb);

	MultiBodyConfig mbc(mb);

	mbc.q = {{}, {0.}, {0.}, {0.}, {1., 0., 0., 0.}};
	mbc.alpha = {{}, {0.}, {0.}, {0.}, {0., 0., 0.}};
	mbc.alphaD = {{}, {0.}, {0.}, {0.}, {0., 0., 0.}};

	forwardKinematics(mb, mbc);

	for(std::size_t i = 0; i < mb.nrJoints(); ++i)
	{
		for(int j = 0; j < mb.joint(i).dof(); ++j)
		{
			mbc.alpha[i][j] = 1.;
			forwardVelocity(mb, mbc);

			Vector3d CDot_diff = makeCoMDotFormStep(mb, mbc);
			MatrixXd CJac = comJac.jacobian(mb, mbc);

			BOOST_CHECK_EQUAL(CJac.rows(), 6);
			BOOST_CHECK_EQUAL(CJac.cols(), mb.nrDof());

			VectorXd alpha = dofToVector(mb, mbc.alpha);

			Vector3d CDot = CJac.block(3, 0, 3, mb.nrDof())*alpha;

			BOOST_CHECK_SMALL((CDot_diff - CDot).norm(), TOL);
			mbc.alpha[i][j] = 0.;
		}
	}

	for(std::size_t i = 0; i < mb.nrJoints(); ++i)
	{
		for(int j = 0; j < mb.joint(i).dof(); ++j)
		{
			mbc.alpha[i][j] = 1.;
			forwardVelocity(mb, mbc);

			Vector3d CDot_diff = makeCoMDotFormStep(mb, mbc);
			MatrixXd CJac = comJac.jacobian(mb, mbc);

			VectorXd alpha = dofToVector(mb, mbc.alpha);

			Vector3d CDot = CJac.block(3, 0, 3, mb.nrDof())*alpha;

			BOOST_CHECK_SMALL((CDot_diff - CDot).norm(), TOL);
		}
	}
	mbc.alpha = {{}, {0.}, {0.}, {0.}, {0., 0., 0.}};



	Quaterniond q;
	q = AngleAxisd(cst::pi<double>()/8., Vector3d::UnitZ());
	mbc.q = {{}, {0.4}, {0.2}, {-0.1}, {q.w(), q.x(), q.y(), q.z()}};
	forwardKinematics(mb, mbc);

	for(std::size_t i = 0; i < mb.nrJoints(); ++i)
	{
		for(int j = 0; j < mb.joint(i).dof(); ++j)
		{
			mbc.alpha[i][j] = 1.;
			forwardVelocity(mb, mbc);

			Vector3d CDot_diff = makeCoMDotFormStep(mb, mbc);
			MatrixXd CJac = comJac.sJacobian(mb, mbc);

			VectorXd alpha = dofToVector(mb, mbc.alpha);

			Vector3d CDot = CJac.block(3, 0, 3, mb.nrDof())*alpha;

			BOOST_CHECK_SMALL((CDot_diff - CDot).norm(), TOL);
			mbc.alpha[i][j] = 0.;
		}
	}

	for(std::size_t i = 0; i < mb.nrJoints(); ++i)
	{
		for(int j = 0; j < mb.joint(i).dof(); ++j)
		{
			mbc.alpha[i][j] = 1.;
			forwardVelocity(mb, mbc);

			Vector3d CDot_diff = makeCoMDotFormStep(mb, mbc);
			MatrixXd CJac = comJac.sJacobian(mb, mbc);

			VectorXd alpha = dofToVector(mb, mbc.alpha);

			Vector3d CDot = CJac.block(3, 0, 3, mb.nrDof())*alpha;

			BOOST_CHECK_SMALL((CDot_diff - CDot).norm(), TOL);
		}
	}
	mbc.alpha = {{}, {0.}, {0.}, {0.}, {0., 0., 0.}};


	mbc.bodyPosW = {I, I, I};
	BOOST_CHECK_THROW(comJac.sJacobian(mb, mbc), std::domain_error);
}

