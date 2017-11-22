#include "MyStrategy.h"
#include "MyFormationBruteforcer.h"

#define PI 3.14159265358979323846
#define _USE_MATH_DEFINES

void MyStrategy::selectVehicles(VehicleType vt, Move& mv)
{
	mv.setAction(ActionType::CLEAR_AND_SELECT);
	mv.setRight(1024);
	mv.setBottom(1024);
	mv.setVehicleType(vt);
}

xypoint MyStrategy::getCenterOfGroup(VehicleType vt)
{
	double x = 0, y = 0, count = 0;
	for (auto& q : mOurVehicles)
		if (q.second.mType == vt)
		{
			x += q.second.mX;
			y += q.second.mY;
			count++;
		}

	return {x / count, y / count};
}

void MyStrategy::firstTickActions(const Player& me, const World& world, const Game& game, Move& move)
{
	for (auto& x : world.getNewVehicles())
	{
		if (x.getPlayerId() != me.getId())
			mEnemyVehicles[x.getId()] = { x.getX(), x.getY(), x.getType() };
		else
			mOurVehicles[x.getId()] = { x.getX(), x.getY(), x.getType() };
	}

	const xypoint tankCenter = getCenterOfGroup(VehicleType::TANK);
	const xypoint helicopter_center = getCenterOfGroup(VehicleType::HELICOPTER);
	const xypoint ifvCenter = getCenterOfGroup(VehicleType::IFV);
	const xypoint fighterCenter = getCenterOfGroup(VehicleType::FIGHTER);
	const xypoint arrvCenter = getCenterOfGroup(VehicleType::ARRV);

	//cout << tankCenter.first << " " << tankCenter.second << endl;
	//cout << helicopterCenter.first << " " << helicopterCenter.second << endl;
	//cout << ifvCenter.first << " " << ifvCenter.second << endl;
	//cout << fighterCenter.first << " " << fighterCenter.second << endl;
	//cout << arrvCenter.first << " " << arrvCenter.second << endl;

	xypoint tankCell, helicopterCell, ifvCell, fighterCell, arrvCell;

	tankCell.first = round((tankCenter.first - 45) / 75.0);
	tankCell.second = round((tankCenter.second - 45) / 75.0);
	helicopterCell.first = round((helicopter_center.first - 45) / 75.0);
	helicopterCell.second = round((helicopter_center.second - 45) / 75.0);
	ifvCell.first = round((ifvCenter.first - 45) / 75.0);
	ifvCell.second = round((ifvCenter.second - 45) / 75.0);
	fighterCell.first = round((fighterCenter.first - 45) / 75.0);
	fighterCell.second = round((fighterCenter.second - 45) / 75.0);
	arrvCell.first = round((arrvCenter.first - 45) / 75.0);
	arrvCell.second = round((arrvCenter.second - 45) / 75.0);

	auto mfb = MyFormationBruteforcer(tankCell, ifvCell, arrvCell);
	mfb.buildPathToFormation();

	int nextTurnAt = 0;
	auto path = mfb.getFormationPath();
	if (path.size())
	{
		vector<FormationStep> nowRunning;

		const auto intersectionHandler = [&]
		{
			bool tankMove = false;
			for (auto& x : nowRunning)
				if (x.mVt == model::VehicleType::TANK)
					tankMove = true;
			if (tankMove)
				nextTurnAt += 75 / 0.3 + 60;
			else
				nextTurnAt += 75 / 0.4 + 60;
			nowRunning.clear();
		};

		for (int i = 0; i < path.size(); ++i)
		{
			auto& turn = path[i];

			bool intersects = false;

			for (auto& x : nowRunning)
				if (x.mMoveFrom == turn.mMoveFrom || x.mMoveFrom == turn.mMoveTo || x.mMoveTo == turn.mMoveTo || x.mMoveTo == turn.mMoveFrom)
					intersects = true;

			if (intersects)
			{
				intersectionHandler();
			}

			double moveAngle = atan2(turn.mMoveTo.second - turn.mMoveFrom.second, turn.mMoveTo.first - turn.mMoveFrom.first);
			mDelayedFunctions.push({ nextTurnAt, [=](Move& move, const World& world)
			{
				selectVehicles(turn.mVt, move);
				mExecutionQueue.push_front([=](Move& move, const World& world)
				{
					move.setAction(ActionType::MOVE);
					move.setX(cos(moveAngle) * 75);
					move.setY(sin(moveAngle) * 75);
					if (fabs(move.getX()) < 1e-6)
						move.setX(0);
					if (fabs(move.getY()) < 1e-6)
						move.setY(0);
				});
				swap(mExecutionQueue[0], mExecutionQueue[1]);
			} });

			nextTurnAt += 5 + 5; // selection & move commands overhead
			nowRunning.push_back(turn);
		}
		intersectionHandler();
	}

	// 2.0 - radius
	// dist btw units = 3
	const bool horisontalFormation = mfb.getFormation()[0].second == mfb.getFormation()[1].second;
	const int formationIndex = (horisontalFormation ? mfb.getFormation()[0].second : mfb.getFormation()[0].first);

	mDelayedFunctions.push({nextTurnAt, [=](Move& move, const World& world)
	{
		const xypoint tankCenter = getCenterOfGroup(VehicleType::TANK);
		const xypoint ifvCenter = getCenterOfGroup(VehicleType::IFV);
		const xypoint arrvCenter = getCenterOfGroup(VehicleType::ARRV);

		vector<double> targetShifts;

		const auto selectHorisontallyJthRow = [=](Move& move, xypoint center, int j, VehicleType type)
		{
			move.setAction(ActionType::CLEAR_AND_SELECT);
			move.setVehicleType(type);
			move.setTop(center.second + (j - 5) * 6);
			move.setBottom(center.second + (j - 4) * 6);
			move.setLeft(center.first - (1 + 5 * 6));
			move.setRight(center.first + (1 + 5 * 6));
		};

		const auto selectVerticallyJthRow = [=](Move& move, xypoint center, int j, VehicleType type)
		{
			move.setAction(ActionType::CLEAR_AND_SELECT);
			move.setVehicleType(type);
			move.setLeft(center.first + (j - 5) * 6);
			move.setRight(center.first + (j - 4) * 6);
			move.setTop(center.second - (1 + 5 * 6));
			move.setBottom(center.second + (1 + 5 * 6));
		};

		const auto enqueueToFrontMoveVertOnJthShiftWithShift = [=](vector<double> targetShifts, int j, int shift)
		{
			mExecutionQueue.push_front([=](Move& move, const World& world)
			{
				move.setAction(ActionType::MOVE);
				move.setY(targetShifts[j] + shift);
			});
			std::swap(mExecutionQueue[0], mExecutionQueue[1]);
		};

		const auto enqueueToFrontMoveHorisOnJthShiftWithShift = [=](vector<double> targetShifts, int j, int shift)
		{
			mExecutionQueue.push_front([=](Move& move, const World& world)
			{
				move.setAction(ActionType::MOVE);
				move.setX(targetShifts[j] + shift);
			});
			std::swap(mExecutionQueue[0], mExecutionQueue[1]);
		};

		const auto processMovesWithShiftsVert = [=](vector<double> targetShifts, int j, int tankShift, int ifvShift, int arrvShift)
		{
			mExecutionQueue.push_back([=](Move& move, const World& world)
			{
				selectHorisontallyJthRow(move, tankCenter, j, VehicleType::TANK);
				enqueueToFrontMoveVertOnJthShiftWithShift(targetShifts, j, tankShift);
			});
			mExecutionQueue.push_back([=](Move& move, const World& world)
			{
				selectHorisontallyJthRow(move, ifvCenter, j, VehicleType::IFV);
				enqueueToFrontMoveVertOnJthShiftWithShift(targetShifts, j, ifvShift);
			});
			mExecutionQueue.push_back([=](Move& move, const World& world)
			{
				selectHorisontallyJthRow(move, arrvCenter, j, VehicleType::ARRV);
				enqueueToFrontMoveVertOnJthShiftWithShift(targetShifts, j, arrvShift);
			});
		};

		const auto processMovesWithShiftsHoris = [=](vector<double> targetShifts, int j, int tankShift, int ifvShift, int arrvShift)
		{
			mExecutionQueue.push_back([=](Move& move, const World& world)
			{
				selectVerticallyJthRow(move, tankCenter, j, VehicleType::TANK);
				enqueueToFrontMoveHorisOnJthShiftWithShift(targetShifts, j, tankShift);
			});
			mExecutionQueue.push_back([=](Move& move, const World& world)
			{
				selectVerticallyJthRow(move, ifvCenter, j, VehicleType::IFV);
				enqueueToFrontMoveHorisOnJthShiftWithShift(targetShifts, j, ifvShift);
			});
			mExecutionQueue.push_back([=](Move& move, const World& world)
			{
				selectVerticallyJthRow(move, arrvCenter, j, VehicleType::ARRV);
				enqueueToFrontMoveHorisOnJthShiftWithShift(targetShifts, j, arrvShift);
			});
		};

		if (horisontalFormation) // * * *
		{
			switch (formationIndex)
			{
			case 0:
				for (int i = 0; i < 10; ++i)
					targetShifts.push_back(12 * i);
				for (int j = 9; j >= 0; --j)
				{
					processMovesWithShiftsVert(targetShifts, j, 0, 6, 12);
				}
				break;
			case 1:
				for (int i = 0; i < 5; ++i)
					targetShifts.push_back(-6 - 12 * (4 - i));
				for (int i = 0; i < 5; ++i)
					targetShifts.push_back(6 + 12 * i);
				for (int j = 0; j < 5; ++j)
				{
					processMovesWithShiftsVert(targetShifts, j, 0, -6, -12);
				}
				for (int j = 9; j >= 5; --j)
				{
					processMovesWithShiftsVert(targetShifts, j, 0, 6, 12);
				}
				break;
			case 2:
				for (int i = 0; i < 10; ++i)
					targetShifts.push_back(-12 * i);
				std::reverse(targetShifts.begin(), targetShifts.end());
				for (int j = 0; j <= 9; ++j)
				{
					processMovesWithShiftsVert(targetShifts, j, 0, -6, -12);
				}
				break;
			default:
				throw;
			}
		}
		else
		{
			switch (formationIndex)
			{
			case 0:
				for (int i = 0; i < 10; ++i)
					targetShifts.push_back(12 * i);
				for (int j = 9; j >= 0; --j)
				{
					processMovesWithShiftsHoris(targetShifts, j, 0, 6, 12);
				}
				break;
			case 1:
				for (int i = 0; i < 5; ++i)
					targetShifts.push_back(-6 - 12 * (4 - i));
				for (int i = 0; i < 5; ++i)
					targetShifts.push_back(6 + 12 * i);
				for (int j = 0; j < 5; ++j)
				{
					processMovesWithShiftsHoris(targetShifts, j, 0, -6, -12);
				}
				for (int j = 9; j >= 5; --j)
				{
					processMovesWithShiftsHoris(targetShifts, j, 0, 6, 12);
				}
				break;
			case 2:
				for (int i = 0; i < 10; ++i)
					targetShifts.push_back(-12 * i);
				std::reverse(targetShifts.begin(), targetShifts.end());
				for (int j = 0; j <= 9; ++j)
				{
					processMovesWithShiftsHoris(targetShifts, j, 0, -6, -12);
				}
				break;
			default:
				throw;
			}
		}
	} });

	// TANK IFV ARRV
}

void MyStrategy::move(const Player& me, const World& world, const Game& game, Move& move)
{
	if (world.getTickIndex() == 0)
		firstTickActions(me, world, game, move);

	for (auto& x : world.getVehicleUpdates())
		if (mOurVehicles.count(x.getId()))
		{
			mOurVehicles[x.getId()].mX = x.getX();
			mOurVehicles[x.getId()].mY = x.getY();
			if (!x.getDurability())
				mOurVehicles.erase(x.getId());
		}
		else
		{
			mEnemyVehicles[x.getId()].mX = x.getX();
			mEnemyVehicles[x.getId()].mY = x.getY();
			if (!x.getDurability())
				mEnemyVehicles.erase(x.getId());
		}

	while (mDelayedFunctions.size() && mDelayedFunctions.top().first <= world.getTickIndex())
	{
		mExecutionQueue.push_back(mDelayedFunctions.top().second);
		mDelayedFunctions.pop();
	}

	if (mExecutionQueue.size() && world.getTickIndex() % 5 == 0)
	{
		mExecutionQueue.front()(move, world);
		mExecutionQueue.pop_front();
	}

	// TANK && FIGHTER
	// IFV & HELICOPTER
	// ARRV
	// On a circle, or just a block
}

MyStrategy::MyStrategy()
{
}
