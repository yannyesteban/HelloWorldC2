//
// Created by yanny on 25/5/2019.
//

#include "DroidBlaster.h"
#include "Sound.h"
#include "Log.h"
#include <unistd.h>
static const int32_t SHIP_SIZE = 64;
static const int32_t SHIP_FRAME_1 = 0;
static const int32_t SHIP_FRAME_COUNT = 8;
static const float SHIP_ANIM_SPEED = 8.0f;

static const int32_t ASTEROID_COUNT = 16;
static const int32_t ASTEROID_SIZE = 64;
static const int32_t ASTEROID_FRAME_1 = 0;
static const int32_t ASTEROID_FRAME_COUNT = 16;
static const float ASTEROID_MIN_ANIM_SPEED = 8.0f;
static const float ASTEROID_ANIM_SPEED_RANGE = 16.0f;

static const int32_t STAR_COUNT = 50;

DroidBlaster::DroidBlaster(android_app* pApplication):
        mTimeManager(),
        mGraphicsManager(pApplication),
        mPhysicsManager(mTimeManager, mGraphicsManager),
        mSoundManager(pApplication),
        mEventLoop(pApplication, *this),

        mAsteroidTexture(pApplication, "asteroid.png"),
        mShipTexture(pApplication, "ship.png"),
        mStarTexture(pApplication, "star.png"),
        mBGM(pApplication, "bgm.mp3"),
        mCollisionSound(pApplication, "collision.mp3"),

        mAsteroids(pApplication, mTimeManager, mGraphicsManager,
                   mPhysicsManager),
        mShip(pApplication, mGraphicsManager, mSoundManager),
        mStarField(pApplication, mTimeManager, mGraphicsManager,
                   STAR_COUNT, mStarTexture),
        mSpriteBatch(mTimeManager, mGraphicsManager) {

    Log::info("Creating DroidBlaster");

    Sprite *shipGraphics = mSpriteBatch.registerSprite(mShipTexture, SHIP_SIZE, SHIP_SIZE);
    shipGraphics->setAnimation(SHIP_FRAME_1, SHIP_FRAME_COUNT, SHIP_ANIM_SPEED, true);
    Sound* collisionSound =
            mSoundManager.registerSound(mCollisionSound);
    mShip.registerShip(shipGraphics, collisionSound);

    // Creates asteroids.
    for (int32_t i = 0; i < ASTEROID_COUNT; ++i) {
        Sprite *asteroidGraphics = mSpriteBatch.registerSprite(
                mAsteroidTexture, ASTEROID_SIZE,
                ASTEROID_SIZE);
        float animSpeed = ASTEROID_MIN_ANIM_SPEED
                          + RAND(ASTEROID_ANIM_SPEED_RANGE);
        asteroidGraphics->setAnimation(ASTEROID_FRAME_1,
                                       ASTEROID_FRAME_COUNT, animSpeed, true);
        mAsteroids.registerAsteroid(
                asteroidGraphics->location, ASTEROID_SIZE,
                ASTEROID_SIZE);
    }
}
void DroidBlaster::run() {
    mEventLoop.run();
}
status DroidBlaster::onActivate() {
    Log::info("Activating DroidBlaster");
    if (mGraphicsManager.start() != STATUS_OK) return STATUS_KO;
    if (mSoundManager.start() != STATUS_OK) return STATUS_KO;

    mSoundManager.playBGM(mBGM);
// Initializes game objects.
    mAsteroids.initialize();
    mShip.initialize();
    mTimeManager.reset();
    return STATUS_OK;

    /*
    if (mGraphicsManager.start() != STATUS_OK)
        return STATUS_KO;

    mGraphicsManager.loadTexture(mAsteroidTexture);
    mGraphicsManager.loadTexture(mShipTexture);

    mAsteroids.initialize();
    mShip.initialize();
    mTimeManager.reset();
    return STATUS_OK;
     */
}
void DroidBlaster::onDeactivate() {
    Log::info("Deactivating DroidBlaster");
    mGraphicsManager.stop();
    mSoundManager.stop();
}
status DroidBlaster::onStep() {

    mTimeManager.update();
    mPhysicsManager.update();
    mAsteroids.update();
    Log::info("Starting step");
    return mGraphicsManager.update();


}
void DroidBlaster::onStart() {
    Log::info("onStart");
}


