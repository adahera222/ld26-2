#include "MainClass.hpp"
#include "WarmupState.hpp"
#include "Components.hpp"
#include "PlayerInput.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include <iostream>
#include <list>

static const int MAX_BINDS = 4;

MainClass::MainClass(int argc, char** argv)
{
    std::list<std::string> stack;
    for (int i = 1; i < argc; i++)
        stack.push_front(argv[i]);

    mSettings.handleArgs(stack);

    {
        sf::Event ev;
        ev.type = sf::Event::KeyPressed;
        ev.key.code = sf::Keyboard::W;
        ev.key.alt = ev.key.control = ev.key.shift = false;
        
        mInput.addBind("Forward", ev);

        ev.key.code = sf::Keyboard::A;

        mInput.addBind("Left", ev);

        ev.key.code = sf::Keyboard::D;

        mInput.addBind("Right", ev);

        ev.key.code = sf::Keyboard::Space;

        mInput.addBind("Fire", ev);
    }

    mManager.setInput(mInput);
    mManager.setSettings(mSettings);
    mManager.setViews(mGame, mUi);

    Kunlaboro::EntitySystem& sys = mManager.getSystem();

    sys.registerComponent<Components::Physical>("Components.Physical");
    sys.registerComponent<Components::Inertia>("Components.Inertia");
    sys.registerComponent<Components::ShapeDrawable>("Components.ShapeDrawable");
    sys.registerComponent<Components::TexturedDrawable>("Components.TexturedDrawable");
    sys.registerComponent<Components::SpatialContainer>("Components.SpatialContainer");
    sys.registerComponent<PlayerInput>("PlayerInput");
    sys.registerComponent<GameState>("Warmup");
}

MainClass::~MainClass()
{

}

int MainClass::operator()()
{
    int currentBind = (mSettings.getBool("Bind") ? 0 : 10);

    sf::RenderWindow app;

    if (mSettings.getBool("Fullscreen"))
        app.create(sf::VideoMode::getDesktopMode(), "Ludumdare #26 Warmup", sf::Style::None);
    else
        app.create(sf::VideoMode(mSettings.getInt("Width"), mSettings.getInt("Height")), "Ludumdare #26 Warmup");

    app.setFramerateLimit(60);
    sf::Event ev;
    sf::Clock clock;

    {
        Kunlaboro::EntitySystem& sys = mManager.getSystem();

        Kunlaboro::EntityId id = sys.createEntity();
        sys.addComponent(id, "Warmup");
        sys.finalizeEntity(id);
    }

    mGame = app.getDefaultView();
    mUi = app.getDefaultView();
    {
        float aspect = (float)app.getSize().x/(float)app.getSize().y;
        mGame.setSize(1080*aspect,1080);

        sf::Vector2f tSize = (sf::Vector2f)app.getSize();
        mUi.setSize(tSize);
        mUi.setCenter(tSize/2.f);
    }

    while (app.isOpen())
    {
        float dt = clock.restart().asSeconds();

        while (app.pollEvent(ev))
        {
            mInput.handleEvent(ev);

            if (ev.type == sf::Event::Closed)
            {
                app.close();
            }
            else if (ev.type == sf::Event::Resized)
            {
                float aspect = (float)ev.size.width/(float)ev.size.height;
                mGame.setSize(sf::Vector2f(1080*aspect, 1080));

                sf::Vector2f tSize = (sf::Vector2f)app.getSize();
                mUi.setSize(tSize);
                mUi.setCenter(tSize/2.f);
            }
        }

        if (currentBind <= MAX_BINDS && !mInput.isBinding())
        {
            if (currentBind < MAX_BINDS)
                std::cout << "Press a key/button/axis you want to bind to ";

            std::string bind;
            switch(currentBind)
            {
            case 0:
                bind = "Forward"; break;
            case 1:
                bind = "Left"; break;
            case 2:
                bind = "Right"; break;
            case 3:
                bind = "Fire"; break;
            }

            if (bind.empty())
            {
                currentBind++;
                continue;
            }

            std::cout << bind << std::endl;

            mInput.startBind(bind);

            currentBind++;
        }
        else if (currentBind > MAX_BINDS)
        {
            mManager.update(dt);

            app.clear();

            app.setView(mGame);
            mManager.draw(app);

            app.setView(mUi);
            mManager.drawUi(app);

            app.display();
        }
    }

    return 0;
}