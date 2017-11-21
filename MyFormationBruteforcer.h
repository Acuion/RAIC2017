#pragma once

#include "model\VehicleType.h"
#include <algorithm>
#include <vector>

using namespace std;
using xypoint = pair<int, int>;

struct FormationStep
{
	model::VehicleType mVt;
	xypoint mMoveTo;
};

class MyFormationBruteforcer
{
public:
	void buildPathToFormation();

	MyFormationBruteforcer(xypoint tankStartCell, xypoint ifvStartCell, xypoint arrvStartCell);
	~MyFormationBruteforcer();
private:
	void buildPathToPoints(xypoint targetTankCell, xypoint targetIfvCell, xypoint targetArrvCell);

	vector<FormationStep> mCurrentFormationPath;

	xypoint mTankStartCell;
	xypoint mIfvStartCell;
	xypoint mArrvStartCell;
};

