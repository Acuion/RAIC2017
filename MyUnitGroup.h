#pragma once

#define VALFHDR [=](Move& move, const World& world)
#include <functional>
#include "model/World.h"
#include "model/Move.h"
#include <queue>
#include <memory>
#include "MyGlobalInfoStorer.h"

using namespace std;
using namespace model;
using xypoint = pair<int, int>;
using dxypoint = pair<double, double>;

enum class CondQueueCondition
{
	AllUnitStopped,
	NoCondition
};

class MyUnitGroup
{
public:
	bool act(Move& move, const World& world);
	void pushToConditionalQueue(pair<CondQueueCondition, function<void(Move&, const World&)>> func);

	void lockInterrupts();
	void unlockInterrupts();
	bool mayBeInterrupted();

	void move(dxypoint vector, bool saveFormation);
	void scale(dxypoint point, double factor);
	void rotate(dxypoint point, double factor);
	void forcedSelect(Move& move);
	void appendGroup(shared_ptr<MyUnitGroup> group, Move& move);

	const set<int>& getGroupIdList();
	bool moving() const;
	xypoint getCenterOfGroup() const;

	MyUnitGroup(Move& move, const World& world, const MyGlobalInfoStorer& globaler);
private:
	static int sCurrentlySelectedGroup, sGroupsCount;
	
	double getMaxSpeedOnVector(dxypoint vector, const World& world);

	int mGroupNumber;

	bool mDoNotInterruptPlease;
	deque<pair<CondQueueCondition, function<void(Move&, const World&)>>> mConditionalQueue;
	deque<function<void(Move&, const World&)>> mCurrentExecutionQueue;

	set<int> mIngroupIds;

	const MyGlobalInfoStorer& mGlobaler;
};

