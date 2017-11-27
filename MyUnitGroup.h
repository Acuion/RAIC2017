#pragma once

#define VALFHDR [=](Move& move, const World& world)
#include <functional>
#include "model/World.h"
#include "model/Move.h"
#include <queue>
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

	void move(dxypoint vector, bool saveFormation);
	void scale(dxypoint point, double factor);
	void rotate(dxypoint point, double factor);
	void forcedSelect(Move& move);

	bool moving() const;
	xypoint getCenterOfGroup() const;

	MyUnitGroup(Move& move, const World& world, const MyGlobalInfoStorer& globaler);
private:
	static int sCurrentlySelectedGroup, sGroupsCount;
	
	double getMaxSpeedOnVector(dxypoint vector, const World& world);

	int mGroupNumber;

	deque<pair<CondQueueCondition, function<void(Move&, const World&)>>> mConditionalQueue;
	deque<function<void(Move&, const World&)>> mCurrentExecutionQueue;

	set<int> mIngroupIds;

	const MyGlobalInfoStorer& mGlobaler;
};

