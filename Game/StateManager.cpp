#include "StateManager.hpp"
#include "InputManager.hpp"
#include "SettingsManager.hpp"
#include "Telemetry.hpp"
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

StateManager::StateManager() : mInput(NULL), mSettings(NULL), mGameView(NULL), mUiView(NULL)
{
    mRUpdate = mSystem.getMessageRequestId(Kunlaboro::Reason_Message, "LD26.Update");
    mRDraw   = mSystem.getMessageRequestId(Kunlaboro::Reason_Message, "LD26.Draw"  );
    mRDrawUi = mSystem.getMessageRequestId(Kunlaboro::Reason_Message, "LD26.DrawUi");

    mSystem.registerComponent("StateManager.Internals", [this](){ return new StateManager::Internals(*this); });
    Kunlaboro::EntityId id = mSystem.createEntity();
    mSystem.addComponent(id, "StateManager.Internals");

    mGlobalFont.loadFromFile("VeraMono.ttf");
}

StateManager::~StateManager()
{
}

void StateManager::setViews(sf::View& game, sf::View& ui)
{
    mGameView = &game;
    mUiView = &ui;
}
void StateManager::setInput(InputManager& in)
{
    mInput = &in;
}
void StateManager::setSettings(SettingsManager& in)
{
    mSettings = &in;
}
void StateManager::setSound(SoundManager& in)
{
    mSound = &in;
}
void StateManager::setTelemetry(Telemetry& in)
{
    mTelem = &in;
}

Kunlaboro::EntitySystem& StateManager::getSystem() { return mSystem; }

void StateManager::update(float dt)
{
    Kunlaboro::Message msg(Kunlaboro::Type_Message, NULL, std::min(dt, 0.5f));
    mSystem.sendGlobalMessage(mRUpdate, msg);
}
void StateManager::draw(sf::RenderTarget& target)
{
    Kunlaboro::Message msg(Kunlaboro::Type_Message, NULL, &target);
    mSystem.sendGlobalMessage(mRDraw, msg);
}

/// The evil global variables of doom!
/// TODO: Stop them from taking over the world
int frames = 0;
sf::Text prof;

void StateManager::drawUi(sf::RenderTarget& target)
{
    Kunlaboro::Message msg(Kunlaboro::Type_Message, NULL, &target);
    mSystem.sendGlobalMessage(mRDrawUi, msg);

    if (frames++ > 10)
    {
        prof.setFont(mGlobalFont);
        prof.setCharacterSize(12);

        char tmp[256];
#ifdef WIN32
        sprintf_s(tmp, "Components: %d, entities: %d.\nFPS: %-4d (%.2f avg)\nDT:  %-4.2f (%.2f avg)", mSystem.numCom(), mSystem.numEnt(), mTelem->getFPS(), mTelem->getAverageFPS(), mTelem->getDT(), mTelem->getAverageDT());
#else
        sprintf(tmp, "Components: %d, entities: %d.\nFPS: %-4d (%.2f avg)\nDT:  %-4.2f (%.2f avg)", mSystem.numCom(), mSystem.numEnt(), mTelem->getFPS(), mTelem->getAverageFPS(), mTelem->getDT(), mTelem->getAverageDT());
#endif
        prof.setString(tmp);
    }

    target.draw(prof);
}

IState::IState(const std::string& name) : Kunlaboro::Component(name)
{
}

void IState::addedToEntity()
{
    requestMessage("LD26.Update", &IState::update);
    requestMessage("LD26.Draw",   &IState::draw);
    requestMessage("LD26.DrawUi", &IState::drawUi);

    changeRequestPriority("LD26.Update", -10);
    changeRequestPriority("LD26.Draw",   -10);
    changeRequestPriority("LD26.DrawUi", -10);

    setup();
}

void IState::setHandled()
{
    if (mCurMsg != NULL)
        mCurMsg->handled = true;
}

StateManager::Internals::Internals(StateManager& man) : Kunlaboro::Component("StateManager.Internals"), mState(man) { }
void StateManager::Internals::addedToEntity()
{
    StateManager* man = &mState;

    requestMessage("Get.Input",    [man](Kunlaboro::Message& msg){ msg.payload = man->mInput; msg.handled = true;    });
    requestMessage("Get.Settings", [man](Kunlaboro::Message& msg){ msg.payload = man->mSettings; msg.handled = true; });
    requestMessage("Get.Sounds",   [man](Kunlaboro::Message& msg){ msg.payload = man->mSound; msg.handled = true;    });
    requestMessage("Get.Font",     [man](Kunlaboro::Message& msg){ msg.payload = &man->mGlobalFont; msg.handled = true; });
    requestMessage("Get.GameView", [man](Kunlaboro::Message& msg){ msg.payload = man->mGameView; msg.handled = true; });
    requestMessage("Get.UiView",   [man](Kunlaboro::Message& msg){ msg.payload = man->mUiView; msg.handled = true;   });

    requestMessage("Now Playing",  [this](const Kunlaboro::Message& msg)
    {
        mCurrentSong = boost::any_cast<std::string>(msg.payload);
        mSongPopout = 0;
        mSongTimeout = 2;
    });
    requestMessage("LD26.Update",  [this](const Kunlaboro::Message& msg)
    {
        float dt = boost::any_cast<float>(msg.payload);

        if (mSongTimeout > 0)
            mSongTimeout = std::max(0.f, mSongTimeout - dt);

        if (!mCurrentSong.empty() && mSongTimeout > 0 && mSongPopout < 1)
            mSongPopout = std::min(1.f, mSongPopout + dt);
        else if (mSongTimeout == 0 && mSongPopout > 0)
            mSongPopout = std::max(0.f, mSongPopout - dt);
        else if (mSongTimeout == 0 && mSongPopout == 0 && !mCurrentSong.empty())
            mCurrentSong.clear();
    });
    changeRequestPriority("LD26.Update", -11);
    requestMessage("LD26.DrawUi",  [this](const Kunlaboro::Message& msg)
    {
        sf::RenderTarget& target = *boost::any_cast<sf::RenderTarget*>(msg.payload);

        sf::RectangleShape rect;
        rect.setSize(sf::Vector2f(512, 96));
        rect.setOrigin(rect.getSize()/2.f);
        rect.setFillColor(sf::Color::Black);
        rect.setOutlineColor(sf::Color::White);
        rect.setOutlineThickness(6.f);

        rect.setPosition(target.getSize().x + 262 - (512 * mSongPopout), target.getSize().y - 100);

        target.draw(rect);

        sf::Text playing("Now Playing:", mState.mGlobalFont);
        sf::FloatRect bounds = playing.getLocalBounds();
        playing.setPosition(rect.getPosition() - sf::Vector2f(350, 72) + sf::Vector2f(bounds.left + bounds.width/2, bounds.top + bounds.height/2));

        target.draw(playing);
        playing.setString(mCurrentSong);
        playing.move(0, bounds.height + 16);
        playing.setCharacterSize(20U);

        target.draw(playing);
    });
}