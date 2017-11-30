#include "MyUnitGroup.h"

int MyUnitGroup::sCurrentlySelectedGroup = -1;
int MyUnitGroup::sGroupsCount = 0; // max == 100

bool MyUnitGroup::act(Move& move, const World& world)
{
	for (auto it = mIngroupIds.begin(); it != mIngroupIds.end();)
		if (!mGlobaler.getOurVehicles().count(*it))
			it = mIngroupIds.erase(it);
		else
			++it;

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

	if (!mCurrentExecutionQueue.size())
		return false;

	if (sCurrentlySelectedGroup != mGroupNumber)
	{
		forcedSelect(move);
		return true;
	}

	mCurrentExecutionQueue.front()(move, world);
	mCurrentExecutionQueue.pop_front();
	return true;
}

void MyUnitGroup::pushToConditionalQueue(pair<CondQueueCondition, function<void(Move&, const World&)>> func)
{
	mConditionalQueue.push_back(func);
}

void MyUnitGroup::lockInterrupts()
{
	mDoNotInterruptPlease = true;
}

void MyUnitGroup::unlockInterrupts()
{
	mDoNotInterruptPlease = false;
}

bool MyUnitGroup::mayBeInterrupted()
{
	return !mDoNotInterruptPlease;
}

void MyUnitGroup::move(dxypoint vector, bool saveFormation)
{
	if (saveFormation)
	{
		mConditionalQueue.push_back({ CondQueueCondition::NoCondition, VALFHDR
		{
			move.setAction(ActionType::MOVE);
			move.setX(vector.first);
			move.setY(vector.second);
			move.setMaxSpeed(getMaxSpeedOnVector(vector, world));
		} });
	}
	else
	{
		mConditionalQueue.push_back({ CondQueueCondition::NoCondition, VALFHDR
		{
			move.setAction(ActionType::MOVE);
			move.setX(vector.first);
			move.setY(vector.second);
		} });
	}
}

void MyUnitGroup::scale(dxypoint point, double factor)
{
	mConditionalQueue.push_back({ CondQueueCondition::NoCondition, VALFHDR
	{
		move.setAction(ActionType::SCALE);
		move.setX(point.first);
		move.setY(point.second);
		move.setFactor(factor);
	} });
}

void MyUnitGroup::rotate(dxypoint point, double angle)
{
	mConditionalQueue.push_back({ CondQueueCondition::NoCondition, VALFHDR
	{
		move.setAction(ActionType::ROTATE);
		move.setX(point.first);
		move.setY(point.second);
		move.setAngle(angle);
	} });
}

void MyUnitGroup::forcedSelect(Move& move)
{
	move.setAction(ActionType::CLEAR_AND_SELECT);
	move.setGroup(mGroupNumber);
	sCurrentlySelectedGroup = mGroupNumber;
}

void MyUnitGroup::appendGroup(shared_ptr<MyUnitGroup> group, Move& move)
{
	mConditionalQueue.push_back({ CondQueueCondition::NoCondition, VALFHDR
	{
		lockInterrupts();
		group->forcedSelect(move);
		mConditionalQueue.push_back({ CondQueueCondition::NoCondition, VALFHDR
		{
			for (auto& x : mGlobaler.getSelectedAllies())
				mIngroupIds.insert(x);
			move.setAction(ActionType::ASSIGN);
			move.setGroup(mGroupNumber);
			unlockInterrupts();
		}
		});
	}
	});
}

const set<int>& MyUnitGroup::getGroupIdList()
{
	return mIngroupIds;
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

xypoint MyUnitGroup::getCenterOfGroup() const
{
	xypoint center = { 0,0 };
	for (auto& x : mIngroupIds)
	{
		center.first += mGlobaler.getUnitInfo(x).mX;
		center.second += mGlobaler.getUnitInfo(x).mY;
	}
	center.first /= mIngroupIds.size();
	center.second /= mIngroupIds.size();
	return center;
}

MyUnitGroup::MyUnitGroup(Move& move, const World& world, const MyGlobalInfoStorer& globaler) // will assign (takes 1 turn)
	: mGlobaler(globaler)
	, mDoNotInterruptPlease(false)
{
	mGroupNumber = ++sGroupsCount;

	for (auto& x : mGlobaler.getSelectedAllies())
		mIngroupIds.insert(x);

	move.setAction(ActionType::ASSIGN);
	move.setGroup(mGroupNumber);
}

double MyUnitGroup::getMaxSpeedOnVector(dxypoint vector, const World& world)
{
	return 0.18; // todo
}
