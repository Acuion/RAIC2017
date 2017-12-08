#include "MyStrategy.h"
#include "MyFormationBruteforcer.h"
#include <set>

#define PI 3.14159265358979323846
#define _USE_MATH_DEFINES
#define sq(x) ((x)*(x))

#define VALFHDR [=](Move& move, const World& world)
#define GRVALFHDR [=](Move& move, const World& world, MyUnitGroup& thisGroup)
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
		xypoint gridedPos = {u.second.mX / 16, u.second.mY / 16};
		double r = 0;
		bool air = false;
		switch (u.second.mType)
		{
		case VehicleType::ARRV:
			r = 60;
			break;
		case VehicleType::FIGHTER:
			air = true;
			r = 120;
			break;
		case VehicleType::HELICOPTER:
			air = true;
			r = 100;
			break;
		case VehicleType::IFV:
			r = 80;
			break;
		case VehicleType::TANK:
			r = 80;
			break;
		}
		double mdf = 1;
		if (air)
		{
			switch (world.getWeatherByCellXY()[u.second.mX / 32][u.second.mY / 32])
			{
			case WeatherType::CLEAR:
				break;
			case WeatherType::CLOUD:
				mdf = 0.8;
				break;
			case WeatherType::RAIN:
				mdf = 0.6;
				break;
			}
		}
		else
		{
			switch (world.getTerrainByCellXY()[u.second.mX / 32][u.second.mY / 32])
			{
			case TerrainType::PLAIN: break;
			case TerrainType::SWAMP:
				break;
			case TerrainType::FOREST:
				mdf = 0.8;
				break;
			}
		}
		mdf = 0.6; // todo
		r *= mdf;
		r /= 16;

		for (int x = gridedPos.first - r; x <= gridedPos.first + r; ++x)
			for (int y = gridedPos.second - r; y <= gridedPos.second + r; ++y)
				if (x >= 0 && y >= 0 && x < 64 && y < 64 && (x - gridedPos.first) * (x - gridedPos.first) + (y - gridedPos.second) * (y - gridedPos.second) <= r * r)
				{
					nukePlaces.insert({x, y});
					ptid[{x, y}] = u.first;
				}
	}

	xypoint bestnp = {-1, -1};
	double bestscore = -1e9;

	for (auto& np : nukePlaces)
	{
		const double score = mGlobaler.getNukeValueAtCell(np.first, np.second);
		if (score > bestscore)
		{
			bestscore = score;
			bestnp = np;
		}
	}

	static double nukeAttempts = 0;
	if (bestscore > 30)
	{
		nukeAttempts++;
		if (nukeAttempts < 20)
			return false;
		move.setAction(ActionType::TACTICAL_NUCLEAR_STRIKE);
		move.setVehicleId(ptid[bestnp]);
		move.setX(bestnp.first * 16);
		move.setY(bestnp.second * 16); // todo: 16 -> gridsize
		mLastNuke = world.getTickIndex();
		return true;
	}

	return false;
}

shared_ptr<MyUnitGroup> MyStrategy::createGroup(Move& move, const World& world, double angle)
{
	auto sp = make_shared<MyUnitGroup>(move, world, mGlobaler, angle);
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
		xypoint ournp = {world.getMyPlayer().getNextNuclearStrikeX(), world.getMyPlayer().getNextNuclearStrikeY()};
		xypoint enenp = {
			world.getOpponentPlayer().getNextNuclearStrikeX(), world.getOpponentPlayer().getNextNuclearStrikeY()
		};

		const auto countDamage = [&](xypoint point)
		{
			double dmg = 0;
			for (auto& x : mGlobaler.getOurVehicles())
			{
				double dst = hypot(x.second.mX - point.first, x.second.mY - point.second);
				if (dst > 50)
					continue;
				dmg += (99 - 99 * (dst / 50));
			}
			return dmg;
		};

		if (enenp.first >= 0)
		{
			if (countDamage(enenp) > 1)
			{
				mPanic = true;
				mPanicPoint = enenp;
			}
		}
		else if (ournp.first >= 0)
		{
			if (countDamage(ournp) > 2)
			{
				mPanic = true;
				mPanicPoint = ournp;
			}
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
				move.setAction(ActionType::MOVE);
				// stop
				return true;
			}
		}
		else
		{
			move.setAction(ActionType::CLEAR_AND_SELECT);
			xypoint lu = mPanicPoint, rb = mPanicPoint;
			lu.first -= 50;
			lu.second -= 50;
			rb.first += 50;
			rb.second += 50;

			bool ok = false;
			while (!ok)
			{
				ok = true;
				for (auto& x : mGroupActors)
				{
					auto aabb = x->getGridedAabb();
					aabb.first.first *= 16;
					aabb.first.second *= 16;
					aabb.second.first *= 16;
					aabb.second.second *= 16;

					auto pointInside = [&](xypoint pt)
					{
						return pt.first >= aabb.first.first && pt.second >= aabb.first.second && pt.first <= aabb.second.first && pt.second <= aabb.second.second;
					};

					if (!pointInside(lu) && !pointInside(rb) && !pointInside({lu.first, rb.second}) && !pointInside({rb.first, lu.second}))
						continue;

					if (aabb.first.first < lu.first)
					{
						ok = false;
						lu.first = aabb.first.first;
					}
					if (aabb.first.second < lu.second)
					{
						ok = false;
						lu.second = aabb.first.second;
					}


					if (aabb.second.first > rb.first)
					{
						ok = false;
						rb.first = aabb.second.first;
					}
					if (aabb.second.second > rb.second)
					{
						ok = false;
						rb.second = aabb.second.second;
					}
				}
			}

			move.setLeft(lu.first - 5);
			move.setTop(lu.second - 5);
			move.setRight(rb.first + 5);
			move.setBottom(rb.second + 5);
			mPanicSelection = true;
			MyUnitGroup::dropSelection();
		}
	}
	return mPanic;
}

void MyStrategy::pushToTheFrontOfQueue(macroTurnPrototype func)
{
	mMacroExecutionQueue.push_front(func);
	swap(mMacroExecutionQueue[0], mMacroExecutionQueue[1]);
};

void MyStrategy::firstTickActionsRound1(const Player& me, const World& world, const Game& game, Move& move)
{
	lockMacroInterruptions(); // !!!!!

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
			nowRunning.push_back({VehicleType::_UNKNOWN_,{-1, -1},{-1, -1}});

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
				}
			});
			mMacroConditionalQueue.push_back({
				passFunc, VALFHDR
				{
					mSandwichGroup = createGroup(move, world, PI / 4);
					mSandwichGroup->pushToConditionalQueue(CondQueueCondition::NoCondition, mInfinityChaseRound1, false);
					unlockMacroInterruptions(); // !!!!
				}
			});
			mMacroConditionalQueue.push_back({
				[=](const World& world) { return world.getTickIndex() - mLastNoobsToFight > 500; }, mNoobsToTheFight
			});
		}
	});
}

void MyStrategy::move(const Player& me, const World& world, const Game& game, Move& move)
{
	mGlobaler.setMyId(me.getId());
	mGlobaler.processNews(world.getNewVehicles(), me.getId());
	mGlobaler.processUpdates(world.getVehicleUpdates());
	mGlobaler.updateFacilities(world.getFacilities(), world.getTickIndex());

	if (world.getTickIndex() == 0)
	{
		if (world.getFacilities().size())
			mGameMode = GameMode::Round2;
		else
			mGameMode = GameMode::Round1;

		if (mGameMode == GameMode::Round1)
			firstTickActionsRound1(me, world, game, move);
		else
		{
			mMacroConditionalQueue.push_back({
				[=](const World& world) { return world.getTickIndex() - mLastNoobsToFight > 250; }, mNoobsToTheFight
			});
			auto pushVehicleGroup = [this](VehicleType vt)
			{			
				mMacroExecutionQueue.push_back(VALFHDR
				{
					selectVehicles(vt, move);
				});
				mMacroExecutionQueue.push_back(VALFHDR
				{
					auto gr = createGroup(move, world, PI / 2);
					gr->setVehicleType(vt);
					gr->pushToConditionalQueue(CondQueueCondition::NoCondition, GRVALFHDR
					{
						thisGroup.scale(thisGroup.getCenterOfGroup(), 0.1, move, world);
					});
					gr->pushToConditionalQueue(CondQueueCondition::AllUnitStopped, mSmartChaseRound2);
				});
			};
			pushVehicleGroup(VehicleType::IFV);
			pushVehicleGroup(VehicleType::TANK);
			pushVehicleGroup(VehicleType::ARRV);
			pushVehicleGroup(VehicleType::HELICOPTER);
			pushVehicleGroup(VehicleType::FIGHTER);
		}
	}

	if (world.getTickIndex() % 5 == 0)
	{
		bool nukeReady = mLastNuke + me.getRemainingNuclearStrikeCooldownTicks() < world.getTickIndex();

		mGlobaler.buildMaps(mGroupActors, nukeReady);

		if (nukeReady && nukeEmAll(me, world, move))
			return;

		auto newf = mGlobaler.getNewFacility();
		if (newf.first != -1 && newf.second == FacilityType::VEHICLE_FACTORY)
		{
			mMacroConditionalQueue.push_back({
				[=](const World& world)
				{
					return mGlobaler.getOurFacilities().count(newf.first)
						       ? world.getTickIndex() - mGlobaler.getOurFacilities().at(newf.first).mCapturedAt > 200
						       : true;
				},
			VALFHDR
				{
					move.setAction(ActionType::SETUP_VEHICLE_PRODUCTION);
					move.setFacilityId(newf.first);
					move.setVehicleType(VehicleType::IFV);
				}
			});
		}

		int startedFrom = mCurrActingGroup;
		while (true)
		{
			bool moved = false;
			bool mayBeInterrupted;
			auto const updateIntStatus = [&]
			{
				if (mCurrActingGroup == -1) // macro actions
				{
					mayBeInterrupted = macroMayBeInterrupted();
				}
				else
				{
					mayBeInterrupted = mGroupActors[mCurrActingGroup]->mayBeInterrupted();
				}
			};
			updateIntStatus();
			if (mayBeInterrupted && nukePanic(move, world))
				return;

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
			updateIntStatus(); // after a move
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
	  , mLastNoobsToFight(-1)
	  , mDoNotInterruptMacroPlease(false)
	  , mCurrActingGroup(-1)
	  , mThisGroupActedTimes(0)
{
	mInfinityChaseRound1 = GRVALFHDR
	{
		xypoint theCenter = thisGroup.getCenterOfGroup();

		xypoint nearest = {512, 512};
		double currDist = 1e9;
		for (auto& x : mGlobaler.getEnemyVehicles())
		{
			double dist = hypot(x.second.mX - theCenter.first, x.second.mY - theCenter.second);
			if (dist < currDist)
			{
				nearest = {x.second.mX, x.second.mY};
				currDist = dist;
			}
		}

		if (world.getTickIndex() - mLastNuke > 30)
		{
			thisGroup.move({nearest.first - theCenter.first, nearest.second - theCenter.second}, true, move, world);
			if (abs(move.getX()) < 32 && abs(move.getY()) < 32 && thisGroup.getGroupActsCount() % 3 == 0)
			{
				move.setAction(ActionType::SCALE);
				move.setX(theCenter.first + 12);
				move.setY(theCenter.second + 12);
				move.setFactor(0.91);
				move.setMaxSpeed(0.15);
			}
		}

		const auto absAnglesDiff = [](double a, double b)
		{
			double diff = min(abs(a - b), 2 * PI - abs(a - b));
			return min(diff, abs(diff - PI));
		};

		double angleToNearest = atan2(nearest.second - theCenter.second, nearest.first - theCenter.first);
		double angleDiff = absAnglesDiff(angleToNearest, thisGroup.getGroupAngle());
		const double rotateConst = PI / 18;
		if (angleDiff > PI / 5 && hypot(nearest.first - theCenter.first, nearest.second - theCenter.second) < 32 * 7)
		{
			double plusdiff = absAnglesDiff(angleToNearest, thisGroup.getGroupAngle() + rotateConst);
			double minusdiff = absAnglesDiff(angleToNearest, thisGroup.getGroupAngle() - rotateConst);

			move.setAction(ActionType::ROTATE);
			move.setX(theCenter.first);
			move.setY(theCenter.second);
			if (plusdiff < minusdiff)
			{
				move.setAngle(rotateConst);
			}
			else
			{
				move.setAngle(-rotateConst);
			}
			thisGroup.setGroupAngle(thisGroup.getGroupAngle() + move.getAngle());
			thisGroup.pushToConditionalQueue(CondQueueCondition::AllUnitStopped, mInfinityChaseRound1, false);
		}
		else
		{
			thisGroup.pushToConditionalQueue(CondQueueCondition::NoCondition, mInfinityChaseRound1, false);
		}
	};
	mSmartChaseRound2 = GRVALFHDR
	{
		xypoint theCenter = thisGroup.getCenterOfGroup();
		xypoint lookTo;

		bool land = true;
		switch (thisGroup.getVehicleType())
		{
		case VehicleType::ARRV:
			break;
		case VehicleType::FIGHTER:
			land = false;
			break;
		case VehicleType::HELICOPTER:
			land = false;
			break;
		case VehicleType::IFV:
			break;
		case VehicleType::TANK:
			break;
		}

		xypoint nearestEnemy = thisGroup.getClosestEnemy();
		double currUnitDist = hypot(theCenter.first - nearestEnemy.first, theCenter.second - nearestEnemy.second);

		set<pair<double, xypoint>> nearestStructures;
		for (auto& x : world.getFacilities())
			if (x.getOwnerPlayerId() != mGlobaler.getMyId())
			{
				vector<xypoint> points;
				points.push_back({x.getLeft(), x.getTop()});
				points.push_back({x.getLeft() + 32, x.getTop()});
				points.push_back({x.getLeft(), x.getTop() + 32});
				points.push_back({x.getLeft() + 32, x.getTop() + 32});
				for (auto& pt : points)
				{
					if (pt.first >= 0 && pt.second >= 0 && pt.first < 1024 && pt.second < 1024)
					{
						double dist = hypot(pt.first - theCenter.first, pt.second - theCenter.second);
						nearestStructures.insert({ dist, pt });
					}
				}
			}

		thisGroup.buildMoveMap();

		auto tocle = [&]()
		{
			auto en = thisGroup.getClosestEnemy();
			double angle = atan2(theCenter.second - en.second, theCenter.first - en.first);
			double dist = 70;
			if (thisGroup.getVehicleType() == VehicleType::FIGHTER)
				dist = 110;

			if (land)
			{
				if (thisGroup.getDef() >= mGlobaler.getCellDangerLand(en.first / 16, en.second / 16))
				{
					dist = 10;
				}
			}
			else
			{
				if (thisGroup.getDef() >= mGlobaler.getCellDangerAir(en.first / 16, en.second / 16))
				{
					dist = 10;
				}
			}

			bool finallymvd = false;
			for (int k = 0, l = 0, r = 0; k <= 16; ++k)
			{
				double gangle = angle;
				if (k % 2)
				{
					gangle += (PI / 8) * l;
					++l;
				}
				else
				{
					gangle -= (PI / 8) * r;
					++r;
				}
				xypoint gotop = { en.first + cos(gangle) * dist, en.second + sin(gangle) * dist };
				bool badgotop = false;
				if (gotop.first < 32 || gotop.second < 32 || gotop.first >= 1024 - 32 || gotop.second >= 1024 - 32)
				{
					badgotop = true;
				}

				if (!badgotop)
				{
					if (thisGroup.smartMoveTo(gotop, move, world))
					{
						finallymvd = true;
						break;
					}
				}
			}
			if (!finallymvd)
			{			
				for (int k = 0, l = 0, r = 0; k <= 16; ++k)
				{
					double gangle = angle;
					if (k % 2)
					{
						gangle += (PI / 8) * l;
						++l;
					}
					else
					{
						gangle -= (PI / 8) * r;
						++r;
					}
					xypoint gotop = { en.first + cos(gangle) * dist, en.second + sin(gangle) * dist };
					bool badgotop = false;
					if (gotop.first < 32 || gotop.second < 32 || gotop.first >= 1024 - 32 || gotop.second >= 1024 - 32)
					{
						badgotop = true;
					}

					if (!badgotop)
					{
						thisGroup.move({ gotop.first - theCenter.first, gotop.second - theCenter.second }, true, move, world);
						finallymvd = true;
						break;
					}
				}
			}
			if (!finallymvd)
				thisGroup.move({ rand() % 10, rand() % 10 }, true, move, world);
		};

		if (thisGroup.getVehicleType() != VehicleType::HELICOPTER && thisGroup.getVehicleType() != VehicleType::FIGHTER)
		{
			while (nearestStructures.size() && !thisGroup.smartMoveTo(nearestStructures.begin()->second, move, world))
				nearestStructures.erase(nearestStructures.begin());
			if (!nearestStructures.size())
			{
				tocle();
			}
		}
		else
		{
			tocle();
		}

		if (thisGroup.getGroupRadius() > 47)
		{
			if (world.getTickIndex() % 4)
			{
				move.setAction(ActionType::SCALE);
				move.setX(theCenter.first);
				move.setY(theCenter.second);
				move.setFactor(0.1);
			}
			else
			{
				move.setAction(ActionType::ROTATE);
				move.setX(theCenter.first);
				move.setY(theCenter.second);
				move.setAngle(PI);
			}
		}

		if (thisGroup.getTag() == "noob")
		{
			lookTo = nearestEnemy;

			const auto absAnglesDiff = [](double a, double b)
			{
				double diff = min(abs(a - b), 2 * PI - abs(a - b));
				return min(diff, abs(diff - PI));
			};

			double angleToNearest = atan2(lookTo.second - theCenter.second, lookTo.first - theCenter.first);
			double angleDiff = absAnglesDiff(angleToNearest, thisGroup.getGroupAngle());
			const double rotateConst = PI / 18;
			if (angleDiff > PI / 5 && hypot(lookTo.first - theCenter.first, lookTo.second - theCenter.second) < 32 * 7)
			{
				double plusdiff = absAnglesDiff(angleToNearest, thisGroup.getGroupAngle() + rotateConst);
				double minusdiff = absAnglesDiff(angleToNearest, thisGroup.getGroupAngle() - rotateConst);

				move.setAction(ActionType::ROTATE);
				move.setX(theCenter.first);
				move.setY(theCenter.second);
				if (plusdiff < minusdiff)
				{
					move.setAngle(angleDiff);
				}
				else
				{
					move.setAngle(-angleDiff);
				}
				thisGroup.setGroupAngle(thisGroup.getGroupAngle() + move.getAngle());
				thisGroup.pushToConditionalQueue(CondQueueCondition::AllUnitStopped, mSmartChaseRound2, false);
			}
			else
			{
				thisGroup.pushToConditionalQueue(CondQueueCondition::NoCondition, mSmartChaseRound2, false);
			}
		}
		else
			thisGroup.pushToConditionalQueue(CondQueueCondition::NoCondition, mSmartChaseRound2, false);
	};
	mNoobsToTheFight = VALFHDR
	{
		mLastNoobsToFight = world.getTickIndex();
		for (auto& x : mGlobaler.getOurFacilities())
		{
			if (x.second.mType == FacilityType::VEHICLE_FACTORY && world.getTickIndex() - x.second.mCapturedAt > 1200)
			{
				mMacroExecutionQueue.push_back(VALFHDR
				{
					VehicleType toBuildNext;
					if (x.second.mCurrentlyConstructing == VehicleType::IFV)
						toBuildNext = VehicleType::HELICOPTER;
					else
						toBuildNext = VehicleType::IFV;
					move.setAction(ActionType::SETUP_VEHICLE_PRODUCTION);
					move.setVehicleType(toBuildNext);
					move.setFacilityId(x.first);
					mGlobaler.getOurFacilities()[x.first].mCurrentlyConstructing = toBuildNext;
					pushToTheFrontOfQueue(VALFHDR
					{
						lockMacroInterruptions();
						move.setAction(ActionType::CLEAR_AND_SELECT);
						move.setLeft(x.second.mX - 10);
						move.setTop(x.second.mY - 10);
						move.setRight(x.second.mX + 80);
						move.setBottom(x.second.mY + 80);
						move.setVehicleType(x.second.mCurrentlyConstructing);
						pushToTheFrontOfQueue(VALFHDR
						{
							auto noobs = createGroup(move, world, PI / 2);
							noobs->pushToConditionalQueue(CondQueueCondition::NoCondition, GRVALFHDR{ thisGroup.scale(thisGroup.getCenterOfGroup(), 0.1, move, world); }, false);
							noobs->pushToConditionalQueue(CondQueueCondition::AllUnitStopped, mSmartChaseRound2, false);
							noobs->setVehicleType(x.second.mCurrentlyConstructing);
							noobs->setTag("noob");
							unlockMacroInterruptions();
						});
					});
				});
			}
		}

		mMacroConditionalQueue.push_back({
			[=](const World& world) { return world.getTickIndex() - mLastNoobsToFight > 3000; }, mNoobsToTheFight
		});
	};
}
