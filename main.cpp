#include <iostream>
#include <Windows.h>
using namespace std;
#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <fstream>

//Finds the of a line between two points in radians
float getAngleInRadians(float x1, float y1, float x2, float y2) {
    float deltaX = x2 - x1;
    float deltaY = y2 - y1;
    return atan2(deltaY, deltaX);
}

//Finds the distance between two points
float distance(float x1, float y1, float x2, float y2) {
	return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
}

//Finds the distance between two points, wrapping around the edges of the window
float wrappedDistance(float x1, float y1, float x2, float y2, float width, float height) {
    float dx = abs(x2 - x1);
    dx = min(dx, width - dx);
    float dy = abs(y2 - y1);
    dy = min(dy, height - dy);
    return sqrt(dx * dx + dy * dy);
}

//Finds the angle of a line between two points, wrapping around the edges of the window
float wrappedAngle(float x1, float y1, float x2, float y2, float width, float height) {
    float dx = x2 - x1;
    if (abs(dx) > width / 2) dx = (dx > 0) ? dx - width : dx + width;

    float dy = y2 - y1;
    if (abs(dy) > height / 2) dy = (dy > 0) ? dy - height : dy + height;

    return atan2(dy, dx);
}

//Returns a color based on the type of particle
sf::Color color(int type) {
    switch (type) {
    case 0:
        return sf::Color(255, 0, 0);
    case 1:
        return sf::Color(0, 255, 0);
    case 2:
        return sf::Color(0, 0, 255);
    case 3:
        return sf::Color(255, 255, 0);
    case 4:
        return sf::Color(0, 255, 255);
    case 5:
        return sf::Color(255, 0, 255);
    case 6:
        return sf::Color(255, 100, 0);
    case 7:
        return sf::Color(0, 255, 100);
    case 8:
        return sf::Color(100, 0, 255);
    case 9:
        return sf::Color(255, 0, 100);
    case 10:
        return sf::Color(100, 255, 0);
    case 11:
        return sf::Color(0, 100, 255);
    default:
        return sf::Color(0, 0, 0);
    }
}

//Tests if heatVisions is enabled
bool heatVision = false;
bool outFocus = false;

//Draws a line between two points with a specified width and color
void drawLine(sf::RenderWindow& window, sf::Vector2f point1, sf::Vector2f point2, int lineWidth, sf::Color lineColor) {
    int x0 = point1.x;
    int y0 = point1.y;
    int x1 = point2.x;
    int y1 = point2.y;
    int dx = abs(x1 - x0);
    int sx, sy;
    sx = (x0 < x1) ? 1 : -1;
    int dy = -abs(y1 - y0);
    sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;
    while (true) {
        sf::RectangleShape rect(sf::Vector2f(lineWidth, lineWidth));
        rect.setFillColor(lineColor);
        rect.setPosition(sf::Vector2f(x0, y0));
        window.draw(rect);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

//The class for particles
class Particle
{
public:
    float x;
    float y;
    float xVelocity = 0;
	float yVelocity = 0;
	int type;
    int size;
    Particle(float x, float y, int type, int size) {
        this->x = x;
        this->y = y;
		this->type = type;
        this->size = size;
    }
    void draw(sf::RenderWindow &window) {
		sf::CircleShape circle(size);
        if (heatVision) {
            circle.setFillColor(sf::Color(255,255,255));
        }
        else {
            circle.setFillColor(color(type));
        }
        if (outFocus) {
            circle.setFillColor(sf::Color(circle.getFillColor().r, circle.getFillColor().g, circle.getFillColor().b, 50));
            circle.setScale({ 10,10 });
        }
        circle.setPosition({ x - size / 2, y - size / 2 });
		window.draw(circle);
    }
	void moveAngle(float angle, float speed) {
		angle = angle * 3.14159 / 180;
		xVelocity += cos(angle) * speed;
		yVelocity += sin(angle) * speed;
	}
    void moveRadians(float angle, float speed) {
        xVelocity += cos(angle) * speed;
        yVelocity += sin(angle) * speed;
    }
    void move() {
		x += xVelocity;
		y += yVelocity;
		xVelocity *= 0.9;
		yVelocity *= 0.9;
    }
};

//Checks if the mouse is clicking on a rectangle shape or text
bool click(sf::Vector2i window, sf::RectangleShape item) {
    return (sf::Mouse::getPosition().x - window.x > item.getPosition().x && sf::Mouse::getPosition().x - window.x < item.getPosition().x + item.getLocalBounds().size.x && sf::Mouse::getPosition().y - window.y > item.getPosition().y && sf::Mouse::getPosition().y - window.y < item.getPosition().y + item.getLocalBounds().size.y && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left));
}
bool click(sf::Vector2i window, sf::Text item) {
    return (sf::Mouse::getPosition().x - window.x > item.getPosition().x && sf::Mouse::getPosition().x - window.x < item.getPosition().x + item.getLocalBounds().size.x && sf::Mouse::getPosition().y - window.y > item.getPosition().y && sf::Mouse::getPosition().y - window.y < item.getPosition().y + (item.getLocalBounds().size.y * 2) && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left));
}

//List of particles
vector<Particle> particles = {};

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

	//String that keeps track of the current edit for the menu item of particles
    string currentEdit;

    //Loading fonts
    sf::Font font_parchment("PARCHM.ttf");
    sf::Font font_candara("cour.ttf");

	//Setting up the window and seeding the random number generator (for some reason)
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    srand(time(nullptr));
    sf::RenderWindow window(desktop, "Particle Life Simulator", sf::Style::None);
    sf::Clock clock;

	//Current selected particle type to create
    int type = 0;

	//Variables to control the pause state and mouse input
    bool paused = false;
    bool pauseWait = false;
    bool mouseWait = false;
    bool needHelp = true;

    //Loading saved settings
	ifstream file1("settings.txt");
    ifstream file2("types.txt");

	//Setting the icon for the window
	sf::Image icon("particle.png");
    window.setIcon(icon);

	//Reading settings from the file
    vector<int> settings;
    string line;

    if (file1.is_open()) {
        while (getline(file1, line)) {
            settings.push_back(stoi(line));
        }
        file1.close();
    }
    else {
        cerr << "Error: Could not open settings.txt" << endl;
        return 1;
    }

    //Creating random attractions between particles and getting them
    float attractionMatrix[12][12];
    for (int i = 0; i < 12; i++) {
        for (int j = 0; j < 12; j++) {
            attractionMatrix[i][j] = ((rand() % 201) - 100);
        }
    }

    //Creating particles
    for (int i = 0; i < settings[0]; i++) {
        particles.push_back(Particle(rand() % 1000, rand() % 700, rand() % settings[2], settings[1]));
    }

    //Getting what half to the distancing distance so as not to calculate it over and over
    float half = settings[4] / 2;

    //Creating menu UI
    sf::RectangleShape back({ (float)window.getSize().x * 3 / 4, (float)window.getSize().y * 3 / 4 });
    back.setPosition({ (float)window.getSize().x / 8, (float)window.getSize().y / 8 });
    back.setFillColor(sf::Color(20, 20, 20));
    sf::Text text_title(font_parchment, "Particle Life Simulation");
    text_title.setPosition({ back.getPosition().x + 10, back.getPosition().y + 10 });
    text_title.setCharacterSize(60);
    sf::Text text_label(font_candara, "Size:\nAmount:\nTypes:\n");
    text_label.setPosition({ back.getPosition().x + 20, back.getPosition().y + 100 });
    text_label.setCharacterSize(25);
    sf::RectangleShape deleteButton({ 100,30 });
    deleteButton.setPosition({ back.getPosition().x + 20, text_label.getPosition().y + text_label.getLocalBounds().size.y + 10 });
    deleteButton.setFillColor(sf::Color::Red);
    sf::RectangleShape resetButton({ 100,30 });
    resetButton.setPosition({ back.getPosition().x + 20, deleteButton.getPosition().y + deleteButton.getLocalBounds().size.y + 10 });
    resetButton.setFillColor(sf::Color::Green);
    sf::Text text_size(font_candara, to_string(settings[1]));
    text_size.setCharacterSize(25);
    text_size.setPosition({ text_label.getPosition().x + text_label.getLocalBounds().size.x + 10, text_label.getPosition().y });
    text_size.setString(to_string(settings[1]));
    text_size.setFillColor(sf::Color::White);
    sf::Text text_amount(font_candara, to_string(settings[1]));
    text_amount.setCharacterSize(25);
    text_amount.setPosition({ text_label.getPosition().x + text_label.getLocalBounds().size.x + 10, text_size.getPosition().y + text_size.getLocalBounds().size.y + 12});
    text_amount.setString(to_string(particles.size()));
    text_amount.setFillColor(sf::Color::White);
    sf::Text text_types(font_candara, to_string(settings[1]));
    text_types.setCharacterSize(25);
    text_types.setPosition({ text_label.getPosition().x + text_label.getLocalBounds().size.x + 10, text_amount.getPosition().y + text_amount.getLocalBounds().size.y + 12 });
    text_types.setString(to_string(settings[2]));
    text_types.setFillColor(sf::Color::White);
	sf::Text text_menuHelp(font_candara, "Press Tab to toggle menu.");
	text_menuHelp.setFillColor(sf::Color::White);
	text_menuHelp.setCharacterSize(20);
	text_menuHelp.setPosition({ back.getPosition().x + 20, back.getPosition().y + back.getLocalBounds().size.y - 50 });
    sf::Text text_moreHelp(font_candara, "Press green to randomize the attractions.\nPress red or c to clear the screen.\nPress the number keys to change the selected particle.\nClick to place particles\nShift click to continously place particles.\nPress s to turn on particle trails.\nPress b to blur the particles.");
    text_moreHelp.setFillColor(sf::Color::White);
    text_moreHelp.setCharacterSize(20);
    text_moreHelp.setPosition({ back.getPosition().x + 20, back.getPosition().y + back.getLocalBounds().size.y - 200 });

    bool sizeWait = true;

    bool menu = false;

    bool snake = false;

    while (window.isOpen())
    {
        float deltaTime = clock.restart().asSeconds();

        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
            if (event->is<sf::Event::FocusLost>())
                paused = true;
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
            {
                if (keyPressed->scancode == sf::Keyboard::Scan::Escape)
                    window.close();
                if (keyPressed->scancode == sf::Keyboard::Scan::Hyphen)
                    heatVision = !heatVision;
				if (keyPressed->scancode == sf::Keyboard::Scan::Tab)
					menu = !menu;
				if (keyPressed->scancode == sf::Keyboard::Scan::S) {
					snake = !snake;
				}
                if (keyPressed->scancode == sf::Keyboard::Scan::B) {
                    outFocus = !outFocus;
                }

                if (text_size.getFillColor() == sf::Color::Blue || text_amount.getFillColor() == sf::Color::Blue || text_types.getFillColor() == sf::Color::Blue) {
                    if (keyPressed->scancode == sf::Keyboard::Scan::Num1) {
                        currentEdit += "1";
                    }
					if (keyPressed->scancode == sf::Keyboard::Scan::Num2) {
						currentEdit += "2";
					}
                    if (keyPressed->scancode == sf::Keyboard::Scan::Num3) {
                        currentEdit += "3";
                    }
					if (keyPressed->scancode == sf::Keyboard::Scan::Num4) {
						currentEdit += "4";
					}
					if (keyPressed->scancode == sf::Keyboard::Scan::Num5) {
						currentEdit += "5";
					}
                    if (keyPressed->scancode == sf::Keyboard::Scan::Num6) {
                        currentEdit += "6";
                    }
                    if (keyPressed->scancode == sf::Keyboard::Scan::Num7) {
                        currentEdit += "7";
                    }
					if (keyPressed->scancode == sf::Keyboard::Scan::Num8) {
						currentEdit += "8";
					}
                    if (keyPressed->scancode == sf::Keyboard::Scan::Num9) {
                        currentEdit += "9";
                    }
                    if (keyPressed->scancode == sf::Keyboard::Scan::Num0) {
                        currentEdit += "0";
                    }
					if (keyPressed->scancode == sf::Keyboard::Scan::Backspace) {
						if (!currentEdit.empty()) {
							currentEdit.pop_back();
						}
					}
                }
            }
        }


        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num1)) {
            type = 1;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num2)) {
            type = 2;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num3)) {
            type = 3;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num4)) {
            type = 4;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num5)) {
            type = 5;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num6)) {
            type = 6;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num7)) {
            type = 7;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num8)) {
            type = 8;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num9)) {
            type = 9;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num0)) {
            type = 0;
        }

        if (!menu && !paused && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && sf::Mouse::getPosition().x - window.getPosition().x > 0 && sf::Mouse::getPosition().x - window.getPosition().x < window.getSize().x && sf::Mouse::getPosition().y - window.getPosition().y > 0 && sf::Mouse::getPosition().y - window.getPosition().y < window.getSize().y) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);
            if (type == 0) {
                particles.push_back(Particle(mousePos.x, mousePos.y, rand() % settings[2], settings[1]));
            }
            else {
                particles.push_back(Particle(mousePos.x, mousePos.y, type - 1, settings[1]));
            }
        }
        else if (!menu && !mouseWait && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && sf::Mouse::getPosition().x - window.getPosition().x > 0 && sf::Mouse::getPosition().x - window.getPosition().x < window.getSize().x && sf::Mouse::getPosition().y - window.getPosition().y > 0 && sf::Mouse::getPosition().y - window.getPosition().y < window.getSize().y) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);
            if (type == 0) {
                particles.push_back(Particle(mousePos.x, mousePos.y, rand() % settings[2], settings[1]));
            }
            else {
                particles.push_back(Particle(mousePos.x, mousePos.y, type - 1, settings[1]));
            }
            mouseWait = true;
        }
        else if (!sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
            mouseWait = false;
        }

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::C)) {
			particles.clear();
		}

        if (!paused) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) && !pauseWait) {
                paused = true;
                pauseWait = true;
            }
            int num = -1;
            int num2 = -1;
            //float oldX;
            //float oldY;
            for (auto& particle : particles) {
                //oldX = particle.x;
                //oldY = particle.y;
                num++;
                num2 = -1;
                //particle.xVelocity = 0;
                //particle.yVelocity = 0;
                for (auto& particle2 : particles) {
                    num2++;
                    if (&particle != &particle2) {
                        float angle = wrappedAngle(particle.x, particle.y, particle2.x, particle2.y, window.getSize().x, window.getSize().y);
                        float force = attractionMatrix[particle.type][particle2.type];
                        float dist = wrappedDistance(particle.x, particle.y, particle2.x, particle2.y, window.getSize().x, window.getSize().y);

                        if (dist > particle.size * half && dist < particle.size * settings[4]) {
                            particle.moveRadians(angle, (force / dist) * deltaTime * particle.size);
                        }
                        else if (dist <= particle.size * half) {
                            particle.moveRadians(angle, (-20.0f * deltaTime * particle.size) / dist);
                        }
                        dist = distance(particle.x, particle.y, particle2.x, particle2.y);
                        /*
                        if (dist < particle.size * 4 && settings[3] && num < num2) {
                            drawLine(window, sf::Vector2f(particle.x, particle.y), sf::Vector2f(particle2.x, particle2.y), particle.size, sf::Color(255,255,255,10));
                        }
                        */
                    }
                }
                particle.move();

                if (particle.x > window.getSize().x) particle.x = 0;
                if (particle.x < 0) particle.x = window.getSize().x;
                if (particle.y > window.getSize().y) particle.y = 0;
                if (particle.y < 0) particle.y = window.getSize().y;
                /*for (auto& particle2 : particles) {
                    float dist = distance(particle.x, particle.y, particle2.x, particle2.y);
                    if (&particle != &particle2 && dist < particle.size + particle2.size) {
                        particle.x = oldX;
                        particle.y = oldY;
                    }
                }*/
            }
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) && !pauseWait) {
            paused = false;
            pauseWait = true;
        }

        if(!snake) window.clear(sf::Color(0, 0, 0));
        for (auto& particle : particles) {
            particle.draw(window);
        }
        if (needHelp) {
            window.draw(text_menuHelp);
        }
        if (menu) {
            needHelp = false;
            snake = false;
            if (click(window.getPosition(), deleteButton)) {
                particles.clear();
            }
            if (click(window.getPosition(), resetButton)) {
                for (int i = 0; i < 12; i++) {
                    for (int j = 0; j < 12; j++) {
                        attractionMatrix[i][j] = ((rand() % 201) - 100);
                    }
                }
            }
            if (text_size.getFillColor() == sf::Color::White) {
                text_size.setString(to_string(settings[1]));
                if (click(window.getPosition(), text_size) && sizeWait) {
                    text_size.setFillColor(sf::Color::Blue);
                    currentEdit = "";
                    sizeWait = false;
                }
            }
            else {
                text_size.setString(currentEdit);
                if ((click(window.getPosition(), text_size) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Enter )) && sizeWait) {
                    text_size.setFillColor(sf::Color::White);
                    settings[1] = stoi(currentEdit);
                    currentEdit = "";
                    sizeWait = false;
                }
            }
            if (text_amount.getFillColor() == sf::Color::White) {
                text_amount.setString(to_string(particles.size()));
                if (click(window.getPosition(), text_amount) && sizeWait) {
                    text_amount.setFillColor(sf::Color::Blue);
                    currentEdit = "";
                    sizeWait = false;
                }
            }
            else {
                text_amount.setString(currentEdit);
                if ((click(window.getPosition(), text_amount) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Enter)) && sizeWait) {
                    text_amount.setFillColor(sf::Color::White);
                    particles.clear();
                    for (int i = 0; i < stoi(currentEdit); i++) {
                        particles.push_back(Particle(rand() % window.getSize().x, rand() % window.getSize().y, rand() % settings[2], settings[1]));
                    }
                    currentEdit = "";
                    sizeWait = false;
                }
            }
            if (text_types.getFillColor() == sf::Color::White) {
                text_types.setString(to_string(settings[2]));
                if (click(window.getPosition(), text_types) && sizeWait) {
                    text_types.setFillColor(sf::Color::Blue);
                    currentEdit = "";
                    sizeWait = false;
                }
            }
            else {
                text_types.setString(currentEdit);
                if ((click(window.getPosition(), text_types) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Enter)) && sizeWait) {
                    text_types.setFillColor(sf::Color::White);
					settings[2] = stoi(currentEdit);
                    currentEdit = "";
                    sizeWait = false;
                }
            }
                
            window.draw(back);
            window.draw(text_title);
            window.draw(text_label);
            window.draw(deleteButton);
            window.draw(text_size);
            window.draw(resetButton);
            window.draw(text_amount);
            window.draw(text_types);
			window.draw(text_moreHelp);
        }
        window.display();

        if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) pauseWait = false;
		if (!sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) sizeWait = true;
    }
}