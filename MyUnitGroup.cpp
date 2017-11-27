#include "MyUnitGroup.h"

int MyUnitGroup::sCurrentlySelectedGroup = -1;
int MyUnitGroup::sGroupsCount = 0; // max == 100

bool MyUnitGroup::act(Move& move, const World& world)
{
	for (auto& x : mIngroupIds)
		if (!mGlobaler.getOurVehicles().count(x))
			mIngroupIds.erase(x);

	if (!mIngroupIds.size())
		return false;

	if (!mCurrentExecutionQueue.size() && mConditionalQueue.size())
	{
		auto fitem = mConditionalQueue.front();
		bool conditionOk = false;
		switch (fitem.first)
		{
		case CondQueueCondition::NoCondition:
		{
			conditionOk = true;
			break;
		}
		case CondQueueCondition::AllUnitStopped:
		{
			conditionOk = !moving();
			break;
		}
		}
		if (conditionOk)
		{
			mCurrentExecutionQueue.push_back(fitem.second);
			mConditionalQueue.pop_front();
		}
	}

	if (sCurrentlySelectedGroup != mGroupNumber)
	{
		move.setAction(ActionType::CLEAR_AND_SELECT);
		move.setGroup(mGroupNumber);
		return true;
	}

	if (mCurrentExecutionQueue.size())
	{
		mCurrentExecutionQueue.front()(move, world);
		mCurrentExecutionQueue.pop_front();
		return true;
	}

	return false;
}

void MyUnitGroup::pushToConditionalQueue(pair<CondQueueCondition, function<void(Move&, const World&)>> func)
{
	mConditionalQueue.push_back(func);
}

bool MyUnitGroup::moving() const
{
	bool allStopped = true;
	for (auto& x : mIngroupIds)
		if (mGlobaler.allyMoved(x))
		{
			allStopped = false;
			break;
		}
	return !allStopped;
}

MyUnitGroup::MyUnitGroup(Move& move, const World& world, const MyGlobalInfoStorer& globaler) // will assign (takes 1 turn)
	: mGlobaler(globaler)
{
	mGroupNumber = ++sGroupsCount;

	for (auto& x : mGlobaler.getSelectedAllies())
		mIngroupIds.insert(x);

	move.setAction(ActionType::ASSIGN);
	move.setGroup(mGroupNumber);
}
