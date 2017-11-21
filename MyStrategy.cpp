#include "MyStrategy.h"

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
	for (auto& q : ourVehicles)
		if (q.second.getType() == vt)
		{
			x += q.second.getX();
			y += q.second.getY();
			count++;
		}

	return { x / count, y / count };
}

void MyStrategy::firstTickActions(const Player& me, const World& world, const Game& game, Move& move)
{
	for (auto& x : world.getNewVehicles())
	{
		if (x.getPlayerId() != me.getId())
			enemyVehicles[x.getId()] = x;
		else
			ourVehicles[x.getId()] = x;
	}

	xypoint tankCenter, helicopterCenter, ifvCenter, fighterCenter, arrvCenter;
	tankCenter = getCenterOfGroup(VehicleType::TANK);
	helicopterCenter = getCenterOfGroup(VehicleType::HELICOPTER);
	ifvCenter = getCenterOfGroup(VehicleType::IFV);
	fighterCenter = getCenterOfGroup(VehicleType::FIGHTER);
	arrvCenter = getCenterOfGroup(VehicleType::ARRV);

	//cout << tankCenter.first << " " << tankCenter.second << endl;
	//cout << helicopterCenter.first << " " << helicopterCenter.second << endl;
	//cout << ifvCenter.first << " " << ifvCenter.second << endl;
	//cout << fighterCenter.first << " " << fighterCenter.second << endl;
	//cout << arrvCenter.first << " " << arrvCenter.second << endl;

	xypoint tankCell, helicopterCell, ifvCell, fighterCell, arrvCell;

	tankCell.first = round((tankCenter.first - 45) / 75.0);
	tankCell.second = round((tankCenter.second - 45) / 75.0);
	helicopterCell.first = round((helicopterCenter.first - 45) / 75.0);
	helicopterCell.second = round((helicopterCenter.second - 45) / 75.0);
	ifvCell.first = round((ifvCenter.first - 45) / 75.0);
	ifvCell.second = round((ifvCenter.second - 45) / 75.0);
	fighterCell.first = round((fighterCenter.first - 45) / 75.0);
	fighterCell.second = round((fighterCenter.second - 45) / 75.0);
	arrvCell.first = round((arrvCenter.first - 45) / 75.0);
	arrvCell.second = round((arrvCenter.second - 45) / 75.0);

	// 2 * 3 * 9^4
	// ������� ��� ������ ����� ������� ������ ��������
	// ����� �� �����
	// ���������� ��� ���� ����
}

void MyStrategy::move(const Player& me, const World& world, const Game& game, Move& move)
{
	if (world.getTickIndex() == 0)
		firstTickActions(me, world, game, move);

	while (delayedFunctions.size() && delayedFunctions.top().first <= world.getTickIndex())
	{
		executionQueue.push_back(delayedFunctions.top().second);
		delayedFunctions.pop();
	}

	if (executionQueue.size() && world.getTickIndex() % 5 == 0)
	{
		executionQueue.front()(move, world);
		executionQueue.pop_front();
	}

	// TANK && FIGHTER
	// IFV & HELICOPTER
	// ARRV
	// On a circle, or just a block
}

MyStrategy::MyStrategy()
{
}