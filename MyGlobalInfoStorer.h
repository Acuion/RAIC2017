#pragma once
#include <set>
#include "model/VehicleType.h"
#include "model/VehicleUpdate.h"
#include "model/Vehicle.h"
#include <map>

using namespace std;
using namespace model;

struct VehicleBasicInfo
{
	double mX, mY;
	VehicleType mType;
};

class MyGlobalInfoStorer
{
public:
	void processUpdates(const vector<VehicleUpdate>& vu);
	void processNews(const vector<Vehicle>& startVehicleInfo, int myPlayerId);

	const map<int, VehicleBasicInfo>& getOurVehicles() const;
	const map<int, VehicleBasicInfo>& getEnemyVehicles() const;
	const set<int>& getSelectedAllies() const;
	bool allyMoved(int id) const;
	bool isAlly(int id) const;
	const VehicleBasicInfo& getUnitInfo(int id) const;
	bool anyAllyMoved() const;
private:
	set<int> mSelectedAllies;
	map<int, bool> mAllyMovedThisTurn;
	map<int, VehicleBasicInfo> mOurVehicles, mEnemyVehicles;
};

