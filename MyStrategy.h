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
#include <memory>

using namespace model;
using namespace std;
using macroTurnPrototype = function<void(Move&, const World&)>;
using groupTurnPrototype = function<void(Move&, const World&, MyUnitGroup&)>;
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
	shared_ptr<MyUnitGroup> createGroup(Move& move, const World& world);

	void lockMacroInterruptions();
	void unlockMacroInterruptions();
	bool macroMayBeInterrupted();

	bool nukePanic(Move& move, const World& world);

	bool mPanic, mPanicSelection;
	int mPanicTime;
	xypoint mPanicPoint;
	int mLastNuke;
	groupTurnPrototype mInfinityChase;

	bool mDoNotInterruptMacroPlease;
	deque<macroTurnPrototype> mMacroExecutionQueue;
	deque<pair<function<bool(const World&)>, macroTurnPrototype>> mMacroConditionalQueue;

	MyGlobalInfoStorer mGlobaler;

	vector<shared_ptr<MyUnitGroup>> mGroupActors;
	int mCurrActingGroup;
	int mThisGroupActedTimes;

	enum class GameMode
	{
		Round1,
		Round2
	};
	GameMode mGameMode;

	shared_ptr<MyUnitGroup> mSandwichGroup;
};

#endif
