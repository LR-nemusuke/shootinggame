
# include <Siv3D.hpp> // OpenSiv3D v0.4.3

enum class State
{
	Title,
	Game,
};

struct GameData
{
	int32 highScore = NULL;
};

using App = SceneManager<State, GameData>;

class  Title : public App::Scene {
public:
	Title(const InitData& init):IScene{ init } {}

	void update() override {
		startTransition.update(startButton.mouseOver());
		exitTransition.update(exitButton.mouseOver());
		if (startButton.mouseOver() || exitButton.mouseOver())
		{
			Cursor::RequestStyle(CursorStyle::Hand);
		}

		if (startButton.leftClicked())
		{
			changeScene(State::Game);
		}
		else if (exitButton.leftClicked())
		{
			System::Exit();
		}
	}

	void draw() const override
	{
		FontAsset(U"TitleFont")(U"Shooting")
			.drawAt(TextStyle::OutlineShadow(0.2, ColorF{ 0.2, 0.6, 0.2 }, Vec2{ 3, 3 }, ColorF{ 0.0, 0.5 }), 100, Vec2{ 400, 100 });

		startButton.draw(ColorF{ 1.0, startTransition.value() }).drawFrame(2);
		exitButton.draw(ColorF{ 1.0, exitTransition.value() }).drawFrame(2);

		FontAsset(U"Menu")(U"PLAY").drawAt(startButton.center(), Color{ 127,255,212 });
		FontAsset(U"Menu")(U"EXIT").drawAt(exitButton.center(), Color{ 127,255,212 });

		scoreFont(U"highscore : {}"_fmt(getData().highScore)).draw(Arg::bottomRight(780, 590));
	}

private:
	Rect startButton{ Arg::center = Scene::Center().movedBy(0, 50), 300, 60 };
	Transition startTransition{ 0.4s, 0.2s };

	Rect exitButton{ Arg::center = Scene::Center().movedBy(0, 150), 300, 60 };
	Transition exitTransition{ 0.4s, 0.2s };

	const Font scoreFont{ 30 };
};

class Game : public App::Scene {
public:
	Game(const InitData& init) :IScene{ init } {
		//initialize
		for (const auto& emoji : emojis) {
			targets << Texture(Emoji(emoji));
		}
	}

	void update() override {
		//change systemconst
		double deltaTime = Scene::DeltaTime();
		passedTime += deltaTime;
		targetSpawnTimer += deltaTime;
		playerShotCoolTimer += deltaTime;

		//generate target
		while (targetSpawnTimer > targetSpawnTime) {
			targetSpawnTimer -= targetSpawnTime;
			targetSpawnTime = Random(1.0, 2.5);
			targetInfo << GenerateTraget();
		}

		//player
		playerPosY = Cursor::Pos().y;
		if (playerPosY < 0)playerPosY = 0;
		if (playerPosY > 600)playerPosY = 600;
		playerPos = { 50, playerPosY };

		//player bullet generate
		if (playerShotCoolTimer >= playerShotCoolTime && MouseL.down()) {
			playerShotCoolTimer = 0.0;
			playerBullets << playerPos.moveBy(50, 0);
		}

		//player bullet move
		for (auto& playerBullet : playerBullets) {
			playerBullet.x += deltaTime * bulletSpeed;
		}
		//over frame player bullet destroy
		playerBullets.remove_if([](const Vec2& b)
			{
				return b.x > 850;
			}
		);

		//target move
		for (auto& target : targetInfo) {
			target.targetPos.x -= deltaTime * targetSpeed;
			const double h = 500 * Periodic::Jump0_1(target.time);
			target.targetPos.y = target.defoY - h;
		}

		//destroy if target over x==100
		targetInfo.remove_if([&](const TargetInfo& t)
			{
				if (t.targetPos.x < 100) {
					score += scores[t.targetType];
					lostScores << PutScore(t.targetPos, scores[t.targetType], passedTime);
					return true;
				}
				else {
					return false;
				}
			}
		);

		//player bullet hit target
		for (auto itTarget = targetInfo.begin(); itTarget != targetInfo.end();) {
			TargetInfo itTargetInfo = *itTarget;
			const Circle targetCircle(itTargetInfo.targetPos, 40);
			bool skip = false;

			for (auto itBullet = playerBullets.begin(); itBullet != playerBullets.end();) {
				if (targetCircle.intersects(*itBullet)) {
					int32 addscore = -scores[itTargetInfo.targetType];
					score += addscore;
					gotScores << PutScore(itTargetInfo.targetPos, addscore, passedTime);
					itTarget = targetInfo.erase(itTarget);
					playerBullets.erase(itBullet);
					skip = true;
					break;
				}
				++itBullet;
			}
			if (skip)continue;
			++itTarget;
		}

		//print time out
		gotScores.remove_if([&](const ScoreInfo& s)
			{
				return passedTime - s.printStartTime > 1.0;
			}
		);

		lostScores.remove_if([&](const ScoreInfo& s)
			{
				return passedTime - s.printStartTime > 1.0;
			}
		);

		//game finish
		if (passedTime > 60) {
			playerPos = Vec2(50, 550);
			targetInfo.clear();
			playerBullets.clear();
			gotScores.clear();
			targetSpawnTime = initialTargetSpawnTime;
			getData().highScore = Max(getData().highScore, score);
			score = 0;
			passedTime = 0.0;
			changeScene(State::Title);
		}
	}

	void draw() const override {
		//drawing
		//player draw
		player.resized(80).drawAt(playerPos);

		//player bullet draw
		for (const auto& playerBullet : playerBullets) {
			Circle(playerBullet, 8).draw(Palette::Orange);
		}

		//target draw
		for (const auto& target : targetInfo) {
			targets[target.targetType].resized(60).drawAt(target.targetPos);
		}

		//get score draw
		for (auto& s : gotScores) {
			scoreFont(U"{}pt"_fmt(s.score)).draw(s.socrePrintPos);
		}

		//lost score draw
		for (auto& s : lostScores) {
			scoreFont(U"{}pt"_fmt(s.score)).draw(s.socrePrintPos);
		}

		Line(100, 0, 100, 600).draw(3, Palette::Green);

		//score draw
		font(U"{}s"_fmt((int32)passedTime)).draw(Arg::bottomRight(780, 560));
		font(U"{}"_fmt(score)).draw(Arg::bottomRight(780, 590));
	}

private:

	Vec2 GenerateTargetPos() {
		double x = Random(850, 950);
		double y = 550;
		Vec2 res = { x, y };
		return res;
	}

	int32 GenerateTargetType() {
		return Random(3);
	}

	struct TargetInfo {
		Vec2 targetPos;
		double defoY;
		double targetType;
		Duration time;
	};

	TargetInfo GenerateTraget() {
		TargetInfo res;
		res.targetPos = { GenerateTargetPos() };
		res.defoY = res.targetPos.y;
		res.targetType = GenerateTargetType();
		res.time = (Duration)Random(1.0, 3.0);
		return res;
	}

	struct ScoreInfo {
		int32 score;
		Vec2 socrePrintPos;
		double printStartTime;
	};

	ScoreInfo PutScore(Vec2 pos, int32 s, double time) {
		ScoreInfo res;
		res.socrePrintPos = pos;
		res.score = s;
		res.printStartTime = time;
		return res;
	}

	const Font font{ 30 };
	const Font scoreFont{ 30 };

	//player texture
	const Texture player{ Emoji(U"🏹") };
	//target texture
	const Array<String> emojis = { U"😆" ,U"😢",U"😭",U"👿" };
	Array<Texture> targets;

	//target info
	Array<TargetInfo> targetInfo;

	//player bullet
	Array<Vec2> playerBullets;

	//target speed
	const double targetSpeed = 100.0;
	//player bullet speed
	const double bulletSpeed = 200.0;

	//target generate time
	double initialTargetSpawnTime = 2.0;
	double targetSpawnTime = initialTargetSpawnTime;
	double targetSpawnTimer = 0.0;

	//player shot time
	const double playerShotCoolTime = 0.8;
	double playerShotCoolTimer = 0.0;

	//time
	double passedTime = 0.0;

	//score
	int32 score = 0;
	//scores = { happy, pien, sad, angry }
	Array<int32> scores = { 50, -15, -30, -50 };
	//print score array
	Array<ScoreInfo> gotScores, lostScores;

	int32 playerPosY = 0;
	Vec2 playerPos = { 50, playerPosY };

};

void Main()
{
	Scene::SetBackground(ColorF{ 0.2,0.2,0.2,0.1 });
	FontAsset::Register(U"TitleFont", FontMethod::MSDF, 50, U"example/font/RocknRoll/RocknRollOne-Regular.ttf");
	FontAsset(U"TitleFont").setBufferThickness(4);
	FontAsset::Register(U"Menu", FontMethod::MSDF, 40, Typeface::Medium);
	FontAsset::Register(U"GameScore", 30, Typeface::Light);

	App manager;
	manager.add<Title>(State::Title);
	manager.add<Game>(State::Game);

	while (System::Update())
	{
		if (not manager.update())
		{
			break;
		}
	}
}
