#include <Kunlaboro/EntitySystem.hpp>
#include <Kunlaboro/Component.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#pragma once

namespace sf { class RenderTarget; }
class InputManager;
class SettingsManager;

class IState : public Kunlaboro::Component
{
public:
    IState(const std::string&);

    void addedToEntity();

    virtual void update(float dt) = 0;
    virtual void draw(sf::RenderTarget& target) = 0;
    virtual void drawUi(sf::RenderTarget& target) = 0;

private:
    inline void update(const Kunlaboro::Message& msg) { update(boost::any_cast<float>(msg.payload));             }
    inline void draw(const Kunlaboro::Message& msg)   { draw  (*boost::any_cast<sf::RenderTarget*>(msg.payload)); }
    inline void drawUi(const Kunlaboro::Message& msg) { drawUi(*boost::any_cast<sf::RenderTarget*>(msg.payload)); }
};

class StateManager
{
public:
    StateManager();
    ~StateManager();

    void setInput(InputManager&);
    void setSettings(SettingsManager&);

    void update(float);

    void draw(sf::RenderTarget&);
    void drawUi(sf::RenderTarget&);

    Kunlaboro::EntitySystem& getSystem();

private:
    class Internals : public Kunlaboro::Component
    {
    public:
        Internals(StateManager&);

        void addedToEntity();

        StateManager& mState;
    };

    Kunlaboro::EntitySystem mSystem;
    InputManager* mInput;
    SettingsManager* mSettings;

    Kunlaboro::RequestId mRUpdate, mRDraw, mRDrawUi;

    friend class Internals;
};