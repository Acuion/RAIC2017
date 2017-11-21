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
	vector<FormationStep> getFormationPath() const;
	vector<xypoint> getFormation() const;

	MyFormationBruteforcer(xypoint tankStartCell, xypoint ifvStartCell, xypoint arrvStartCell);
	~MyFormationBruteforcer();
private:
	void buildPathToPoints(xypoint targetTankCell, xypoint targetIfvCell, xypoint targetArrvCell);

	bool mPathIsEmpty;
	vector<FormationStep> mCurrentFormationPath;
	vector<xypoint> mFinalFormation;

	xypoint mTankStartCell;
	xypoint mIfvStartCell;
	xypoint mArrvStartCell;
};

