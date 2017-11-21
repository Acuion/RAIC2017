#include "MyFormationBruteforcer.h"
#include <queue>
#include <map>

void MyFormationBruteforcer::buildPathToFormation()
{
	vector<vector<xypoint>> rows = 
	{
		{ make_pair(0,0), make_pair(0,1), make_pair(0,2) },
		{ make_pair(1,0), make_pair(1,1), make_pair(1,2) },
		{ make_pair(2,0), make_pair(2,1), make_pair(2,2) },
	};
	vector<vector<xypoint>> cols =
	{
		{ make_pair(0,0), make_pair(1,0), make_pair(2,0) },
		{ make_pair(0,1), make_pair(1,1), make_pair(2,1) },
		{ make_pair(0,2), make_pair(1,2), make_pair(2,2) },
	};

	for (auto& x : rows)
		do
		{
			buildPathToPoints(x[0], x[1], x[2]);
		} while (next_permutation(x.begin(), x.end()));

	for (auto& x : cols)
		do
		{
			buildPathToPoints(x[0], x[1], x[2]);
		} while (next_permutation(x.begin(), x.end()));
}

vector<FormationStep> MyFormationBruteforcer::getFormationPath() const
{
	return mCurrentFormationPath;
}

vector<xypoint> MyFormationBruteforcer::getFormation() const
{
	return mFinalFormation;
}

MyFormationBruteforcer::MyFormationBruteforcer(const xypoint tankStartCell, const xypoint ifvStartCell, const xypoint arrvStartCell)
	: mPathIsEmpty(true)
	, mTankStartCell(tankStartCell)
	, mIfvStartCell(ifvStartCell)
	, mArrvStartCell(arrvStartCell)
{
}

MyFormationBruteforcer::~MyFormationBruteforcer()
{
}

void MyFormationBruteforcer::buildPathToPoints(xypoint targetTankCell, xypoint targetIfvCell, xypoint targetArrvCell)
{
	using position = vector<xypoint>;
	queue<position> q;
	map<position, position> parent;

	const position start = { mTankStartCell, mIfvStartCell, mArrvStartCell };
	const position finish = { targetTankCell, targetIfvCell, targetArrvCell };

	const int dr[4] = {0,0,1,-1};
	const int dc[4] = {1,-1,0,0};

	q.push(start);
	parent[start] = start;

	while (!q.empty())
	{
		auto t = q.front();
		q.pop();

		if (t == finish)
		{
			vector<FormationStep> candidate;

			vector<position> moves;
			while (t != start)
			{
				moves.push_back(t);
				t = parent[t];
			}
			moves.push_back(start);
			reverse(moves.begin(), moves.end());

			for (int i = 1; i < moves.size(); ++i)
			{
				if (moves[i][0] != moves[i - 1][0])
				{
					candidate.push_back({model::VehicleType::TANK, moves[i - 1][0], moves[i][0] });
				}
				if (moves[i][1] != moves[i - 1][1])
				{
					candidate.push_back({ model::VehicleType::IFV, moves[i - 1][1], moves[i][1] });
				}
				if (moves[i][2] != moves[i - 1][2])
				{
					candidate.push_back({ model::VehicleType::ARRV, moves[i - 1][2], moves[i][2] });
				}
			}

			if (mPathIsEmpty || mCurrentFormationPath.size() > candidate.size())
			{
				mPathIsEmpty = false;
				mCurrentFormationPath = candidate;
				mFinalFormation = finish;
			}
			break;
		}

		const auto checkAndPush = [&](const xypoint& nxt, const position& nxtpos)
		{
			if (nxt.first >= 0 && nxt.second >= 0 && nxt.first < 3 && nxt.second < 3 && !parent.count(nxtpos))
			{
				bool unq = true;
				for (auto& x : t)
					if (x == nxt)
						unq = false;
				if (unq)
				{
					parent[nxtpos] = t;
					q.push(nxtpos);
				}
			}
		};

		for (int k = 0; k < 4; ++k)
		{
			xypoint nxt = t[0];
			nxt.first += dr[k];
			nxt.second += dc[k];
			auto nxtpos = { nxt, t[1], t[2] };
			checkAndPush(nxt, nxtpos);

			nxt = t[1];
			nxt.first += dr[k];
			nxt.second += dc[k];
			nxtpos = { t[0], nxt, t[2] };
			checkAndPush(nxt, nxtpos);

			nxt = t[2];
			nxt.first += dr[k];
			nxt.second += dc[k];
			nxtpos = { t[0], t[1], nxt };
			checkAndPush(nxt, nxtpos);
		}
	}
}
