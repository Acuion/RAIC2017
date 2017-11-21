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

using namespace model;
using namespace std;
using turnPrototype = function<void(Move&, const World&)>;
using xypoint = pair<int, int>;

class MyStrategy : public Strategy {
public:
    MyStrategy();

    void move(const model::Player& me, const model::World& world, const model::Game& game, model::Move& move) override;
private:
	class ComparisonClass
	{
	public:
		bool operator() (const pair<int, function<void(Move&, const World&)>>& a, const pair<int, function<void(Move&, const World&)>>& b)
		{
			return b.first < a.first;
		}
	};

	void firstTickActions(const Player& me, const World& world, const Game& game, Move& move);
	xypoint getCenterOfGroup(VehicleType vt);
	void selectVehicles(VehicleType vt, Move& mv);

	deque<turnPrototype> mExecutionQueue;
	map<int, Vehicle> mOurVehicles, mEnemyVehicles;
	priority_queue<pair<int, turnPrototype>,
		vector<pair<int, turnPrototype>>,
		ComparisonClass> mDelayedFunctions;
};

#endif
