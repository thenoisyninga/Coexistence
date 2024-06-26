#include <SFML/Graphics.hpp>
#include <iostream>
#include <future>
#include <ctime>
#include <cmath>
#include <vector>
#include "PerlinNoise.hpp"

using namespace std;
using namespace sf;

// Char identifiers for entities on screen
// (These chars are stored in the blueprints array, so that we can keep track of everyone's position)
char waterCharIdentifier = 'w';
char landCharIdentifier = 'l';
char rabbitCharIdentifier = 'r';
char plantCharIdentifier = 'p';
char wolfCharIdentifier = 'W';

// Setting global variables
const int height = 1080 / 2;
const int width = 1920 / 2;

// -------- RABBIT VARIABLES ----------
int intialNumRabbits = 30;
float rabbitSize = 1.5;
int rabbitVision = 30;

// This is the maximum of each thing an entity can live with, more than
// that will end in death (does not apply to reproductive urge)
float rabbitMaxHunger = 100;
float rabbitMaxThirst = 120;
float rabbitMaxReproductiveUrge = 100;

// These delta values are the amount that is incremented per frame to the
// respective thing(hunger, thirst or reproductive urge)
float rabbitHungerDelta = 0.05;
float rabbitThirstDelta = 0.05;
float rabbitReproductiveUrgeDelta = 0.05;

// Speed is randomly assigned to each object, so here are the min and max
// values for it.
float rabbitSpeedMin = 0.3;
float rabbitSpeedMax = 0.5;

// -------- WOLF VARIABLES ----------
int initialNumWolves = 20;
float wolfSize = 1.5;
int wolfVision = 40;

float wolfMaxHunger = 100;
float wolfMaxThirst = 80;
float wolfMaxReproductiveUrge = 50;

// These delta values are the amount that is incremented per frame to the
// respective thing(hunger, thirst or reproductive urge)
float wolfHungerDelta = 0.05;
float wolfThirstDelta = 0.05;
float wolfReproductiveUrgeDelta = 0.05;

float wolfSpeedMin = 0.5;
float wolfSpeedMax = 1;

// -------- PLANT VARIABLES ----------
float plantDensity = 4;
float plantSize = 5;
int numPlants = floor((width * height) * plantDensity);

// -------- COLOR VARIABLES ----------
// These store rgba values for colors used
int landColorRGBA[4] = {(int)(255 * 3.1 / 100), (int)(255 * 64.7 / 100), (int)(255 * 9.0 / 100), 255};
int waterColorRGBA[4] = {(int)(255 * 11.0 / 100), (int)(255 * 29.0 / 100), (int)(255 * 85.5 / 100), 255};
int treeColorRGBA[4] = {(int)(255 * 0.0 / 100), (int)(255 * 42.0 / 100), (int)(255 * 15.7 / 100), 255};

// -------- OTHER VARIABLES ----------
int frameRate = 30;

const float pi = 3.142;

class Rabbit;
class Plant;
class Wolf;

// Vector arrays to store objects
vector<Rabbit *> rabbits;
vector<Plant *> plants;
vector<Wolf *> wolves;

// Objects for terrain generation and display
Image terrainTextureImage;
Texture terrainTexture;
Sprite backgroundSprite;

// The POSITION BLUEPRINT
// Array that holds the position data for entities
vector<char> positionBlueprint[width][height];

// Boolean that is false until terrain is generated
bool terrainGenerated = false;

// Some Function headers
bool isLand(int x, int y);
bool isWithinBounds(int x, int y);
void addToPositionBlueprint(char charIdentifier, int x, int y);
bool checkPositionInBlueprint(char charIdentifier, int x, int y);
void removePositionFromBlueprint(char charIdentifier, int x, int y);
void removePlant(Vector2f position);
void addRabbit(Vector2f position);
void removeRabbit(Vector2f position);
void addWolf(Vector2f position);
void removeWolf(Vector2f position);

// ----------------- CLASSES ------------------

class Animal
{
protected:
    Sprite shape; // SFML Shape object for the animal
    Texture animalTexture;
    float speed;                 // Speed of the animal
    Vector2f direction;          // Direction the animal is headed
    Vector2f position;           // Current position of the animal
    Vector2f headedTo;           // Direction the animal is headed when roaming randomly
    Vector2f closestFoodSource;  // Stores the location of closest food source for the animal
    Vector2f closestWaterSource; // Stores the location of closest water source for the animal
    Vector2f closestMate;        // Stored the location of the closest mate for the animal
    float maxHunger;             // Max hunger that the animal can live with
    float maxThirst;             // Max thirst that the animal can live with
    float maxReproductiveUrge;   // Max reproductive urge
    float hungerLevel;           // Current hunger level of the animal
    float thirstLevel;           // Current thirst level of the animal
    float reproductiveUrge;      // Current reproductive urge of the animal

    friend void initializeRabbits(RenderWindow *window);

public:
    Animal(
        float speed,
        Vector2f direction,
        Vector2f position,
        float maxHunger,
        float maxThirst,
        float maxReproductiveUrge)
        : speed(speed),
          direction(direction),
          position(position),
          maxHunger(maxHunger),
          maxThirst(maxThirst),
          maxReproductiveUrge(maxReproductiveUrge),
          closestFoodSource(Vector2f(-1, -1)),
          closestWaterSource(Vector2f(-1, -1)),
          closestMate(Vector2f(-1, -1))
    {
        // Setting the shape origin to center
        shape.setOrigin(shape.getGlobalBounds().width / 2, shape.getGlobalBounds().height / 2);

        shape.setPosition(floor(position.x), floor(position.y));

        // Setting the initial headed to to a valid value so that it doesnt break
        headedTo = position;
    }

    virtual void draw(RenderWindow *window) = 0; // virtual function

    Vector2f getPosition()
    {
        return position;
    }

    // A function that choses random coordinates that are in range of
    // the animal's sight as the next headed to value
    void roam()
    {
        if (terrainGenerated)
        {
            Vector2f vectorToNextPoint = headedTo - position;
            float distanceToNextPoint = pow(pow(vectorToNextPoint.x, 2) + pow(vectorToNextPoint.y, 2), 0.5);

            // Generate new value if animal is close to the current headedTo position
            if (distanceToNextPoint < 5)
            {
                // Select new point to roam to
                int x, y;

                do
                {
                    float theta = (((float)(rand() % 1000) / 1000)) * (float)(2 * pi);
                    float r = (((float)(rand() % 1000) / 1000)) * rabbitVision;

                    x = (int)round(position.x + (float)(r * cos(theta)));
                    y = (int)round(position.y + (float)(r * sin(theta)));

                } while (!isLand(x, y));

                headedTo = Vector2f(x, y);
            }
        }
    }

    // Common part of the move function for all children
    virtual void move()
    {
        // Basic move funcion that just sets the animal's direction to its next goal
        Vector2f vectorToNextPoint = headedTo - position;
        float distanceToNextPoint = pow(pow(vectorToNextPoint.x, 2) + pow(vectorToNextPoint.y, 2), 0.5);

        direction = Vector2f(vectorToNextPoint.x / distanceToNextPoint, vectorToNextPoint.y / distanceToNextPoint);
    }

    void update()
    {
        roam();
        move();
    }
};

class Rabbit : public Animal
{
protected:
    Vector2f threatsAverageLocation;

public:
    Rabbit(
        float speed,
        Vector2f direction,
        Vector2f position,
        float maxHunger,
        float maxThirst,
        float maxReproductiveUrge)
        : Animal(
              speed,
              direction,
              position,
              maxHunger,
              maxThirst,
              maxReproductiveUrge),
          threatsAverageLocation(Vector2f(-1, -1))
    {
        // Setting the rabbit sprite with the rabbit image and giving it size
        Image rabbitImage;
        rabbitImage.loadFromFile("assets/images/RabbitFace.png");
        animalTexture.loadFromImage(rabbitImage);
        shape.setTexture(animalTexture);
        shape.setScale(Vector2f(shape.getScale().x / 15 * rabbitSize, shape.getScale().y / 15 * rabbitSize));
        // shape.setFillColor(Color::White);
        shape.setOrigin(shape.getGlobalBounds().width / 2, shape.getGlobalBounds().height / 2);

        // Setting random values for thirst hunger and mating urge
        hungerLevel = (float)(rand() % (int)(rabbitMaxHunger));
        thirstLevel = (float)(rand() % (int)(rabbitMaxThirst));
        reproductiveUrge = (float)(rand() % (int)(rabbitMaxReproductiveUrge));
    }

    void move() override
    {
        // Calling the parent's move because it is common for both children
        Animal::move();

        // Calculate velocity
        Vector2f velocity;

        velocity.x = direction.x * speed;
        velocity.y = direction.y * speed;

        // Remove old position blueprint
        removePositionFromBlueprint('r', floor(position.x), floor(position.y));

        position.x = position.x + velocity.x;
        position.y = position.y + velocity.y;

        // Add new position to blueprint
        addToPositionBlueprint('r', floor(position.x), floor(position.y));

        shape.setPosition(position);
    }

    void draw(RenderWindow *window)
    {
        window->draw(shape);
    }

    // Returns true if rabbit is near a plant
    bool atPlant()
    {
        bool found = false;
        for (int i = 0; i < 5; i++)
        {
            for (int j = 0; j < 5; j++)
            {
                found = checkPositionInBlueprint(plantCharIdentifier, position.x + i - 2, position.y + j - 2);
                if (found)
                    return found;
            }
        }
        return found;
    }

    // Returns true if rabbit is near water
    bool atWater()
    {
        bool found = false;
        for (int i = 0; i < 5; i++)
        {
            for (int j = 0; j < 5; j++)
            {
                found = checkPositionInBlueprint(waterCharIdentifier, position.x + i - 2, position.y + j - 2);
                if (found)
                    return found;
            }
        }
        return found;
    }

    // Returns true if rabbit is near another rabbit
    bool atMate()
    {
        bool found = false;
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                if (i != 0 && j != 0)
                {
                    found = checkPositionInBlueprint(rabbitCharIdentifier, position.x + i - 1, position.y + j - 1);
                    if (found)
                        return found;
                }
            }
        }

        return found;
    }

    // A function that scans the surroundings and takes note of important things
    void scanSurroundings()
    {

        closestFoodSource = Vector2f(-1, -1);
        closestWaterSource = Vector2f(-1, -1);
        closestMate = Vector2f(-1, -1);
        threatsAverageLocation = Vector2f(-1, -1);

        // Scan suroundings and look for plant
        for (int r = 0; r < rabbitVision + 1; r++)
        {
            // First checking the pixel the rabbit is currently on
            if (r == 0)
            {
                int search_x = round(position.x);
                int search_y = round(position.y);

                // If plant found and plant not already found (is closest)
                if (checkPositionInBlueprint(plantCharIdentifier, search_x, search_y) && closestFoodSource == Vector2f(-1, -1))
                {
                    closestFoodSource = Vector2f(search_x, search_y);
                }

                // If water found and is closest
                if (checkPositionInBlueprint(waterCharIdentifier, search_x, search_y) && closestWaterSource == Vector2f(-1, -1))
                {
                    closestWaterSource = Vector2f(search_x, search_y);
                }
            }

            // Checking all other pixels in the rabbit's field of vision other than its own position
            else
            {
                float dtheta = (float)(1 / (float)(2 * r));
                for (float theta = 0; theta < (2 * pi); theta += dtheta)
                {
                    int search_x = round(position.x + r * cos(theta));
                    int search_y = round(position.y + r * sin(theta));

                    // If plant found and plant not already found
                    if (checkPositionInBlueprint(plantCharIdentifier, search_x, search_y) && closestFoodSource == Vector2f(-1, -1))
                    {
                        closestFoodSource = Vector2f(search_x, search_y);
                    }

                    // If water found
                    if (checkPositionInBlueprint(waterCharIdentifier, search_x, search_y) && closestWaterSource == Vector2f(-1, -1))
                    {
                        closestWaterSource = Vector2f(search_x, search_y);
                    }

                    // If mate found
                    if (checkPositionInBlueprint(rabbitCharIdentifier, search_x, search_y) && closestMate == Vector2f(-1, -1))
                    {
                        if (floor(position.x) != search_x || floor(position.y) != search_y)
                        {

                            closestMate = Vector2f(search_x, search_y);
                        }
                    }
                }
            }
        }
    }

    void update()
    {

        // Increasing hunger, thirst and reproductive urge with time
        hungerLevel += rabbitHungerDelta;
        thirstLevel += rabbitThirstDelta;
        reproductiveUrge += rabbitReproductiveUrgeDelta;

        // Scanning surroundings to take note of everything
        scanSurroundings();

        // Now checking where to go to next

        // If hunger level is high then set next destination as food (if available)
        if (hungerLevel > (float)(maxHunger / 2) && closestFoodSource != Vector2f(-1, -1))
        {
            headedTo = closestFoodSource;

            if (atPlant())
            {
                hungerLevel = 0;
            }
        }
        // Else if thirst level is high then set next destination as water (if available)
        else if (thirstLevel > (float)(maxThirst / 2) && closestWaterSource != Vector2f(-1, -1))
        {
            headedTo = closestWaterSource;

            if (atWater())
            {
                thirstLevel = 0;
            }
        }
        // Else if reprodcutive urge is high then set next destination as mate (if available)
        else if (reproductiveUrge > (float)(maxReproductiveUrge / 2) && closestMate != Vector2f(-1, -1))
        {
            headedTo = closestMate;

            if (atMate())
            {
                reproductiveUrge = 0;
                // CREATE BABY
                addRabbit(position);
            }
        }
        // If all urges satisfied, then just roam randomly
        else
        {
            roam();
        }

        // Kill if too much hunger or thirst
        if (hungerLevel > maxHunger || thirstLevel > maxThirst)
        {
            removeRabbit(position);
        }

        // Move towards the next point (headed to)
        move();
    }
};

// Same as above except food is rabbit instead of plant and mate is fox instead of rabbit
class Wolf : public Animal
{
protected:
public:
    Wolf(
        float speed,
        Vector2f direction,
        Vector2f position,
        float maxHunger,
        float maxThirst,
        float maxReproductiveUrge)
        : Animal(
              speed,
              direction,
              position,
              maxHunger,
              maxThirst,
              maxReproductiveUrge)
    {

        Image wolfImage;
        wolfImage.loadFromFile("assets/images/WolfFace.png");
        animalTexture.loadFromImage(wolfImage);
        shape.setTexture(animalTexture);
        shape.setScale(Vector2f(shape.getScale().x / 12 * wolfSize, shape.getScale().y / 12 * wolfSize));

        shape.setOrigin(shape.getScale().x / 2, shape.getScale().y / 2);

        // Setting random values for thirst hunger and mating urge
        hungerLevel = (float)(rand() % (int)(wolfMaxHunger));
        thirstLevel = (float)(rand() % (int)(wolfMaxThirst));
        reproductiveUrge = (float)(rand() % (int)(wolfMaxReproductiveUrge));
    }

    void move() override
    {

        Animal::move();

        Vector2f velocity;

        velocity.x = direction.x * speed;
        velocity.y = direction.y * speed;

        // Remove old position blueprint
        removePositionFromBlueprint(wolfCharIdentifier, floor(position.x), floor(position.y));

        position.x = position.x + velocity.x;
        position.y = position.y + velocity.y;

        addToPositionBlueprint(wolfCharIdentifier, floor(position.x), floor(position.y));

        shape.setPosition(position);
    }

    void draw(RenderWindow *window)
    {
        window->draw(shape);
    }

    bool atRabbit()
    {

        return checkPositionInBlueprint(rabbitCharIdentifier, position.x, position.y);
    }

    bool atWater()
    {
        bool found = false;
        for (int i = 0; i < 5; i++)
        {
            for (int j = 0; j < 5; j++)
            {
                found = checkPositionInBlueprint(waterCharIdentifier, position.x + i - 2, position.y + j - 2);
                if (found)
                    return found;
            }
        }
        return found;
    }

    bool atMate()
    {
        bool found = false;
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                if (i != 0 && j != 0)
                {
                    found = checkPositionInBlueprint(wolfCharIdentifier, position.x + i - 1, position.y + j - 1);
                    if (found)
                        return found;
                }
            }
        }

        return found;
    }

    void scanSurroundings()
    {

        closestFoodSource = Vector2f(-1, -1);
        closestWaterSource = Vector2f(-1, -1);
        closestMate = Vector2f(-1, -1);

        // Scan suroundings and look for plant
        for (int r = 0; r < wolfVision + 1; r++)
        {
            // First checking the pixel the wolf is currently on
            if (r == 0)
            {
                int search_x = round(position.x);
                int search_y = round(position.y);

                // If rabbit found and plant not already found
                if (checkPositionInBlueprint(rabbitCharIdentifier, search_x, search_y) && closestFoodSource == Vector2f(-1, -1))
                {
                    closestFoodSource = Vector2f(search_x, search_y);
                }

                // If water found
                if (checkPositionInBlueprint(waterCharIdentifier, search_x, search_y) && closestWaterSource == Vector2f(-1, -1))
                {
                    closestWaterSource = Vector2f(search_x, search_y);
                }
            }
            // Checking all other pixels in the wolf's field of vision other than its own position
            else
            {
                float dtheta = (float)(1 / (float)(2 * r));
                for (float theta = 0; theta < (2 * pi); theta += dtheta)
                {
                    int search_x = round(position.x + r * cos(theta));
                    int search_y = round(position.y + r * sin(theta));

                    // If rabbit found and plant not already found
                    if (checkPositionInBlueprint(rabbitCharIdentifier, search_x, search_y) && closestFoodSource == Vector2f(-1, -1))
                    {
                        closestFoodSource = Vector2f(search_x, search_y);
                    }

                    // If water found
                    if (checkPositionInBlueprint(waterCharIdentifier, search_x, search_y) && closestWaterSource == Vector2f(-1, -1))
                    {
                        closestWaterSource = Vector2f(search_x, search_y);
                    }

                    // If mate found
                    if (checkPositionInBlueprint(wolfCharIdentifier, search_x, search_y) && closestMate == Vector2f(-1, -1))
                    {
                        if (floor(position.x) != search_x || floor(position.y) != search_y)
                        {

                            closestMate = Vector2f(search_x, search_y);
                        }
                    }
                }
            }
        }
    }

    void update()
    {

        hungerLevel += wolfHungerDelta;
        thirstLevel += wolfThirstDelta;
        reproductiveUrge += wolfReproductiveUrgeDelta;

        scanSurroundings();

        // If hunger level is down then check for plant before roaming
        if (hungerLevel > (float)(maxHunger / 2) && closestFoodSource != Vector2f(-1, -1))
        {
            headedTo = closestFoodSource;

            if (atRabbit())
            {
                hungerLevel = 0;

                removeRabbit(Vector2f(floor(position.x), floor(position.y)));
            }
        }
        else if (thirstLevel > (float)(maxThirst / 2) && closestWaterSource != Vector2f(-1, -1))
        {
            headedTo = closestWaterSource;

            if (atWater())
            {
                thirstLevel = 0;
            }
        }
        else if (reproductiveUrge > (float)(maxReproductiveUrge / 2) && closestMate != Vector2f(-1, -1))
        {
            headedTo = closestMate;

            if (atMate())
            {
                reproductiveUrge = 0;
                // CREATE BABY
                addWolf(position);
            }
        }
        else
        {
            roam();
        }

        // Kill if too much hunger
        if (hungerLevel > maxHunger || thirstLevel > maxThirst)
        {
            removeWolf(position);
        }

        move();
    }
};

class Plant
{
    CircleShape shape;
    Vector2f position;

public:
    Plant(Vector2f position) : position(position)
    {
        // Set graphical stuff
        shape.setRadius(plantSize);
        shape.setOutlineColor(Color(0, 50, 0, 255));
        shape.setOutlineThickness(0.5);
        // Centering the shape's origin
        shape.setOrigin(shape.getGlobalBounds().width / 2, shape.getGlobalBounds().height / 2);
        shape.setPosition(floor(position.x), floor(position.y));

        shape.setFillColor(Color(treeColorRGBA[0], treeColorRGBA[1], treeColorRGBA[2], treeColorRGBA[3]));
    }

    Vector2f getPosition()
    {
        return this->position;
    }

    void draw(RenderWindow *window)
    {
        window->draw(shape);
    }
};

void displayLoadingScreen(RenderWindow *window)
{

    // Creating and setting text
    Text headingText;
    Font font;
    font.loadFromFile("assets/fonts/8-bit-hud.ttf");
    headingText.setFont(font);
    headingText.setString("Generating\nTerrain...");

    headingText.setCharacterSize(25);

    sf::FloatRect bounds = headingText.getLocalBounds();
    headingText.setOrigin(-bounds.left + bounds.width / 2.f, -bounds.top + bounds.height / 2.f);

    headingText.setPosition(Vector2f(window->getSize().x / 2, window->getSize().y / 2));
    headingText.setFillColor(Color::White);

    // Drawing and displaying loading screen text
    window->clear();
    window->draw(headingText);

    window->display();
}

void drawPopulationStats(RenderWindow *window)
{
    // Drawing a translucent rectangle and the number of population
    // as text on top of it.

    Text text;
    Font font;
    font.loadFromFile("assets/fonts/Jersey15-Regular.ttf");
    text.setFont(font);

    String textString = "Rabbits Alive: " + to_string(rabbits.size()) + "                                                                                                                                                                     Wolves Alive: " + to_string(wolves.size());
    RectangleShape shape(Vector2f(width, 30));

    shape.setFillColor(Color(0, 0, 0, 255 * 0.9));
    text.setFillColor(Color::White);

    text.setString(textString);
    text.setCharacterSize(20);

    shape.setPosition(Vector2f(0, 0));
    text.setPosition(Vector2f((width / 2) - (text.getLocalBounds().width / 2), (text.getLocalBounds().height - 10)));

    window->draw(shape);
    window->draw(text);
}

bool onIntro = true;

void drawIntroScreen(RenderWindow *window)
{
    // Draw the intro menu image
    Image introImage;
    introImage.loadFromFile("assets/images/IntroScreen.png");
    Texture introTexture;
    introTexture.loadFromImage(introImage);

    Sprite introSprite;
    introSprite.setTexture(introTexture);
    introSprite.setScale(0.5, 0.5);

    window->draw(introSprite);
}

// ------------ TERRAIN FUNCTIONS ----------------

// To generate a terrain using perlin noise
void generateTerrain()
{
    terrainTextureImage.create(width, height, sf::Color(0, 0, 0, 0));

    const siv::PerlinNoise::seed_type seed = rand();
    const siv::PerlinNoise perlin{seed};

    for (int i = 0; i < width; i++)
    {
        for (int j = 0; j < height; j++)
        {
            const double noise = perlin.octave2D_01((i * 0.01), (j * 0.01), 1, 0.2);

            // Values above 0.4 are land, rest are water
            if (noise > 0.4)
            {
                addToPositionBlueprint('l', i, j);
                terrainTextureImage.setPixel(i, j, Color(landColorRGBA[0], landColorRGBA[1], landColorRGBA[2], landColorRGBA[3]));
            }
            else
            {
                addToPositionBlueprint('w', i, j);
                terrainTextureImage.setPixel(i, j, Color(waterColorRGBA[0], waterColorRGBA[1], waterColorRGBA[2], waterColorRGBA[3]));
            }
        }
    }

    terrainTexture.loadFromImage(terrainTextureImage);

    backgroundSprite.setTexture(terrainTexture);

    terrainGenerated = true;

    // Printing seed for the terrain generated
    cout << "\nSeed: " << seed << endl;
}

// ------------ POSITION BLUEPRINT FUNCTIONS ----------------

// Add the character to the blueprint at specific pixel
void addToPositionBlueprint(char charIdentifier, int x, int y)
{
    // Adding the char to the blueprint
    if (isWithinBounds(x, y))
    {
        positionBlueprint[x][y].push_back(charIdentifier);
    }
}

// Checks if the character exists in the blueprint at the specific pixel
bool checkPositionInBlueprint(char charIdentifier, int x, int y)
{
    if (isWithinBounds(x, y))
    {
        for (int i = 0; i < positionBlueprint[x][y].size(); i++)
        {
            if (positionBlueprint[x][y][i] == charIdentifier)
            {
                return true;
            }
        }
    }

    return false;
}

// Remove the character to the blueprint at specific pixel
void removePositionFromBlueprint(char charIdentifier, int x, int y)
{
    if (isWithinBounds(x, y))
    {
        int existsAt = -1;

        // Checking if charIdentifier exists on the coordinates
        for (int i = 0; i < positionBlueprint[x][y].size(); i++)
        {
            if (positionBlueprint[x][y][i] == charIdentifier)
            {
                existsAt = i;
                break;
            }
        }

        // Removing position from blueprint if it exists
        if (existsAt != -1)
        {
            vector<char>::iterator it = positionBlueprint[x][y].begin();
            advance(it, existsAt);

            positionBlueprint[x][y].erase(it);
        }
    }
}

// ------------ UTILITY FUNCTIONS ----------------

// Returns true if the given coordinates are on land
bool isLand(int x, int y)
{
    if (isWithinBounds(x, y))
    {
        Color thisPixel = terrainTextureImage.getPixel(x, y);

        return thisPixel.r == landColorRGBA[0] && thisPixel.g == landColorRGBA[1] && thisPixel.b == landColorRGBA[2] && thisPixel.a == landColorRGBA[3];
    }
    else
    {
        return false;
    }
}

// Returns true if the given coordinates are within the bounds of the screen
bool isWithinBounds(int x, int y)
{

    return (x < width && x > 0 && y < height && y > 0);
}

// ------------ FUNCTIONS FOR RABBITS ------------------

// add a rabbit to the simulation at given position
void addRabbit(Vector2f position)
{
    int rabbit_x = floor(position.x);
    int rabbit_y = floor(position.y);

    // Creating a new rabbit with a pointer
    Rabbit *rabbit = new Rabbit((rabbitSpeedMin + ((float)(rand() % 1000) / 1000) * (rabbitSpeedMax - rabbitSpeedMin)),
                                Vector2f(1, 1),
                                Vector2f(rabbit_x, rabbit_y),
                                rabbitMaxHunger,
                                rabbitMaxThirst,
                                rabbitMaxReproductiveUrge);

    // Adding the rabbit's position to the blueprint
    addToPositionBlueprint('r', floor(position.x), floor(position.y));
    rabbits.push_back(rabbit);
}

// Remove a rabbit from the simulation fromt the specific point
void removeRabbit(Vector2f position)
{
    // Variable to hold index of the element to remove
    int targetAt = -1;

    // Getting the index
    for (int i = 0; i < rabbits.size(); i++)
    {
        if (Vector2f((float)floor(rabbits[i]->getPosition().x), (float)floor(rabbits[i]->getPosition().y)) == Vector2f((float)floor(position.x), (float)floor(position.y)))
        {
            targetAt = i;
            break;
        }
    }

    // If index is valid then remove the rabbit
    if (targetAt >= 0 && targetAt <= rabbits.size())
    {
        vector<Rabbit *>::iterator it = rabbits.begin();

        removePositionFromBlueprint(rabbitCharIdentifier, rabbits[targetAt]->getPosition().x, rabbits[targetAt]->getPosition().y);

        advance(it, targetAt);
        rabbits.erase(it);
    }
}

void initializeRabbits()
{
    // Create rabbits and set them on land
    for (int i = 0; i < intialNumRabbits; i++)
    {

        int rabbit_x;
        int rabbit_y;

        // Generate coordinates until they are on land
        do
        {
            rabbit_x = rand() % width;
            rabbit_y = rand() % height;
        } while (!isLand(rabbit_x, rabbit_y));

        addRabbit(Vector2f((float)rabbit_x, (float)rabbit_y));
    }
}

// Calls update function of all existing rabbits
void updateAllRabbits()
{
    for (int i = 0; i < rabbits.size(); i++)
    {
        rabbits[i]->update();
    }
}

// Calls draw function of all existing rabbits
void drawAllRabbits(RenderWindow *window)
{

    for (int i = 0; i < rabbits.size(); i++)
    {
        rabbits[i]->draw(window);
    }
}

// ------------ FUNCTIONS FOR WOLVES ------------------

// Same as rabbits

void addWolf(Vector2f position)
{
    int wolf_x = floor(position.x);
    int wolf_y = floor(position.y);
    // int wolf_x = floor(width / 2);
    // int wolf_y = floor(height / 2);

    Wolf *wolf = new Wolf((wolfSpeedMin + ((float)(rand() % 1000) / 1000) * (wolfSpeedMax - wolfSpeedMin)),
                          Vector2f(1, 1),
                          Vector2f(wolf_x, wolf_y),
                          wolfMaxHunger,
                          wolfMaxThirst,
                          wolfMaxReproductiveUrge);

    addToPositionBlueprint(wolfCharIdentifier, floor(position.x), floor(position.y));
    wolves.push_back(wolf);
}

void removeWolf(Vector2f position)
{

    int targetAt = -1;

    for (int i = 0; i < wolves.size(); i++)
    {
        if (wolves[i]->getPosition() == position)
        {
            targetAt = i;
            break;
        }
    }

    if (targetAt >= 0 && targetAt <= wolves.size())
    {
        vector<Wolf *>::iterator it = wolves.begin();

        removePositionFromBlueprint(wolfCharIdentifier, wolves[targetAt]->getPosition().x, wolves[targetAt]->getPosition().y);

        advance(it, targetAt);
        wolves.erase(it);
    }
}

void initializeWolves()
{
    for (int i = 0; i < initialNumWolves; i++)
    {

        int wolf_x;
        int wolf_y;

        do
        {
            wolf_x = rand() % width;
            wolf_y = rand() % height;
        } while (!isLand(wolf_x, wolf_y));

        addWolf(Vector2f((float)wolf_x, (float)wolf_y));
    }
}

void updateAllWolves()
{
    for (int i = 0; i < wolves.size(); i++)
    {
        wolves[i]->update();
    }
}

void drawAllWolves(RenderWindow *window)
{

    for (int i = 0; i < wolves.size(); i++)
    {
        wolves[i]->draw(window);
    }
}

// ------------- PLANT FUNCTIONS -----------------------
// Same as rabbits
void addPlant(Vector2f position)
{
    Plant *plant = new Plant(position);

    addToPositionBlueprint('p', floor(position.x), floor(position.y));
    plants.push_back(plant);
}

void removePlant(Vector2f position)
{
    int targetAt = -1;

    for (int i = 0; i < plants.size(); i++)
    {
        if (plants[i]->getPosition() == position)
        {
            targetAt = i;
            break;
        }
    }

    if (targetAt >= 0 && targetAt <= plants.size())
    {
        vector<Plant *>::iterator it = plants.begin();

        advance(it, targetAt);

        plants.erase(it);

        removePositionFromBlueprint('p', position.x, position.y);
    }
}

void initializePlant()
{
    int numPlant = (int)(plantDensity * (width * height) / 10000);

    for (int i = 0; i < (int)(numPlant); i++)
    {
        int plant_x;
        int plant_y;

        do
        {
            plant_x = rand() % width;
            plant_y = rand() % height;
        } while (!isLand(plant_x, plant_y));

        addPlant(Vector2f((float)plant_x, (float)plant_y));
    }
}

void drawAllPlants(RenderWindow *window)
{
    for (int i = 0; i < plants.size(); i++)
    {
        plants[i]->draw(window);
    }
}

// ------------- MASTER FUNCTIONS -----------------------

// Calls update functions of all the classes
void masterUpdate()
{
    updateAllRabbits();
    updateAllWolves();

    fprintf(stderr, "Rabbits Alive: %ld Wolves Alive: %ld\n", rabbits.size(), wolves.size());
}

// Draw everything there is to draw
void masterDraw(RenderWindow *window)
{
    // Draw the terrain
    window->draw(backgroundSprite);

    // Draw the rabbits
    drawAllRabbits(window);

    // Dray the wolves
    drawAllWolves(window);

    // Draw the plants
    drawAllPlants(window);

    // Draw population stats
    drawPopulationStats(window);
}

// Initializes everything that needs to be initialized
void masterInitialize()
{

    generateTerrain();
    initializeRabbits();
    initializeWolves();
    initializePlant();
}

int main()
{
    // Initializing random module
    srand(time(NULL));

    RenderWindow window(VideoMode(width, height), "Co-existence");
    RectangleShape blackScreen(Vector2f(width, height));

    // Initialization for fade effect
    blackScreen.setFillColor(Color::Black);
    int blackScreenAlpha = 255;

    // Initializing everything
    masterInitialize();

    // not redrawing same stuff
    window.setKeyRepeatEnabled(false);

    // Keeping framerate constant
    window.setFramerateLimit(frameRate);

    displayLoadingScreen(&window);

    while (window.isOpen())
    {

        Event event;
        while (window.pollEvent(event))
        {
            if (event.type == Event::Closed)
            {
                window.close();
            }

            // If spacebar pressed on intro then move on from intro
            if (onIntro && event.type == sf::Event::EventType::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Enter)
                {
                    onIntro = false;
                }
            }
        }

        // Draw intro if on intro
        if (onIntro)
        {
            window.clear();
            drawIntroScreen(&window);
        }
        else
        {
            // Fade effect cuz why not?
            if (blackScreenAlpha > 0)
            {
                blackScreenAlpha -= 64;
            }

            if (blackScreenAlpha < 0)
            {
                blackScreenAlpha = 0;
            }

            // Update everything
            masterUpdate();

            window.clear();

            // Draw everything
            masterDraw(&window);

            // Draw the fade screen (will be transparent once the fade has ended)
            blackScreen.setFillColor(Color(0, 0, 0, blackScreenAlpha));
            window.draw(blackScreen);
        };

        // Display everything
        window.display();
    }

    // BAS HOGAYAAAAAAAAAAAAAAAAAAAA
    return 0;
}