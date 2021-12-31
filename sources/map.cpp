#include "map.h"

Map::Map(sint_16 aN, sint_16 aM) :
	mField(aN + 2, std::vector<Object*>(aM + 2, NULL)),
	mFoodtCounter(0),
	mPoisonCounter(0),
	mWallCounter(0)
{
	for (auto& i : mField)
	{
		for (auto& j : i)
		{
			j = new Object(Object::ObjectType::VOID);
		}
	}

	for (sint_16 i = 0; i < mField.size(); ++i)
	{
		setNewObject(Object::ObjectType::WALL, Pair<sint_16>(i, 0));
		setNewObject(Object::ObjectType::WALL, Pair<sint_16>(i, mField[0].size() - 1));
	}

	for (sint_16 i = 0; i < mField[0].size(); ++i)
	{
		setNewObject(Object::ObjectType::WALL, Pair<sint_16>(0, i));
		setNewObject(Object::ObjectType::WALL, Pair<sint_16>(mField.size() - 1, i));
	}

	regenerateField();
	createObjects(BOTS_START_COUNT, Object::ObjectType::BOT);
	getBotsCoordinates();
}

const std::vector<std::vector<Object*>>&
Map::getPresentation()
{
	return mField;
}

void
Map::setNewObject
(
	Object::ObjectType aType,
	Pair<sint_16> aCoord
)
{
	switch (aType)
	{
	case Object::VOID:
		setExictingObject (new Object(Object::ObjectType::VOID), aCoord);
		break;
	case Object::BOT:
		setExictingObject(new Bot(), aCoord);
		break;
	case Object::FOOD:
		setExictingObject(new Object(Object::ObjectType::FOOD), aCoord);
		break;
	case Object::POISON:
		setExictingObject(new Object(Object::ObjectType::POISON), aCoord);
		break;
	case Object::WALL:
		setExictingObject(new Object(Object::ObjectType::WALL), aCoord);
		break;
	default:
		setExictingObject(new Object(Object::ObjectType::NUN), aCoord);
		break;
	}
}

void
Map::setExictingObject
(
	Object* aObjectPtr,
	Pair<sint_16> aCoord
)
{
	delete(mField[aCoord.x][aCoord.y]);
	mField[aCoord.x][aCoord.y] = aObjectPtr;
}

bool
Map::trySetNewObject
(
	Object::ObjectType aType,
	Pair<sint_16> aCoord
)
{
	//if (mField[aCoord.x][aCoord.y]->getType() != aType)
	if (mField[aCoord.x][aCoord.y]->getType() == Object::ObjectType::VOID)
	{
		setNewObject(aType, aCoord);
		return true;
	}
	return false;
}

void 
Map::createObjects
(
	sint_16 aLimit,
	Object::ObjectType aType
)
{
	for (sint_16 cnt = 0; cnt < aLimit; )
	{
		Pair<sint_16> p(rand() % mField.size(), rand() % mField[0].size());
		if (trySetNewObject(aType, p)) ++cnt;
	}
}

void
Map::makeTurn()
{
	uint_16 count = mBotsCoord.size();
	for (uint_16 i = 0; i < count; ++i)
	{
		//std::cout << "  i : " << int(i) << "\n";
		Pair<sint_16> cur = mBotsCoord.front();
		mBotsCoord.pop();

		Object::ObjectType argument = Object::ObjectType::VOID;
		for (uint_8 j = 0; j < 8; ++j)
		{
			//std::cout << "     j : " << int(j) << "\n";
			Bot* botPtr = static_cast<Bot*>(mField[cur.x][cur.y]);
			Bot::Action action = botPtr->makeAction(argument);

			const Direction& dir = botPtr->getDirection();
			Pair<sint_16> next(cur.x + dir.getX(), cur.y + dir.getY());
			Object::ObjectType type = mField[next.x][next.y]->getType();
			switch (action)
			{
			case Bot::Action::NUN:
				std::cout << "bot action error\n";
				break;
			case Bot::Action::VOID:
				break;
			case Bot::Action::GO:
				if (type == Object::ObjectType::FOOD)
				{
					botPtr->feed(0.5);
					--mFoodtCounter;
				}
				else if (type == Object::ObjectType::POISON)
				{
					botPtr->poison(0.5);
					--mPoisonCounter;
				}

				if (type != Object::ObjectType::WALL && type != Object::ObjectType::BOT)
				{
					setExictingObject(botPtr, next);
					mField[cur.x][cur.y] = new Object(Object::ObjectType::VOID);
					cur = next;
				}
				j = 100;
				break;
			case Bot::Action::EAT:
				if (type == Object::ObjectType::FOOD)
				{
					botPtr->feed(1);
					--mFoodtCounter;
					setNewObject(Object::ObjectType::VOID, next);
				}
				else if (type == Object::ObjectType::POISON)
				{
					botPtr->poison(1);
					--mPoisonCounter;
					setNewObject(Object::ObjectType::VOID, next);
				}
				j = 100;
				break;
			case Bot::Action::CONVERT:
				if (type == Object::ObjectType::POISON)
				{
					--mPoisonCounter;
					++mFoodtCounter;
					setNewObject(Object::ObjectType::FOOD, next);
				}
				break;
			case Bot::Action::LOOK:
				argument = mField[cur.x + dir.getX()][cur.y + dir.getY()]->getType();
				break;
			}
		}
		if (!static_cast<Bot*>(mField[cur.x][cur.y])->aging())
		{
			setNewObject(Object::ObjectType::VOID, cur);
		}
		else
		{
			mBotsCoord.push(cur);
		}
	}

	createObjects(FOOD_START_COUNT - mFoodtCounter, Object::ObjectType::FOOD);
	createObjects(POISON_START_COUNT - mPoisonCounter, Object::ObjectType::POISON);

	mFoodtCounter = FOOD_START_COUNT;
	mPoisonCounter = POISON_START_COUNT;
}

bool 
Map::needToEvolve()
{
	return mBotsCoord.size() <= BOT_DOWN_LIMIT;
}

void 
Map::evolve()
{
	std::vector<Bot*> bots;

	while (!mBotsCoord.empty())
	{
		Pair<sint_16> cur = mBotsCoord.front();
		mBotsCoord.pop();
		bots.push_back(static_cast<Bot*>(mField[cur.x][cur.y]));
		bots.back()->reset();
		mField[cur.x][cur.y] = new Object(Object::ObjectType::VOID);
	}

	while (bots.size() < BOT_DOWN_LIMIT)
	{
		bots.push_back(new Bot());
	}

	regenerateField();

	for(auto& i : bots)
	{
		Pair<sint_16> new_coord;
		do
		{
			new_coord = Pair<sint_16>(rand() % mField.size(), rand() % mField[0].size());
		} while (mField[new_coord.x][new_coord.y]->getType() != Object::ObjectType::VOID);
		setExictingObject(i, new_coord);
	}

	for (uint_8 i = 0; i < BOT_MULTIPLY_COUNT; ++i)
	{
		for (auto& j : bots)
		{
			Pair<sint_16> new_coord;
			do
			{
				new_coord = Pair<sint_16>(rand() % mField.size(), rand() % mField[0].size());
			} while (mField[new_coord.x][new_coord.y]->getType() != Object::ObjectType::VOID);

			Bot* new_bot = new Bot(*j);
			new_bot->evolve((i - 1) < 0 ? 0 : (i - 1));
			setExictingObject(static_cast<Object*> (new_bot), new_coord);
		}
	}

	getBotsCoordinates();
}

void 
Map::regenerateField()
{
	for (sint_16 i = 1; i < mField.size() - 1; ++i)
	{
		for (sint_16 j = 1; j < mField[0].size() - 1; ++j)
		{
			delete(mField[i][j]);
			mField[i][j] = new Object(Object::ObjectType::VOID);
		}
	}

	createObjects(FOOD_START_COUNT, Object::ObjectType::FOOD);
	createObjects(POISON_START_COUNT, Object::ObjectType::POISON);
	createObjects(WALL_START_COUNT, Object::ObjectType::WALL);

	mFoodtCounter = FOOD_START_COUNT;
	mPoisonCounter = POISON_START_COUNT;
	mWallCounter = WALL_START_COUNT;
}

void
Map::getBotsCoordinates()
{
	while (!mBotsCoord.empty())
	{
		std::cout << "aueue error\n";
		mBotsCoord.pop();
	}
	for (sint_16 i = 0; i < mField.size(); ++i)
	{
		for (sint_16 j = 0; j < mField[0].size(); ++j)
		{
			if (mField[i][j]->getType() == Object::ObjectType::BOT)
			{
				mBotsCoord.push({ i, j });
			}
		}
	}
}

/*

Pair<sint_16> cur_coord = mBotsCoord.front();
		mBotsCoord.pop();
		Pair<sint_16> new_coord = cur_coord;
		do
		{
			new_coord = Pair<sint_16>(rand() % mField.size(), rand() % mField[0].size());
		} while (new_coord == cur_coord &&
			mField[new_coord.x][new_coord.y]->getType!= );
*/