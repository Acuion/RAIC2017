#pragma once

#ifndef _MY_STRATEGY_H_
#define _MY_STRATEGY_H_

#include "Strategy.h"
#include <cmath>
#include <cstdlib>
#include <functional>
#include <queue>
#include <map>
#include <iostream>
#include <list>

using namespace model;
using namespace std;
using turnPrototype = function<void(Move&, const World&)>;
using xypoint = pair<int, int>;

struct VehicleBasicInfo
{
	double mX, mY;
	VehicleType mType;
};

class MyStrategy : public Strategy {
public:
    MyStrategy();

    void move(const model::Player& me, const model::World& world, const model::Game& game, model::Move& move) override;
private:
	class ComparisonClass
	{
	public:
		bool operator() (const pair<int, function<void(Move&, const World&)>>& a, const pair<int, function<void(Move&, const World&)>>& b) const
		{
			return b.first < a.first;
		}
	};

	void firstTickActions(const Player& me, const World& world, const Game& game, Move& move);
	xypoint getCenterOfGroup(VehicleType vt);
	void selectVehicles(VehicleType vt, Move& mv);

	bool mOurUnitsDontMoving;
	int mLastNuke;
	turnPrototype mInfinityChase;

	deque<turnPrototype> mExecutionQueue;
	map<int, VehicleBasicInfo> mOurVehicles, mEnemyVehicles;
	deque<pair<function<bool(const World&)>, turnPrototype>> mDelayedFunctions;
};

#endif
