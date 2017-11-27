#pragma once

#include "model/VehicleType.h"
#include <algorithm>
#include <vector>

using namespace std;
using xypoint = pair<int, int>;

struct FormationStep
{
	model::VehicleType mVt;
	xypoint mMoveFrom, mMoveTo;
};

class MyFormationBruteforcer
{
public:
	void buildPathToFormation();
	vector<FormationStep> getLandFormationPath() const;
	vector<FormationStep> getAirFormationPath() const;
	vector<xypoint> getLandFormation() const;

	MyFormationBruteforcer(xypoint tankStartCell, xypoint ifvStartCell, xypoint arrvStartCell, xypoint fighterStartCell, xypoint helicopterStartCell);
	~MyFormationBruteforcer();
private:
	void buildLandPathToPoints(xypoint targetTankCell, xypoint targetIfvCell, xypoint targetArrvCell);
	void buildAirPath();

	bool mLandPathIsEmpty, mAirPathIsEmpty;
	vector<FormationStep> mCurrentLandFormationPath, mCurrentAirFormationPath;
	vector<xypoint> mFinalLandFormation;

	xypoint mTankStartCell;
	xypoint mIfvStartCell;
	xypoint mArrvStartCell;
	xypoint mFighterStartCell;
	xypoint mHelicopterStartCell;
};

