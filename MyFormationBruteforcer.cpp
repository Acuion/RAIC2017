#include "MyFormationBruteforcer.h"

void MyFormationBruteforcer::buildPathToFormation()
{
	vector<vector<xypoint>> rows = 
	{
		{ make_pair(0,0), make_pair(0,1), make_pair(0,2) },
		{ make_pair(1,0), make_pair(1,1), make_pair(1,2) },
		{ make_pair(2,0), make_pair(2,1), make_pair(2,2) },
	};
	vector<vector<xypoint>> cols =
	{
		{ make_pair(0,0), make_pair(1,0), make_pair(2,0) },
		{ make_pair(0,1), make_pair(1,1), make_pair(2,1) },
		{ make_pair(0,2), make_pair(1,2), make_pair(2,2) },
	};

	for (auto& x : rows)
		do
		{
			buildPathToPoints(x[0], x[1], x[2]);
		} while (next_permutation(x.begin(), x.end()));

	for (auto& x : cols)
		do
		{
			buildPathToPoints(x[0], x[1], x[2]);
		} while (next_permutation(x.begin(), x.end()));
}

MyFormationBruteforcer::MyFormationBruteforcer(xypoint tankStartCell, xypoint ifvStartCell, xypoint arrvStartCell)
	: mTankStartCell(tankStartCell)
	, mIfvStartCell(ifvStartCell)
	, mArrvStartCell(arrvStartCell)
{
}

MyFormationBruteforcer::~MyFormationBruteforcer()
{
}

void MyFormationBruteforcer::buildPathToPoints(xypoint targetTankCell, xypoint targetIfvCell, xypoint targetArrvCell)
{

}
