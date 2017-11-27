#include "MyGlobalInfoStorer.h"

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

bool MyGlobalInfoStorer::allyMoved(int id) const
{
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
