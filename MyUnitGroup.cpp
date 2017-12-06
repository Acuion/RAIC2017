#include "MyUnitGroup.h"
#include "MyStrategy.h"

int MyUnitGroup::sCurrentlySelectedGroup = -1;
int MyUnitGroup::sGroupsCount = 0; // max == 100

bool MyUnitGroup::act(Move& move, const World& world)
{
	++mGroupActs;

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

void MyUnitGroup::removeDestroyed()
{
	for (auto it = mIngroupIds.begin(); it != mIngroupIds.end();)
		if (!mGlobaler.getOurVehicles().count(*it))
			it = mIngroupIds.erase(it);
		else
			++it;
}

void MyUnitGroup::buildMoveMap()
{
	auto aabb = getGridedAabb();
	auto ltcorner = aabb.first;
	auto rbcorner = aabb.second;

	int width = rbcorner.first - ltcorner.first;
	int height = rbcorner.second - ltcorner.second;

	queue<xypoint> q;
	mMoveParent.assign(1024 / 16, vector<xypoint>(1024 / 16, { -1, -1 }));
	mMoveParent[ltcorner.first][ltcorner.second] = ltcorner;
	q.push(ltcorner);

	const int dx[] = { 0,0,1,-1,1,1,-1,-1 };
	const int dy[] = { 1,-1,0,0,1,-1,1,-1 };

	while (!q.empty())
	{
		const auto t = q.front();
		q.pop();

		for (int k = 0; k < 8; ++k)
		{
			xypoint nxt = { t.first + dx[k], t.second + dy[k] };
			if (pointIsInBounds(nxt, width, height))
			{
				if (mMoveParent[nxt.first][nxt.second].first != -1)
					continue;
				if (!groupAtPointDoesntIntersect(nxt, width, height, ltcorner, rbcorner))
					continue;
				q.push(nxt);
				mMoveParent[nxt.first][nxt.second] = t;
			}
		}
	}
}

bool MyUnitGroup::smartMoveTo(dxypoint point, Move& move, const World& world)
{
	auto aabb = getGridedAabb();
	auto ltcorner = aabb.first;
	auto rbcorner = aabb.second;

	int width = rbcorner.first - ltcorner.first;
	int height = rbcorner.second - ltcorner.second;

	point.first = point.first / 16;
	point.second = point.second / 16;

	if (mMoveParent[point.first][point.second].first != -1)
	{
		xypoint lastDiff;
		int countmv = 0;
		while (mMoveParent[point.first][point.second] != ltcorner)
		{
			point = mMoveParent[point.first][point.second];
		}
		int i = 0;
		xypoint moveTo = { ltcorner.first, ltcorner.second };
		xypoint vector = { point.first - ltcorner.first, point.second - ltcorner.second };
		if (vector.first != 0 || vector.second != 0)
			for (i = 0; pointIsInBounds(moveTo, width, height) && groupAtPointDoesntIntersect(moveTo, width, height, ltcorner, rbcorner); ++i)
			{
				moveTo.first += vector.first;
				moveTo.second += vector.second;
			}
		mMovingVector = vector;
		this->move({ vector.first * i * 16, vector.second * i * 16 }, true, move, world);
		return true;
	}

	return false;
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

void MyUnitGroup::setGroupAngle(double angle)
{
	mGroupAngle = angle;
}

void MyUnitGroup::setVehicleType(VehicleType type)
{
	mVehicleType = type;
}

pair<xypoint, xypoint> MyUnitGroup::getGridedAabb() const
{
	xypoint ltcorner = { 2000, 2000 }, rbcorner = { -1, -1 };
	for (auto& x : mIngroupIds)
	{
		const auto& ui = mGlobaler.getUnitInfo(x);
		ltcorner.first = min(ltcorner.first, (int)ui.mX);
		rbcorner.first = max(rbcorner.first, (int)ui.mX);
		ltcorner.second = min(ltcorner.second, (int)ui.mY);
		rbcorner.second = max(rbcorner.second, (int)ui.mY);
	}
	ltcorner.first = ltcorner.first / 16;
	ltcorner.second = ltcorner.second / 16;
	rbcorner.first = rbcorner.first / 16;
	rbcorner.second = rbcorner.second / 16;

	return { ltcorner, rbcorner };
}

xypoint MyUnitGroup::getMovingVector() const
{
	return mMovingVector;
}

VehicleType MyUnitGroup::getVehicleType() const
{
	return mVehicleType;
}

double MyUnitGroup::getGroupAngle()
{
	return mGroupAngle;
}

int MyUnitGroup::getGroupActsCount() const
{
	return mGroupActs;
}

int MyUnitGroup::getGroupId() const
{
	return mGroupNumber;
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

void MyUnitGroup::dropSelection()
{
	sCurrentlySelectedGroup = -1;
}

MyUnitGroup::MyUnitGroup(Move& move, const World& world, const MyGlobalInfoStorer& globaler, double groupAngle) // will assign (takes 1 turn)
	: mDoNotInterruptPlease(false)
	, mGlobaler(globaler)
	, mGroupActs(0)
	, mGroupAngle(groupAngle)
{
	mMoveParent.resize(1024 / 16, vector<xypoint>(1024 / 16));

	mGroupNumber = ++sGroupsCount;

	for (auto& x : mGlobaler.getSelectedAllies())
		mIngroupIds.insert(x);

	move.setAction(ActionType::ASSIGN);
	move.setGroup(mGroupNumber);
}

bool MyUnitGroup::pointIsInBounds(xypoint point, int width, int height)
{
	return point.first >= 0 && point.second >= 0 && point.first < 64 - width && point.second < 64 - height;
}

bool MyUnitGroup::groupAtPointDoesntIntersect(xypoint point, int width, int height, xypoint ltcorner, xypoint rbcorner)
{
	const auto pointIsEmpty = [&](int x, int y)
	{
		int co;
		if (getVehicleType() == VehicleType::HELICOPTER || getVehicleType() == VehicleType::FIGHTER)
			co = mGlobaler.getCellOccupAir(x, y);
		else
			co = mGlobaler.getCellOccupLand(x, y);
		if (0 == co || getGroupId() == co)
			return true;
		if (ltcorner.first <= x && ltcorner.second <= y && rbcorner.first >= x && rbcorner.second >= y)
			return true;
		return false;
	};

	bool bad = false;
	for (int i = 0; i < width; ++i)
	{
		if (!pointIsEmpty(point.first + i, point.second) || !pointIsEmpty(point.first + i, point.second + height))
		{
			bad = true;
			break;
		}
	}
	for (int i = 0; i < height; ++i)
	{
		if (!pointIsEmpty(point.first, point.second + i) || !pointIsEmpty(point.first + width, point.second + i))
		{
			bad = true;
			break;
		}
	}

	return !bad;
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
