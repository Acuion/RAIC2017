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
	void pushToConditionalQueue(CondQueueCondition cnd, function<void(Move&, const World&)> func, bool recursive = false);

	void lockInterrupts();
	void unlockInterrupts();
	bool mayBeInterrupted();

	void move(dxypoint vector, bool saveFormation, Move& move, const World& world);
	void scale(dxypoint point, double factor, Move& move, const World& world);
	void forcedSelect(Move& move);
	void appendGroup(shared_ptr<MyUnitGroup> group, Move& move);
	void setTag(const string& tag);

	const string& getTag() const;
	const set<int>& getGroupIdList() const;
	bool moving() const;
	xypoint getCenterOfGroup() const;
	xypoint getClosestEnemy() const;
	double getGroupRadius() const;

	MyUnitGroup(Move& move, const World& world, const MyGlobalInfoStorer& globaler);
private:
	struct ConditionalQueueItem
	{
		CondQueueCondition mCond;
		function<void(Move&, const World&)> mFunc;
		bool mRecursive;
	};

	static int sCurrentlySelectedGroup, sGroupsCount;
	
	double getMaxSpeedOnVector(dxypoint vector, const World& world);

	int mGroupNumber;

	bool mDoNotInterruptPlease;
	deque<ConditionalQueueItem> mConditionalQueue;
	deque<function<void(Move&, const World&)>> mCurrentExecutionQueue;

	set<int> mIngroupIds;

	const MyGlobalInfoStorer& mGlobaler;

	string mTag;
};

