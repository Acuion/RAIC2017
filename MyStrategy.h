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
#include "MyGlobalInfoStorer.h"
#include "MyUnitGroup.h"

using namespace model;
using namespace std;
using turnPrototype = function<void(Move&, const World&)>;
using xypoint = pair<int, int>;

class MyStrategy : public Strategy
{
public:
    MyStrategy();

    void move(const model::Player& me, const model::World& world, const model::Game& game, model::Move& move) override;
private:
	void firstTickActions(const Player& me, const World& world, const Game& game, Move& move);
	void selectVehicles(VehicleType vt, Move& mv);
	bool nukeEmAll(const Player& me, const model::World& world, model::Move& move);

	bool mPanic;
	int mPanicTime;
	xypoint mPanicPoint;
	int mLastNuke;
	turnPrototype mInfinityChase;

	deque<turnPrototype> mMacroExecutionQueue;
	deque<pair<function<bool(const World&)>, turnPrototype>> mMacroConditionalQueue;

	MyGlobalInfoStorer mGlobaler;

	vector<MyUnitGroup> mUnitGroups;
};

#endif
