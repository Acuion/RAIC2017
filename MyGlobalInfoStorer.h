#pragma once
#include <set>
#include "model/VehicleType.h"
#include "model/VehicleUpdate.h"
#include "model/Vehicle.h"
#include <map>
#include "model/Facility.h"
#include <queue>
#include <memory>

using namespace std;
using namespace model;
using xypoint = pair<int, int>;
using dxypoint = pair<double, double>;

struct VehicleBasicInfo
{
	double mX, mY;
	VehicleType mType;
};

struct FacilityBasicInfo
{
	double mX, mY;
	FacilityType mType;
	int mCapturedAt;
	VehicleType mCurrentlyConstructing;
};

class MyUnitGroup;

class MyGlobalInfoStorer
{
public:
	MyGlobalInfoStorer();

	void processUpdates(const vector<VehicleUpdate>& vu);
	void processNews(const vector<Vehicle>& startVehicleInfo, int myPlayerId);
	void updateFacilities(const vector<Facility>& fs, int currentTick);
	void setMyId(int id);
	void buildObstacleMap(vector<shared_ptr<MyUnitGroup>> groups);

	int getCellOccup(int x, int y) const;
	map<int, FacilityBasicInfo>& getOurFacilities();
	const map<int, VehicleBasicInfo>& getOurVehicles() const;
	const map<int, VehicleBasicInfo>& getEnemyVehicles() const;
	const set<int>& getSelectedAllies() const;
	pair<int, FacilityType> getNewFacility();
	bool allyMoved(int id) const;
	bool isAlly(int id) const;
	const VehicleBasicInfo& getUnitInfo(int id) const;
	bool anyAllyMoved() const;
	int getMyId() const;
private:
	int mMyId;
	set<int> mSelectedAllies;
	queue<pair<int, FacilityType>> mNewFacilities;
	map<int, bool> mAllyMovedThisTurn;
	map<int, VehicleBasicInfo> mOurVehicles, mEnemyVehicles;
	map<int, FacilityBasicInfo> mOurFacilities;
	vector<vector<int>> mCellOccup;
};
