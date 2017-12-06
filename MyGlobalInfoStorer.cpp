#include "MyGlobalInfoStorer.h"
#include "MyUnitGroup.h"

#define sq(x) ((x)*(x))

MyGlobalInfoStorer::MyGlobalInfoStorer()
{
	mCellOccupLand.resize(1024 / 16 + 1, vector<int>(1024 / 16 + 1, 0));
	mCellOccupAir.resize(1024 / 16 + 1, vector<int>(1024 / 16 + 1, 0));
	mCellDangerLand.resize(1024 / 16 + 1, vector<int>(1024 / 16 + 1, 0));
	mCellDangerAir.resize(1024 / 16 + 1, vector<int>(1024 / 16 + 1, 0));
	mNukeValue.resize(1024 / 16 + 1, vector<double>(1024 / 16 + 1));
}

void MyGlobalInfoStorer::processUpdates(const vector<VehicleUpdate>& vu)
{
	mAllyMovedThisTurn.clear();
	for (auto& x : vu)
		if (mOurVehicles.count(x.getId()))
		{
			if (!x.getDurability())
			{
				mOurVehicles.erase(x.getId());
				mSelectedAllies.erase(x.getId());
			}
			else
			{
				if (mOurVehicles[x.getId()].mX != x.getX() || mOurVehicles[x.getId()].mY != x.getY())
					mAllyMovedThisTurn[x.getId()] = true;
				mOurVehicles[x.getId()].mX = x.getX();
				mOurVehicles[x.getId()].mY = x.getY();
				if (x.isSelected())
					mSelectedAllies.insert(x.getId());
				else
					mSelectedAllies.erase(x.getId());
			}
		}
		else
		{
			if (!x.getDurability())
				mEnemyVehicles.erase(x.getId());
			else
			{
				mEnemyVehicles[x.getId()].mX = x.getX();
				mEnemyVehicles[x.getId()].mY = x.getY();
			}
		}
}

void MyGlobalInfoStorer::processNews(const vector<Vehicle>& startVehicleInfo, int myPlayerId)
{
	for (auto& x : startVehicleInfo)
	{
		if (x.getPlayerId() != myPlayerId)
			mEnemyVehicles[x.getId()] = { x.getX(), x.getY(), x.getType() };
		else
			mOurVehicles[x.getId()] = { x.getX(), x.getY(), x.getType() };
	}
}

void MyGlobalInfoStorer::updateFacilities(const vector<Facility>& fs, int currentTick)
{
	for (auto& x : fs)
		if (x.getOwnerPlayerId() == mMyId)
		{
			if (!mOurFacilities.count(x.getId()))
			{
				mOurFacilities[x.getId()] = { x.getLeft(), x.getTop(), x.getType(), currentTick, VehicleType::IFV };
				mNewFacilities.push({ x.getId(), x.getType() });
			}
		}
		else
		{
			mOurFacilities.erase(x.getId());
		}
}

void MyGlobalInfoStorer::setMyId(int id)
{
	mMyId = id;
}

void MyGlobalInfoStorer::buildMaps(vector<shared_ptr<MyUnitGroup>> groups)
{
	for (auto& y : mCellOccupLand)
		for (auto& x : y)
			x = 0;
	for (auto& y : mCellOccupAir)
		for (auto& x : y)
			x = 0;

	for (auto& x : mOurVehicles)
	{
		int cx = x.second.mX / 16;
		int cy = x.second.mY / 16;
		if (x.second.mType == VehicleType::HELICOPTER || x.second.mType == VehicleType::FIGHTER)
			mCellOccupAir[cx][cy] = 1000;
		else
			mCellOccupLand[cx][cy] = 1000;
	}

	mCellDangerLand.assign(1024 / 16 + 1, vector<int>(1024 / 16 + 1, 0));
	mCellDangerAir.assign(1024 / 16 + 1, vector<int>(1024 / 16 + 1, 0));

	for (int y = 0; y < 64; ++y)
		for (int x = 0; x < 64; ++x)
		{
			double nukeScore = 0;
			for (auto& u : getEnemyVehicles())
			{
				double dist = sqrt(sq(x * 16 - u.second.mX) + sq(y * 16 - u.second.mY));
				if (dist <= 50)
				{
					nukeScore += (99 - 99 * (dist / 50));
				}

				//mCellDangerAir
				//mCellDangerLand

				switch (u.second.mType)
				{
				case VehicleType::ARRV:
					if (dist < 32)
					{
						if (mCellDangerAir[x][y] > 0)
							mCellDangerAir[x][y] += 10;
						if (mCellDangerLand[x][y] > 0)
							mCellDangerLand[x][y] += 10;
					}
					break;
				case VehicleType::FIGHTER:
					if (dist < 32)
						mCellDangerAir[x][y] += 100;
					else if (dist < 128)
						mCellDangerAir[x][y] += 100 / 3;
					break;
				case VehicleType::HELICOPTER:
					if (dist < 32)
					{
						mCellDangerAir[x][y] += 80;
						mCellDangerLand[x][y] += 100;
					}
					else if (dist < 256)
					{
						mCellDangerAir[x][y] += 80 / 3;
						mCellDangerLand[x][y] += 100 / 3;
					}
					break;
				case VehicleType::IFV:
					if (dist < 32)
					{
						mCellDangerAir[x][y] += 80;
						mCellDangerLand[x][y] += 90;
					}
					else if (dist < 256)
					{
						mCellDangerAir[x][y] += 80 / 3;
						mCellDangerLand[x][y] += 90 / 3;
					}
					break;
				case VehicleType::TANK:
					if (dist < 32)
					{
						mCellDangerLand[x][y] += 100;
					}
					else if (dist < 256)
					{
						mCellDangerLand[x][y] += 100 / 3;
					}
					break;
				}
			}
			for (auto& u : getOurVehicles())
			{
				double dist = sqrt(sq(x * 16 - u.second.mX) + sq(y * 16 - u.second.mY));
				if (dist <= 50)
				{
					nukeScore -= (99 - 99 * (dist / 50)) * 0.7;
				}
			}
			mNukeValue[x][y] = nukeScore;
		}

	for (auto& q : groups)
	{
		q->removeDestroyed();
		if (q->getGroupIdList().size())
		{
			xypoint mv = q->getMovingVector();
			pair<xypoint, xypoint> aabb = q->getGridedAabb();

			aabb.first.first += mv.first * 2;
			aabb.first.second += mv.second * 2;
			aabb.second.first += mv.first * 2;
			aabb.second.second += mv.second * 2;

			for (int y = aabb.first.second; y <= aabb.second.second; ++y)
				for (int x = aabb.first.first; x <= aabb.second.first; ++x)
				{
					if (x < 0 || x >= 64 || y < 0 || y >= 64)
						continue;
					if (q->getVehicleType() == VehicleType::HELICOPTER || q->getVehicleType() == VehicleType::FIGHTER)
						mCellOccupAir[x][y] = q->getGroupId();
					else
						mCellOccupLand[x][y] = q->getGroupId();
				}
		}
	}
}

double MyGlobalInfoStorer::getNukeValueAtCell(int x, int y) const
{
	return mNukeValue[x][y];
}

int MyGlobalInfoStorer::getCellOccupLand(int x, int y) const
{
	return mCellOccupLand[x][y];
}

int MyGlobalInfoStorer::getCellOccupAir(int x, int y) const
{
	return mCellOccupAir[x][y];
}

int MyGlobalInfoStorer::getCellDangerLand(int x, int y) const
{
	return mCellDangerLand[x][y];
}

int MyGlobalInfoStorer::getCellDangerAir(int x, int y) const
{
	return mCellDangerAir[x][y];
}

map<int, FacilityBasicInfo>& MyGlobalInfoStorer::getOurFacilities()
{
	return mOurFacilities;
}

const map<int, VehicleBasicInfo>& MyGlobalInfoStorer::getOurVehicles() const
{
	return mOurVehicles;
}

const map<int, VehicleBasicInfo>& MyGlobalInfoStorer::getEnemyVehicles() const
{
	return mEnemyVehicles;
}

const set<int>& MyGlobalInfoStorer::getSelectedAllies() const
{
	return mSelectedAllies;
}

pair<int, FacilityType> MyGlobalInfoStorer::getNewFacility()
{
	auto cp = make_pair(-1, FacilityType::_UNKNOWN_);
	if (mNewFacilities.size())
	{
		cp = mNewFacilities.front();
		mNewFacilities.pop();
	}
	return cp;
}


bool MyGlobalInfoStorer::allyMoved(int id) const
{
	if (!mAllyMovedThisTurn.count(id))
		return false;
	return mAllyMovedThisTurn.at(id);
}

bool MyGlobalInfoStorer::isAlly(int id) const
{
	return mOurVehicles.count(id);
}

const VehicleBasicInfo& MyGlobalInfoStorer::getUnitInfo(int id) const
{
	if (isAlly(id))
		return mOurVehicles.at(id);
	return mEnemyVehicles.at(id);
}

bool MyGlobalInfoStorer::anyAllyMoved() const
{
	return mAllyMovedThisTurn.size();
}

int MyGlobalInfoStorer::getMyId() const
{
	return mMyId;
}
