#include "MyStrategy.h"
#include "MyFormationBruteforcer.h"
#include <set>

#define PI 3.14159265358979323846
#define _USE_MATH_DEFINES
#define sq(x) ((x)*(x))

#define VALFHDR [=](Move& move, const World& world)
#define REFFHDR [&](Move& move, const World& world)

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
				+
					count++;
			}

		return make_pair(x / count, y / count);
	};

	const auto selectVehicles = [](VehicleType vt, Move& mv)
	{
		mv.setAction(ActionType::CLEAR_AND_SELECT);
		mv.setRight(1024);
		mv.setBottom(1024);
		mv.setVehicleType(vt);
	};

	const xypoint tankCenter = getCenterOfGroup(VehicleType::TANK);
	const xypoint ifvCenter = getCenterOfGroup(VehicleType::IFV);
	const xypoint arrvCenter = getCenterOfGroup(VehicleType::ARRV);
	const xypoint helicopterCenter = getCenterOfGroup(VehicleType::HELICOPTER);
	const xypoint fighterCenter = getCenterOfGroup(VehicleType::FIGHTER);

	const auto passFunc = function<bool(const World&)>([=](const World&) { return true; });
	const auto allStopedFunc = function<bool(const World&)>([=](const World&) { return !mGlobaler.anyAllyMoved(); });

	const auto scaleInType = [=](xypoint center, VehicleType type)
	{
		mMacroExecutionQueue.push_back(VALFHDR
		{
			selectVehicles(type, move);
		});
		mMacroExecutionQueue.push_back(VALFHDR
		{
			move.setAction(ActionType::SCALE);
			move.setX(center.first);
			move.setY(center.second);
			move.setFactor(0.1);
		});
	};

	scaleInType(tankCenter, VehicleType::TANK);
	scaleInType(ifvCenter, VehicleType::IFV);
	scaleInType(arrvCenter, VehicleType::ARRV);
	scaleInType(helicopterCenter, VehicleType::HELICOPTER);
	scaleInType(fighterCenter, VehicleType::FIGHTER);

	const auto create4Groups = [=](xypoint center, VehicleType type, bool waitForMoves)
	{
		const auto createGroupFromSelected = VALFHDR
		{
			auto group = createGroup(move, world);
			const auto annoy = VALFHDR
			{
				const auto ourCenter = group->getCenterOfGroup();
				const auto enemyPoint = group->getClosestEnemy();

				double distToPoint = 1e9;
				xypoint moveTo = {512, 512};

				for (double a = 0; a < 2 * PI; a += 0.18)
				{
					const double radius = 200;
					const double border = 100;
					xypoint candidate = {enemyPoint.first + cos(a) * radius, enemyPoint.second + sin(a) * radius};
					if (candidate.first < border || candidate.second < border || candidate.first > 1024 - border || candidate.second > 1024 - border)
						continue;
					double dist = hypot(candidate.first - ourCenter.first, candidate.second - ourCenter.second);
					if (dist < distToPoint)
					{
						distToPoint = dist;
						moveTo = candidate;
					}
				}

				if (group->getGroupRadius() > 32)
					group->scale(ourCenter, 0.1, move, world);
				else
					group->move({ moveTo.first - ourCenter.first, moveTo.second - ourCenter.second }, false, move, world);
			};
			group->pushToConditionalQueue(CondQueueCondition::NoCondition, annoy, true);
		};

		mMacroConditionalQueue.push_back({ waitForMoves ? allStopedFunc : passFunc , VALFHDR
		{
			move.setAction(ActionType::CLEAR_AND_SELECT);
			move.setLeft(0);
			move.setRight(center.first);
			move.setTop(center.second);
			move.setBottom(1024);
			move.setVehicleType(type);
		} });
		mMacroConditionalQueue.push_back({ passFunc , createGroupFromSelected });

		mMacroConditionalQueue.push_back({ passFunc , VALFHDR
		{
			move.setAction(ActionType::CLEAR_AND_SELECT);
			move.setLeft(center.first);
			move.setRight(1024);
			move.setTop(center.second);
			move.setBottom(1024);
			move.setVehicleType(type);
		} });
		mMacroConditionalQueue.push_back({ passFunc , createGroupFromSelected });

		mMacroConditionalQueue.push_back({ passFunc , VALFHDR
		{
			move.setAction(ActionType::CLEAR_AND_SELECT);
			move.setLeft(0);
			move.setRight(center.first);
			move.setTop(0);
			move.setBottom(center.second);
			move.setVehicleType(type);
		} });
		mMacroConditionalQueue.push_back({ passFunc , createGroupFromSelected });

		mMacroConditionalQueue.push_back({ passFunc , VALFHDR
		{
			move.setAction(ActionType::CLEAR_AND_SELECT);
			move.setLeft(center.first);
			move.setRight(1024);
			move.setTop(0);
			move.setBottom(center.second);
			move.setVehicleType(type);
		} });
		mMacroConditionalQueue.push_back({ passFunc , createGroupFromSelected });
	};

	create4Groups(fighterCenter, VehicleType::FIGHTER, true);

	mMacroConditionalQueue.push_back({passFunc,
	VALFHDR
	{
		unlockMacroInterruptions(); // !!!!!
	} });
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
			if (mayBeInterrupted && (!moved || mThisGroupActedTimes >= 3))
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
}
