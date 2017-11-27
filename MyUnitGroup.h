#pragma once

#define BYVALMOVEFUNCHEADER [=](Move& move, const World& world)
#include <functional>
#include "model/World.h"
#include "model/Move.h"
#include <queue>
#include "MyGlobalInfoStorer.h"

using namespace std;
using namespace model;

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

	bool moving() const;

	MyUnitGroup(Move& move, const World& world, const MyGlobalInfoStorer& globaler);
private:
	static int sCurrentlySelectedGroup, sGroupsCount;
	
	int mGroupNumber;

	deque<pair<CondQueueCondition, function<void(Move&, const World&)>>> mConditionalQueue;
	deque<function<void(Move&, const World&)>> mCurrentExecutionQueue;

	set<int> mIngroupIds;

	const MyGlobalInfoStorer& mGlobaler;
};

