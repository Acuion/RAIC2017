#include "MyStrategy.h"
#include "MyFormationBruteforcer.h"
#include <set>

#define PI 3.14159265358979323846
#define _USE_MATH_DEFINES
#define sq(x) ((x)*(x))

#define VALFHDR [=](Move& move, const World& world)
#define REFFHDR [&](Move& move, const World& world)

void MyStrategy::selectVehicles(VehicleType vt, Move& mv)
{
	mv.setAction(ActionType::CLEAR_AND_SELECT);
	mv.setRight(1024);
	mv.setBottom(1024);
	mv.setVehicleType(vt);
}

bool MyStrategy::nukeEmAll(const Player& me, const World& world, Move& move)
{
	set<xypoint> nukePlaces;
	map<xypoint, int> ptid;

	for (auto& u : mGlobaler.getOurVehicles())
	{
		xypoint gridedPos = {round(u.second.mX / 4) * 4, round(u.second.mY / 4) * 4};
		double r = 0;
		switch (u.second.mType)
		{
		case VehicleType::ARRV:
			r = 60;
			break;
		case VehicleType::FIGHTER:
			r = 120;
			break;
		case VehicleType::HELICOPTER:
			r = 100;
			break;
		case VehicleType::IFV:
			r = 80;
			break;
		case VehicleType::TANK:
			r = 80;
			break;
		}
		r *= 0.6;
		r = floor(r / 4) * 4;

		for (int x = gridedPos.first - r; x <= gridedPos.first + r; x += 16)
			for (int y = gridedPos.second - r; y <= gridedPos.second + r; y += 16)
				if ((x - gridedPos.first) * (x - gridedPos.first) + (y - gridedPos.second) * (y - gridedPos.second) <= r * r)
				{
					nukePlaces.insert({x, y});
					ptid[{x, y}] = u.first;
				}
	}

	xypoint bestnp = {-1, -1};
	double bestscore = -1e9;

	for (auto& np : nukePlaces)
	{
		double score = 0;
		for (auto& u : mGlobaler.getEnemyVehicles())
		{
			double dist = sqrt(sq(np.first - u.second.mX) + sq(np.second - u.second.mY));
			if (dist > 50)
				continue;
			score += (99 - 99 * (dist / 50));
		}
		for (auto& u : mGlobaler.getOurVehicles())
		{
			double dist = sqrt(sq(np.first - u.second.mX) + sq(np.second - u.second.mY));
			if (dist > 50)
				continue;
			score -= (99 - 99 * (dist / 50)) * 0.7;
		}
		if (score > bestscore)
		{
			bestscore = score;
			bestnp = np;
		}
	}

	if (bestscore > 0.3)
	{
		move.setAction(ActionType::TACTICAL_NUCLEAR_STRIKE);
		move.setVehicleId(ptid[bestnp]);
		move.setX(bestnp.first);
		move.setY(bestnp.second);
		mLastNuke = world.getTickIndex();
		return true;
	}

	return false;
}

shared_ptr<MyUnitGroup> MyStrategy::createGroup(Move& move, const World& world)
{
	auto sp = make_shared<MyUnitGroup>(move, world, mGlobaler);
	mGroupActors.push_back(sp);
	return sp;
}

void MyStrategy::lockMacroInterruptions()
{
	mDoNotInterruptMacroPlease = true;
}

void MyStrategy::unlockMacroInterruptions()
{
	mDoNotInterruptMacroPlease = false;
}

bool MyStrategy::macroMayBeInterrupted()
{
	return !mDoNotInterruptMacroPlease;
}

bool MyStrategy::nukePanic(Move& move, const World& world)
{
	if (!mPanic)
	{
		xypoint theCenter = { 0,0 };
		for (auto& x : mGlobaler.getOurVehicles())
		{
			theCenter.first += x.second.mX;
			theCenter.second += x.second.mY;
		}
		theCenter.first /= mGlobaler.getOurVehicles().size();
		theCenter.second /= mGlobaler.getOurVehicles().size();

		double disttonuke1 = sqrt(
			(world.getMyPlayer().getNextNuclearStrikeX() - theCenter.first) * (world.getMyPlayer().getNextNuclearStrikeX() -
				theCenter.first) +
				(world.getMyPlayer().getNextNuclearStrikeY() - theCenter.second) * (world.getMyPlayer().getNextNuclearStrikeY() -
					theCenter.second));
		double disttonuke2 = sqrt(
			(world.getOpponentPlayer().getNextNuclearStrikeX() - theCenter.first) * (world.getOpponentPlayer().
				getNextNuclearStrikeX() - theCenter.
				first) +
				(world.getOpponentPlayer().getNextNuclearStrikeY() - theCenter.second) * (world.getOpponentPlayer().
					getNextNuclearStrikeY() - theCenter.
					second));

		if (disttonuke2 < 110 && world.getOpponentPlayer().getNextNuclearStrikeX() >= 0)
		{
			mPanic = true;
			mPanicPoint = {
				world.getOpponentPlayer().getNextNuclearStrikeX(), world.getOpponentPlayer().getNextNuclearStrikeY()
			};
		}
		else if (disttonuke1 < 110 && world.getMyPlayer().getNextNuclearStrikeX() >= 0)
		{
			mPanic = true;
			mPanicPoint = { world.getMyPlayer().getNextNuclearStrikeX(), world.getMyPlayer().getNextNuclearStrikeY() };
		}
		if (mPanic)
		{
			mPanicTime = world.getTickIndex();
		}
	}
	if (mPanic)
	{
		if (mPanicSelection)
		{
			if (world.getTickIndex() - mPanicTime <= 30)
			{
				move.setAction(ActionType::SCALE);
				move.setX(mPanicPoint.first);
				move.setY(mPanicPoint.second);
				move.setFactor(10);
			}
			else if (world.getTickIndex() - mPanicTime <= 60)
			{
				move.setAction(ActionType::SCALE);
				move.setX(mPanicPoint.first);
				move.setY(mPanicPoint.second);
				move.setFactor(0.1);
			}
			else
			{
				mPanicSelection = mPanic = false;
			}
		}
		else
		{
			move.setAction(ActionType::CLEAR_AND_SELECT);
			move.setRight(1024);
			move.setBottom(1024);
			mPanicSelection = true;
		}
	}
	return mPanic;
	// todo: store selection! (or not, xm)
	// calculate groups AABB
	// select groups near the nuke
	// scale out
	// scale in
	// restore selection! (or not, xm)
}

void MyStrategy::firstTickActions(const Player& me, const World& world, const Game& game, Move& move)
{
	lockMacroInterruptions(); // !!!!!

	if (world.getFacilities().size())
		mGameMode = GameMode::Round2;
	else
		mGameMode = GameMode::Round1;

	const auto getCenterOfGroup = [&](VehicleType vt)
	{
		double x = 0, y = 0, count = 0;
		for (auto& q : mGlobaler.getOurVehicles())
			if (q.second.mType == vt)
			{
				x += q.second.mX;
				y += q.second.mY;
				count++;
			}

		return make_pair(x / count, y / count);
	};
	const auto passFunc = function<bool(const World&)>([=](const World&) { return true; });
	const auto allStopedFunc = function<bool(const World&)>([=](const World&) { return !mGlobaler.anyAllyMoved(); });
	const auto pushToTheFrontOfQueue = [=](turnPrototype func)
	{
		mMacroExecutionQueue.push_front(func);
		swap(mMacroExecutionQueue[0], mMacroExecutionQueue[1]);
	};

	const xypoint tankCenter = getCenterOfGroup(VehicleType::TANK);
	const xypoint ifvCenter = getCenterOfGroup(VehicleType::IFV);
	const xypoint arrvCenter = getCenterOfGroup(VehicleType::ARRV);
	const xypoint helicopterCenter = getCenterOfGroup(VehicleType::HELICOPTER);
	const xypoint fighterCenter = getCenterOfGroup(VehicleType::FIGHTER);

	xypoint tankCell, ifvCell, arrvCell, fighterCell, helicopterCell;

	tankCell.first = round((tankCenter.first - 45) / 75.0);
	tankCell.second = round((tankCenter.second - 45) / 75.0);
	ifvCell.first = round((ifvCenter.first - 45) / 75.0);
	ifvCell.second = round((ifvCenter.second - 45) / 75.0);
	arrvCell.first = round((arrvCenter.first - 45) / 75.0);
	arrvCell.second = round((arrvCenter.second - 45) / 75.0);
	fighterCell.first = round((fighterCenter.first - 45) / 75.0);
	fighterCell.second = round((fighterCenter.second - 45) / 75.0);
	helicopterCell.first = round((helicopterCenter.first - 45) / 75.0);
	helicopterCell.second = round((helicopterCenter.second - 45) / 75.0);

	auto mfb = MyFormationBruteforcer(tankCell, ifvCell, arrvCell, fighterCell, helicopterCell);
	mfb.buildPathToFormation();

	auto doThePath = [&](const vector<FormationStep>& path, bool immStart)
	{
		vector<FormationStep> nowRunning;

		if (immStart)
			nowRunning.push_back({ VehicleType::_UNKNOWN_,{ -1, -1 },{ -1, -1 } });

		const auto intersectionHandler = [&]
		{
			nowRunning.clear();
		};

		for (int i = 0; i < path.size(); ++i)
		{
			auto& turn = path[i];

			bool intersects = false;

			for (auto& x : nowRunning)
				if (x.mMoveFrom == turn.mMoveFrom || x.mMoveFrom == turn.mMoveTo || x.mMoveTo == turn.mMoveTo || x.mMoveTo == turn.
					mMoveFrom)
					intersects = true;

			if (intersects)
			{
				intersectionHandler();
			}

			double moveAngle = atan2(turn.mMoveTo.second - turn.mMoveFrom.second, turn.mMoveTo.first - turn.mMoveFrom.first);
			mMacroConditionalQueue.push_back(make_pair(nowRunning.size() ? passFunc : allStopedFunc,
				VALFHDR
			{
				selectVehicles(turn.mVt, move);
				pushToTheFrontOfQueue(VALFHDR
				{
					move.setAction(ActionType::MOVE);
					move.setX(cos(moveAngle) * 75);
					move.setY(sin(moveAngle) * 75);
					if (fabs(move.getX()) < 1e-6)
						move.setX(0);
					if (fabs(move.getY()) < 1e-6)
						move.setY(0);
				});
			}));

			nowRunning.push_back(turn);
		}
		intersectionHandler();
	};

	doThePath(mfb.getAirFormationPath(), false);
	doThePath(mfb.getLandFormationPath(), true);

	// 2.0 - radius
	// dist btw units = 6
	const bool horisontalFormation = mfb.getLandFormation()[0].second == mfb.getLandFormation()[1].second;
	const int formationIndex = (horisontalFormation ? mfb.getLandFormation()[0].second : mfb.getLandFormation()[0].first);



	mMacroConditionalQueue.push_back({
		allStopedFunc, VALFHDR
	{
		const xypoint tankCenter = getCenterOfGroup(VehicleType::TANK);
		const xypoint ifvCenter = getCenterOfGroup(VehicleType::IFV);
		const xypoint arrvCenter = getCenterOfGroup(VehicleType::ARRV);

		vector<double> targetShifts;

		const auto selectHorisontallyJthRow = [=](Move& move, xypoint center, int j, VehicleType type)
		{
			move.setAction(ActionType::CLEAR_AND_SELECT);
			//move.setVehicleType(type);
			move.setTop(center.second + (j - 5) * 6);
			move.setBottom(center.second + (j - 4) * 6);
			move.setLeft(center.first - (1 + 5 * 6));
			move.setRight(center.first + (1 + 5 * 6));
		};

		const auto selectVerticallyJthRow = [=](Move& move, xypoint center, int j, VehicleType type)
		{
			move.setAction(ActionType::CLEAR_AND_SELECT);
			//move.setVehicleType(type);
			move.setLeft(center.first + (j - 5) * 6);
			move.setRight(center.first + (j - 4) * 6);
			move.setTop(center.second - (1 + 5 * 6));
			move.setBottom(center.second + (1 + 5 * 6));
		};

		const auto enqueueToFrontMoveVertOnJthShiftWithShift = [=](vector<double> targetShifts, int j, int shift)
		{
			pushToTheFrontOfQueue(VALFHDR
			{
				move.setAction(ActionType::MOVE);
				move.setY(targetShifts[j] + shift);
			});
		};

		const auto enqueueToFrontMoveHorisOnJthShiftWithShift = [=](vector<double> targetShifts, int j, int shift)
		{
			pushToTheFrontOfQueue(VALFHDR
			{
				move.setAction(ActionType::MOVE);
				move.setX(targetShifts[j] + shift);
			});
		};

		const auto processMovesWithShiftsVert = [=](vector<double> targetShifts, int j, int tankShift, int ifvShift,
			int arrvShift)
		{
			mMacroExecutionQueue.push_back(VALFHDR
			{
				selectHorisontallyJthRow(move, tankCenter, j, VehicleType::TANK);
				enqueueToFrontMoveVertOnJthShiftWithShift(targetShifts, j, tankShift);
			});
			mMacroExecutionQueue.push_back(VALFHDR
			{
				selectHorisontallyJthRow(move, ifvCenter, j, VehicleType::IFV);
				enqueueToFrontMoveVertOnJthShiftWithShift(targetShifts, j, ifvShift);
			});
			mMacroExecutionQueue.push_back(VALFHDR
			{
				selectHorisontallyJthRow(move, arrvCenter, j, VehicleType::ARRV);
				enqueueToFrontMoveVertOnJthShiftWithShift(targetShifts, j, arrvShift);
			});
		};

		const auto processMovesWithShiftsHoris = [=](vector<double> targetShifts, int j, int tankShift, int ifvShift,
			int arrvShift)
		{
			mMacroExecutionQueue.push_back(VALFHDR
			{
				selectVerticallyJthRow(move, tankCenter, j, VehicleType::TANK);
				enqueueToFrontMoveHorisOnJthShiftWithShift(targetShifts, j, tankShift);
			});
			mMacroExecutionQueue.push_back(VALFHDR
			{
				selectVerticallyJthRow(move, ifvCenter, j, VehicleType::IFV);
				enqueueToFrontMoveHorisOnJthShiftWithShift(targetShifts, j, ifvShift);
			});
			mMacroExecutionQueue.push_back(VALFHDR
			{
				selectVerticallyJthRow(move, arrvCenter, j, VehicleType::ARRV);
				enqueueToFrontMoveHorisOnJthShiftWithShift(targetShifts, j, arrvShift);
			});
		};

		xypoint theCenter = {
			(tankCenter.first + ifvCenter.first + arrvCenter.first) / 3,
			(tankCenter.second + ifvCenter.second + arrvCenter.second) / 3
		};
		if (formationIndex == 0)
		{
			if (horisontalFormation)
				theCenter.second += 75;
			else
				theCenter.first += 75;
		}

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
			case 2:
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
			default:
				throw;
			}
			mMacroConditionalQueue.push_back({
				allStopedFunc, VALFHDR
			{
				move.setAction(ActionType::CLEAR_AND_SELECT);
				move.setLeft(0);
				move.setRight(45 + 50);
				move.setTop(0);
				move.setBottom(1024);
				pushToTheFrontOfQueue(VALFHDR
				{
					move.setAction(ActionType::MOVE);
					move.setX(75);
					pushToTheFrontOfQueue(VALFHDR
					{
						move.setAction(ActionType::CLEAR_AND_SELECT);
						move.setLeft(160);
						move.setRight(1024);
						move.setTop(0);
						move.setBottom(1024);
						pushToTheFrontOfQueue(VALFHDR
						{
							move.setAction(ActionType::MOVE);
							move.setX(-75);
							pushToTheFrontOfQueue(VALFHDR
							{
								move.setAction(ActionType::CLEAR_AND_SELECT);
								move.setTop(0);
								move.setBottom(1024);
								move.setLeft(0);
								move.setRight(1024);
							});
						});
					});
				});
			}
			});
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
			case 2:
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
			default:
				throw;
			}

			mMacroConditionalQueue.push_back({
				allStopedFunc, VALFHDR
			{
				move.setAction(ActionType::CLEAR_AND_SELECT);
				move.setTop(0);
				move.setBottom(45 + 50);
				move.setLeft(0);
				move.setRight(1024);
				pushToTheFrontOfQueue(VALFHDR
				{
					move.setAction(ActionType::MOVE);
					move.setY(75);
					pushToTheFrontOfQueue(VALFHDR
					{
						move.setAction(ActionType::CLEAR_AND_SELECT);
						move.setTop(160);
						move.setBottom(1024);
						move.setLeft(0);
						move.setRight(1024);
						pushToTheFrontOfQueue(VALFHDR
						{
							move.setAction(ActionType::MOVE);
							move.setY(-75);
							pushToTheFrontOfQueue(VALFHDR
							{
								move.setAction(ActionType::CLEAR_AND_SELECT);
								move.setTop(0);
								move.setBottom(1024);
								move.setLeft(0);
								move.setRight(1024);
							});
						});
					});
				});
			}
			});
		}

		auto finishCreation = [=]
		{
			mMacroConditionalQueue.push_back({
				passFunc, VALFHDR
			{
				move.setAction(ActionType::CLEAR_AND_SELECT);
				move.setRight(1024);
				move.setBottom(1024);
			}
			});
			mMacroConditionalQueue.push_back({
				allStopedFunc, VALFHDR
			{
				move.setAction(ActionType::ROTATE);
				move.setX(theCenter.first);
				move.setY(theCenter.second);
				if (horisontalFormation)
					move.setAngle(PI / 4);
				else
					move.setAngle(-PI / 4);
			}
			});
			mMacroConditionalQueue.push_back({
				allStopedFunc, VALFHDR
			{
				move.setAction(ActionType::SCALE);
				move.setX(theCenter.first);
				move.setY(theCenter.second);
				move.setFactor(0.2);
				unlockMacroInterruptions(); // !!!!
			}
			});
			if (mGameMode == GameMode::Round1)
				mMacroConditionalQueue.push_back({ allStopedFunc, mInfinityChaseRound1 });
			else
				mMacroConditionalQueue.push_back({ allStopedFunc, mInfinityChaseRound2 });
		};

		if (mGameMode == GameMode::Round2)
		{
			mMacroConditionalQueue.push_back({
				allStopedFunc, VALFHDR
			{
				if (!horisontalFormation)
				{
					move.setAction(ActionType::CLEAR_AND_SELECT);
					move.setRight(theCenter.first);
					move.setBottom(1024);
					pushToTheFrontOfQueue(VALFHDR
					{
						mGroup1 = createGroup(move, world);
						pushToTheFrontOfQueue(VALFHDR
						{
							move.setAction(ActionType::CLEAR_AND_SELECT);
							move.setLeft(theCenter.first);
							move.setRight(1024);
							move.setBottom(1024);
							pushToTheFrontOfQueue(VALFHDR
							{
								mGroup2 = createGroup(move, world);
								finishCreation();
							});
						});
					});
				}
				else
				{
					move.setAction(ActionType::CLEAR_AND_SELECT);
					move.setBottom(theCenter.first);
					move.setRight(1024);
					pushToTheFrontOfQueue(VALFHDR
					{
						mGroup1 = createGroup(move, world);
						pushToTheFrontOfQueue(VALFHDR
						{
							move.setAction(ActionType::CLEAR_AND_SELECT);
							move.setTop(theCenter.first);
							move.setBottom(1024);
							move.setRight(1024);
							pushToTheFrontOfQueue(VALFHDR
							{
								mGroup2 = createGroup(move, world);
								finishCreation();
							});
						});
					});
				}
			}
			});
		}
		else
			finishCreation();
	}
	});
}

void MyStrategy::move(const Player& me, const World& world, const Game& game, Move& move)
{
	mGlobaler.processNews(world.getNewVehicles(), me.getId());
	mGlobaler.processUpdates(world.getVehicleUpdates());

	if (world.getTickIndex() == 0)
		firstTickActions(me, world, game, move);

	if (world.getTickIndex() % 5 == 0)
	{
		if (mLastNuke + me.getRemainingNuclearStrikeCooldownTicks() < world.getTickIndex() && nukeEmAll(me, world, move))
			return;
			
		int startedFrom = mCurrActingGroup;
		while (true)
		{
			bool moved = false;
			bool mayBeInterrupted;
			if (mCurrActingGroup == -1) // macro actions
			{
				mayBeInterrupted = macroMayBeInterrupted();
			}
			else
			{
				mayBeInterrupted = mGroupActors[mCurrActingGroup]->mayBeInterrupted();
			}

			if (mayBeInterrupted && nukePanic(move, world))
				return; // todo: move somewhere else?

			if (mCurrActingGroup == -1) // macro actions
			{
				if (!mMacroExecutionQueue.size() && mMacroConditionalQueue.size())
					if (mMacroConditionalQueue.front().first(world))
					{
						mMacroExecutionQueue.push_back(mMacroConditionalQueue.front().second);
						mMacroConditionalQueue.pop_front();
					}
				if (mMacroExecutionQueue.size())
				{
					mMacroExecutionQueue.front()(move, world);
					mMacroExecutionQueue.pop_front();
					moved = true;
				}
			}
			else
			{
				moved = mGroupActors[mCurrActingGroup]->act(move, world);
			}
			mThisGroupActedTimes++;
			if (mayBeInterrupted && (!moved || mThisGroupActedTimes >= 2))
			{
				mThisGroupActedTimes = 0;
				mCurrActingGroup = (mCurrActingGroup + 1) % (mGroupActors.size() + 1);
				if (mCurrActingGroup == mGroupActors.size())
					mCurrActingGroup = -1;
			}
			if (moved || mCurrActingGroup == startedFrom)
				break;
		}
	}
}

MyStrategy::MyStrategy()
	: mPanic(false)
	, mPanicSelection(false)
	, mLastNuke(-10000)
	, mDoNotInterruptMacroPlease(false)
	, mCurrActingGroup(-1)
	, mThisGroupActedTimes(0)
{
	mInfinityChaseRound1 = VALFHDR
	{
		xypoint theCenter = {0,0};
		for (auto& x : mGlobaler.getOurVehicles())
		{
			theCenter.first += x.second.mX;
			theCenter.second += x.second.mY;
		}
		theCenter.first /= mGlobaler.getOurVehicles().size();
		theCenter.second /= mGlobaler.getOurVehicles().size();

		xypoint nearest = {512, 512};
		double currDist = 1e9;
		for (auto& x : mGlobaler.getEnemyVehicles())
		{
			double dist = sqrt((x.second.mX - theCenter.first) * (x.second.mX - theCenter.first) +
				(x.second.mY - theCenter.second) * (x.second.mY - theCenter.second));
			if (dist < currDist)
			{
				nearest = {x.second.mX, x.second.mY};
				currDist = dist;
			}
		}

		if (world.getTickIndex() - mLastNuke > 30)
		{
			move.setAction(ActionType::MOVE);
			move.setX(nearest.first - theCenter.first);
			move.setY(nearest.second - theCenter.second);
			move.setMaxSpeed(0.18);
			if (abs(move.getX()) < 32 && abs(move.getY()) < 32 && world.getTickIndex() % 10)
			{
				move.setAction(ActionType::SCALE);
				move.setX(theCenter.first + 12);
				move.setY(theCenter.second + 12);
				move.setFactor(0.91);
				move.setMaxSpeed(0.15);
			}
		}
		mMacroExecutionQueue.push_back(mInfinityChaseRound1);
	};
	mInfinityChaseRound2 = VALFHDR
	{
		mGroup1->move({0, 50}, false);
		mGroup2->move({50, 0}, false);

		mMacroExecutionQueue.push_back(mInfinityChaseRound2);
	};
}
