#include "MyUnitGroup.h"
#include "MyStrategy.h"

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
		switch (fitem.mCond)
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
			mCurrentExecutionQueue.push_back(fitem.mFunc);
			if (fitem.mRecursive)
				mConditionalQueue.push_back(mConditionalQueue.front());
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

	mCurrentExecutionQueue.front()(move, world, *this);
	mCurrentExecutionQueue.pop_front();
	return true;
}

void MyUnitGroup::pushToConditionalQueue(CondQueueCondition cnd, function<void(Move&, const World&, MyUnitGroup&)> func, bool recursive)
{
	mConditionalQueue.push_back({ cnd, func, recursive });
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

void MyUnitGroup::move(dxypoint vector, bool saveFormation, Move& move, const World& world)
{
	macroTurnPrototype turn;
	if (saveFormation)
	{
		turn = VALFHDR
		{
			move.setAction(ActionType::MOVE);
		move.setX(vector.first);
		move.setY(vector.second);
		move.setMaxSpeed(getMaxSpeedOnVector(vector, world));
		};
	}
	else
	{
		turn = VALFHDR
		{
			move.setAction(ActionType::MOVE);
		move.setX(vector.first);
		move.setY(vector.second);
		};
	}
	turn(move, world);
}

void MyUnitGroup::scale(dxypoint point, double factor, Move& move, const World& world)
{
	move.setAction(ActionType::SCALE);
	move.setX(point.first);
	move.setY(point.second);
	move.setFactor(factor);
}

void MyUnitGroup::forcedSelect(Move& move)
{
	move.setAction(ActionType::CLEAR_AND_SELECT);
	move.setGroup(mGroupNumber);
	sCurrentlySelectedGroup = mGroupNumber;
}

void MyUnitGroup::setTag(const string& tag)
{
	mTag = tag;
}

const string& MyUnitGroup::getTag() const
{
	return mTag;
}

const set<int>& MyUnitGroup::getGroupIdList() const
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

xypoint MyUnitGroup::getClosestEnemy() const
{
	const auto center = getCenterOfGroup();

	double mindst = 1e9;
	xypoint ans = { -1, -1 };

	for (auto& x : mGlobaler.getEnemyVehicles())
	{
		double currdst = hypot(center.first - x.second.mX, center.second - x.second.mY);
		if (currdst < mindst)
		{
			mindst = currdst;
			ans.first = x.second.mX;
			ans.second = x.second.mY;
		}
	}

	return ans;
}

double MyUnitGroup::getGroupRadius() const
{
	const auto center = getCenterOfGroup();
	double mxd = -1;
	for (auto& x : mIngroupIds)
	{
		const auto& unit = mGlobaler.getUnitInfo(x);
		mxd = max(mxd, hypot(unit.mX - center.first, unit.mY - center.second));
	}
	return mxd;
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
	double minspeed = 1e9;
	for (auto& x : mIngroupIds)
	{
		double cs = 1e9;
		switch (mGlobaler.getUnitInfo(x).mType)
		{
		case VehicleType::ARRV:
		case VehicleType::IFV:
			cs = 0.4;
			break;
		case VehicleType::FIGHTER:
			cs = 1.2;
			break;
		case VehicleType::HELICOPTER:
			cs = 0.9;
			break;
		case VehicleType::TANK:
			cs = 0.3;
			break;
		}
		minspeed = min(cs, minspeed);
		if (cs == 0.3)
			break;
	}
	return minspeed * 0.6;
}