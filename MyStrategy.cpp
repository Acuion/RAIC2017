#include "MyStrategy.h"
#include "MyFormationBruteforcer.h"

#define PI 3.14159265358979323846
#define _USE_MATH_DEFINES

void MyStrategy::selectVehicles(VehicleType vt, Move& mv)
{
	mv.setAction(ActionType::CLEAR_AND_SELECT);
	mv.setRight(1024);
	mv.setBottom(1024);
	mv.setVehicleType(vt);
}

xypoint MyStrategy::getCenterOfGroup(VehicleType vt)
{
	int x = 0, y = 0, count = 0;
	for (auto& q : mOurVehicles)
		if (q.second.getType() == vt)
		{
			x += q.second.getX();
			y += q.second.getY();
			count++;
		}

	return {x / count, y / count};
}

void MyStrategy::firstTickActions(const Player& me, const World& world, const Game& game, Move& move)
{
	for (auto& x : world.getNewVehicles())
	{
		if (x.getPlayerId() != me.getId())
			mEnemyVehicles[x.getId()] = x;
		else
			mOurVehicles[x.getId()] = x;
	}

	const xypoint tankCenter = getCenterOfGroup(VehicleType::TANK);
	const xypoint helicopter_center = getCenterOfGroup(VehicleType::HELICOPTER);
	const xypoint ifvCenter = getCenterOfGroup(VehicleType::IFV);
	const xypoint fighterCenter = getCenterOfGroup(VehicleType::FIGHTER);
	const xypoint arrvCenter = getCenterOfGroup(VehicleType::ARRV);

	//cout << tankCenter.first << " " << tankCenter.second << endl;
	//cout << helicopterCenter.first << " " << helicopterCenter.second << endl;
	//cout << ifvCenter.first << " " << ifvCenter.second << endl;
	//cout << fighterCenter.first << " " << fighterCenter.second << endl;
	//cout << arrvCenter.first << " " << arrvCenter.second << endl;

	xypoint tankCell, helicopterCell, ifvCell, fighterCell, arrvCell;

	tankCell.first = round((tankCenter.first - 45) / 75.0);
	tankCell.second = round((tankCenter.second - 45) / 75.0);
	helicopterCell.first = round((helicopter_center.first - 45) / 75.0);
	helicopterCell.second = round((helicopter_center.second - 45) / 75.0);
	ifvCell.first = round((ifvCenter.first - 45) / 75.0);
	ifvCell.second = round((ifvCenter.second - 45) / 75.0);
	fighterCell.first = round((fighterCenter.first - 45) / 75.0);
	fighterCell.second = round((fighterCenter.second - 45) / 75.0);
	arrvCell.first = round((arrvCenter.first - 45) / 75.0);
	arrvCell.second = round((arrvCenter.second - 45) / 75.0);

	auto mfb = MyFormationBruteforcer(tankCell, ifvCell, arrvCell);
	mfb.buildPathToFormation();

	int turnNum = 0;
	for (auto& turn : mfb.getFormationPath())
	{
		switch (turn.mVt)
		{
		case VehicleType::ARRV:
			break;
		case VehicleType::IFV:
			break;
		case VehicleType::TANK:
			break;
		default:
			throw;
		}
	}

	// 2 * 3 * 9^4
	// столбец или строка будет являтся местом упаковки
	// какая из строк
	// перебираем кто куда едет
}

void MyStrategy::move(const Player& me, const World& world, const Game& game, Move& move)
{
	if (world.getTickIndex() == 0)
		firstTickActions(me, world, game, move);

	while (mDelayedFunctions.size() && mDelayedFunctions.top().first <= world.getTickIndex())
	{
		mExecutionQueue.push_back(mDelayedFunctions.top().second);
		mDelayedFunctions.pop();
	}

	if (mExecutionQueue.size() && world.getTickIndex() % 5 == 0)
	{
		mExecutionQueue.front()(move, world);
		mExecutionQueue.pop_front();
	}

	// TANK && FIGHTER
	// IFV & HELICOPTER
	// ARRV
	// On a circle, or just a block
}

MyStrategy::MyStrategy()
{
}
