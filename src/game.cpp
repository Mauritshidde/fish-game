#include "game.h"

Game::Game(int screenWidth, int screenHeight, int columnCount, int rowCount)
{
    // generate map using mapSize
    Vector2 startingPosition = {screenWidth /4, screenHeight / 2}; // map generation has to give starting position, which is base position 

    tileTextures = {
        {"locked", LoadTexture("sprites/resources/BlankTile.png")},
        {"sea", LoadTexture("sprites/resources/BlankTile.png")},
        {"food", LoadTexture("sprites/resources/AlgenTile.png")},
        {"coral", LoadTexture("sprites/resources/CoralTile.png")},
        {"training", LoadTexture("sprites/buildings/TrainingTile.png")},
        {"castleV1", LoadTexture("sprites/castle/CastleTileLVL1.png")},
        {"castleV2", LoadTexture("sprites/castle/CastleTileLVL2.png")},
        {"castleV3", LoadTexture("sprites/castle/CastleTileLVL3.png")},
        {"castleV4", LoadTexture("sprites/castle/CastleTileLVL4.png")},
        {"castleV5", LoadTexture("sprites/castle/CastleTileLVL5.png")}
    };

    unitTextures = {
        {"warrior1LVL1", LoadTexture("sprites/units/Battlefish.png")},
        {"warrior2LVL1", LoadTexture("sprites/units/BattlefishRed.png")},

        {"warrior1LVL2", LoadTexture("sprites/units/BattleHorse.png")},
        {"warrior2LVL2", LoadTexture("sprites/units/BattleHorseRed.png")},

        {"warrior1LVL3", LoadTexture("sprites/units/Angler.png")},
        {"warrior2LVL3", LoadTexture("sprites/units/AnglerRed.png")},

        {"warrior1LVL4", LoadTexture("sprites/units/BattleHorse.png")},
        {"warrior2LVL4", LoadTexture("sprites/units/BattleHorseRed.png")},

        {"warrior1LVL5", LoadTexture("sprites/units/SharkRegular.png")},
        {"warrior2LVL5", LoadTexture("sprites/units/SharkRed.png")}
    };
    
    castleTypes = {"castleV1", "castleV2", "castleV3", "castleV4", "castleV5"};

    tileHighLiteWhite = LoadTexture("sprites/UI-elements/hexHighlight.png");
    tileHighLiteRed = LoadTexture("sprites/UI-elements/hexRedHighlight.png");
    

    overlay = Overlay(screenWidth, screenHeight, tileTextures);
    map = Map(rowCount, columnCount, tileTextures);
    player = Player(startingPosition, screenWidth, screenHeight, &map, &tileHighLiteWhite, unitTextures);
    wave = Wave2(&map, &player.camera, &tileHighLiteWhite, unitTextures);

    gameTime = 0;
    waveCount = 0;
    score = 0;

    isCastleMenu = false;
    isTrainingMenu = false;
    
    int foodTileCost = player.getTileCost("food");
    int coralTileCost = player.getTileCost("coral");
    int trainingTileCost = player.getTileCost("training");
    overlay.setTileTypeCosts(foodTileCost, coralTileCost, trainingTileCost);

    song = LoadMusicStream("music/GuitarSong.mp3");
}

Game::~Game()
{
    std::vector<std::string> unloadTileTextures = {"locked", "sea", "food", "coral", "training", "castleV1", "castleV2", "castleV3", "castleV4", "castleV5"};

    for (int i=0; i < unloadTileTextures.size(); i++) {
        UnloadTexture(tileTextures[unloadTileTextures.at(i)]);
    }
    
    std::vector<std::string> unloadUnitTextures = {"warrior1LVL1", "warrior2LVL1"};

    for (int i=0; i < unloadUnitTextures.size(); i++) {
        UnloadTexture(unitTextures[unloadUnitTextures.at(i)]);
    }

    UnloadTexture(tileHighLiteWhite);
    UnloadTexture(tileHighLiteRed);
}

void Game::Update(double dt)
{
    bool isMouseOnOverlay = overlay.isMouseOnOverlay(); // check if mouse is on overlay so it can be used for player aswell

    for (int i=0; i < surroundingCenter.size(); i++) {
        if (surroundingCenter.at(i)->isUnitOnTile) {
            if (surroundingCenter.at(i)->unitOnTile->owner != "player") {
                    player.castleHealth -= surroundingCenter.at(i)->unitOnTile->attackDamage * dt;
            }
        }
    }

    if (player.castleHealth <= 0) {
        gameRunning = false;
    }

    if(IsMouseButtonPressed(0)) { // makes build mode and overlay selction work
        
        Vector2 worldMousePos = GetScreenToWorld2D(GetMousePosition(), player.camera);
        Vector2 coord = map.worldPosToGridPos(worldMousePos);
        if(overlay.isBuildMode) {
            if(isMouseOnOverlay) {
                int buildTile = overlay.mouseOnBuildTile();
                if(buildTile != -1) {
                    overlay.selectBuildTile(buildTile);
                }
                // do stuff with overlay
            } else if (!isCastleMenu && !isTrainingMenu) {
                std::string buildTileName = overlay.getBuildTileName();
                if(buildTileName != "" && map.isSurrounded(coord) && map.isTileAvailable(coord, buildTileName)) {
                    bool isBought = player.buyTile(buildTileName);
                    if(isBought) {
                        map.changeTileType(coord, buildTileName);
                        int foodTileCost = player.getTileCost("food");
                        int coralTileCost = player.getTileCost("coral");
                        int trainingTileCost = player.getTileCost("training");
                        overlay.setTileTypeCosts(foodTileCost, coralTileCost, trainingTileCost);
                    } else {
                        noMoneyMsgCountDown = 1.0;
                        // DrawTextEx(GetFontDefault(), "NOT ENOUGH MONEY", (Vector2){GetScreenWidth() / 2, GetScreenHeight() / 2}, 50, 10, DARKGRAY);
                        // not enough money
                    }
                }
            }
        }

        bool isMouseOnCastle = (std::find(castleTypes.begin(), castleTypes.end(), map.getTileType(coord)) != castleTypes.end());
        if(isMouseOnCastle) {
            isCastleMenu = true;
        }

        if(map.getTileType(coord) == "training" && overlay.getBuildTileName() == "") {
            overlay.trainingCooldown = map.getTile(coord)->trainingCooldown;
            isTrainingMenu = true;
            trainingTileLocation = coord;
            selectedTrainingTile = map.getTile(coord);
        }

    }

    if(isCastleMenu) {
        if(IsKeyPressed(KEY_C)) {
            isCastleMenu = false;
        } else if (IsKeyPressed(KEY_L)) {
            bool isBought = player.buyCastleUpgrade();
            if(isBought) {
                isCastleMenu = false;
                int castleLvl = player.getCastleLvl();
                map.changeTileType({8, 8}, castleTypes.at(castleLvl - 1));
            } else {
                noMoneyMsgCountDown = 1.0;
            }
        }
    } else if (isTrainingMenu) {
        // create unit
        Vector2 worldMousePos = GetScreenToWorld2D(GetMousePosition(), player.camera);
        Vector2 coord = map.worldPosToGridPos(worldMousePos);

        if (!selectedTrainingTile->isTraining && !map.getTile(coord)->isUnitOnTile) {
            if (IsKeyPressed(KEY_ONE)) {
                if(player.getFoodAmount() >= 10) {
                    player.addFoodAmount(-10);
                    selectedTrainingTile->trainingCooldown = 10;
                    selectedTrainingTile->isTraining = true;
                    player.playerUnits.createUnit(trainingTileLocation, &player.camera, 1, 1);
                    isTrainingMenu = false;
                } else {
                    noMoneyMsgCountDown = 1.0;
                }
            } else if (IsKeyPressed(KEY_TWO) && player.getCastleLvl() > 1) {
                if(player.getFoodAmount() >= 20) {
                    player.addFoodAmount(-20);
                    selectedTrainingTile->trainingCooldown = 13;
                    selectedTrainingTile->isTraining = true;
                    player.playerUnits.createUnit(trainingTileLocation, &player.camera, 2, 1);
                    isTrainingMenu = false;
                } else {
                    noMoneyMsgCountDown = 1.0;
                }
            } else if (IsKeyPressed(KEY_THREE) && player.getCastleLvl() > 2) {
                if(player.getFoodAmount() >= 40) {
                    player.addFoodAmount(-40);
                    selectedTrainingTile->trainingCooldown = 16;
                    selectedTrainingTile->isTraining = true;
                    player.playerUnits.createUnit(trainingTileLocation, &player.camera, 3, 1);
                    isTrainingMenu = false;
                } else {
                    noMoneyMsgCountDown = 1.0;
                }
            } else if (IsKeyPressed(KEY_FOUR) && player.getCastleLvl() > 3) {
                if(player.getFoodAmount() >= 80) {
                    player.addFoodAmount(-80);
                    selectedTrainingTile->trainingCooldown = 20;
                    selectedTrainingTile->isTraining = true;
                    player.playerUnits.createUnit(trainingTileLocation, &player.camera, 4, 1);
                    isTrainingMenu = false;
                } else {
                    noMoneyMsgCountDown = 1.0;
                }
            } else if (IsKeyPressed(KEY_FIVE) && player.getCastleLvl() > 4) {
                if(player.getFoodAmount() >= 160) {
                    player.addFoodAmount(-160);
                    selectedTrainingTile->trainingCooldown = 24;
                    selectedTrainingTile->isTraining = true;
                    player.playerUnits.createUnit(trainingTileLocation, &player.camera, 5, 1);
                    isTrainingMenu = false;
                } else {
                    noMoneyMsgCountDown = 1.0;
                }
            } else if (IsKeyPressed(KEY_C)) {
                isTrainingMenu = false;
            }
        } else {
            if (IsKeyPressed(KEY_C)) {
                isTrainingMenu = false;
            }
        }
    }

    if(noMoneyMsgCountDown > 0.0) {
        noMoneyMsgCountDown -= dt;
    }

    overlay.updateCooldown(dt);
    bool dontMove = false;
    if(overlay.getBuildTileName() == "") {
        dontMove = true;
    } 
    player.Update(dt, overlay.selectedBuildTile, dontMove); // update all the objects that are in player
    wave.Update(dt);
    map.Update(dt);

    MusicPlayer(); // play the song 
}

void Game::MusicPlayer() 
{
    if (!IsMusicStreamPlaying(song)) { // start song if not already playing
        PlayMusicStream(song);
    } else {
        UpdateMusicStream(song);
    }
}

void Game::Render()
{
    BeginDrawing();
        ClearBackground(BLACK);
        Vector2 worldMousePos = GetScreenToWorld2D(GetMousePosition(), player.camera); // dit voor screen pos naar world pos
        Vector2 coord = map.worldPosToGridPos(worldMousePos);
        BeginMode2D(player.camera);
            map.draw(); // draw the tiles

            player.Render(); // draw player units
            wave.Render();

            if(!overlay.isMouseOnOverlay() && overlay.isBuildMode && !isCastleMenu) {
                std::string buildTileName = overlay.getBuildTileName();
                if(buildTileName != "") {
                    map.drawGhostTile(coord, buildTileName, map.isSurrounded(coord));
                }
            }

        EndMode2D();
        
        if(isCastleMenu) {
            overlay.drawCastleMenu(player.getCastleLvl());
        } else if (isTrainingMenu && overlay.getBuildTileName() == "") {
            overlay.drawTrainingMenu(player.getCastleLvl());
        }

            std::cout << isTrainingMenu ? "Training" : "Not Training";
            std::cout << "name: " << overlay.getBuildTileName() << std::endl;

        if(noMoneyMsgCountDown > 0) {
            Vector2 textDimentions = MeasureTextEx(GetFontDefault(), "NOT ENOUGH MONEY", 50, 10);
            DrawTextEx(GetFontDefault(), "NOT ENOUGH MONEY", (Vector2){(GetScreenWidth() - textDimentions.x) / 2, (GetScreenHeight() - textDimentions.y) / 2 + 50}, 50, 10, RED);
        }
        // DrawText(TextFormat("coord x: %d", int(coord.x)), 100, 100, 10, BLACK);
        // DrawText(TextFormat("coord y: %d", int(coord.y)), 200, 100, 10, BLACK);
        // DrawText(TextFormat("coord z: %d", int(coord.z)), 300, 100, 10, BLACK);

        // ui draw functions that should not move here
        int food = player.getFoodAmount();
        int coral = player.getCoralAmount();
        int time = int(gameTime);
        int timeUntilNextWave = round(wave.timeUntilNextWave);
        overlay.drawInventory(food, coral, score, time, wave.waveCount, timeUntilNextWave);

        overlay.drawBuildMode();
    EndDrawing();
}

void Game::Start() {
    gameRunning = true;
    Vector2 center = {int(map.cols/2), int(map.rows/2)};
    mapCenter = map.getTile(center);
    std::vector<Vector2> surroundingCoords = map.getSurroundingCoords(center);

    for (int i=0; i < surroundingCoords.size(); i++) {
        surroundingCenter.push_back(map.getTile(surroundingCoords.at(i)));
    }

    player.Start(center);
    wave.Start();
}

void Game::run() // start the game loop
{
    double dt;

    Start();
    
    while (gameRunning && !WindowShouldClose())
    {
        dt = GetFrameTime();
        Update(dt);
        Render();

        gameTime += dt;
        // If quit go to main menu
        // When esc press open menu for settings, save, load, continue and exit
    }

    gameOverScreen = GameOver(GetScreenWidth(), GetScreenHeight());

    while (!gameOverScreen.buttonPressed)
    {
        gameOverScreen.Update(GetScreenWidth(), GetScreenHeight());
        gameOverScreen.Draw(GetScreenWidth(), GetScreenHeight(), gameTime);
    }
}