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

enum class CondQueueCondition
{
	AllUnitStopped,
	NoCondition
};

class MyUnitGroup
{
public:
	bool act(Move& move, const World& world);
	void pushToConditionalQueue(CondQueueCondition cnd, function<void(Move&, const World&, MyUnitGroup&)> func, bool recursive = false);

	void lockInterrupts();
	void unlockInterrupts();
	bool mayBeInterrupted();

	void removeDestroyed();
	void buildMoveMap();
	bool smartMoveTo(dxypoint point, Move& move, const World& world);
	void move(dxypoint vector, bool saveFormation, Move& move, const World& world);
	void scale(dxypoint point, double factor, Move& move, const World& world);
	void forcedSelect(Move& move);
	void setTag(const string& tag);
	void setGroupAngle(double angle);
	void setVehicleType(VehicleType type);

	pair<xypoint, xypoint> getGridedAabb() const;
	xypoint getMovingVector() const;
	VehicleType getVehicleType() const;
	double getGroupAngle();
	int getGroupActsCount() const;
	int getGroupId() const;
	const string& getTag() const;
	const set<int>& getGroupIdList() const;
	bool moving() const;
	xypoint getCenterOfGroup() const;
	xypoint getClosestEnemy() const;
	double getGroupRadius() const;

	static void dropSelection();

	MyUnitGroup(Move& move, const World& world, const MyGlobalInfoStorer& globaler, double groupAngle);
private:
	struct ConditionalQueueItem
	{
		CondQueueCondition mCond;
		function<void(Move&, const World&, MyUnitGroup&)> mFunc;
		bool mRecursive;
	};
	
	bool pointIsInBounds(xypoint point, int width, int height);
	bool groupAtPointDoesntIntersect(xypoint point, int width, int height, xypoint ltcorner, xypoint rbcorner);

	static int sCurrentlySelectedGroup, sGroupsCount;

	double getMaxSpeedOnVector(dxypoint vector, const World& world);

	int mGroupNumber;

	bool mDoNotInterruptPlease;
	deque<ConditionalQueueItem> mConditionalQueue;
	deque<function<void(Move&, const World&, MyUnitGroup&)>> mCurrentExecutionQueue;

	set<int> mIngroupIds;

	const MyGlobalInfoStorer& mGlobaler;

	string mTag;
	VehicleType mVehicleType;

	int mGroupActs;

	double mGroupAngle;

	xypoint mMovingVector;
	vector<vector<xypoint>> mMoveParent;
};