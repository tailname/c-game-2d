#include "level.h"
#include <iostream>


using namespace tinyxml2;

int Object::GetPropertyInt(std::string name)
{
    return atoi(properties[name].c_str());
}

float Object::GetPropertyFloat(std::string name)
{
    return strtod(properties[name].c_str(), NULL);
}

std::string Object::GetPropertyString(std::string name)
{
    return properties[name];
}

bool Level::LoadFromFile(std::string filename)
{
    XMLDocument levelFile;

    // Load XML file
    if (levelFile.LoadFile(filename.c_str()) != XML_SUCCESS)
    {
        std::cout << "Loading level \"" << filename << "\" failed." << std::endl;
        return false;
    }

    // Get the root map element
    XMLElement* map = levelFile.FirstChildElement("map");
    if (!map)
    {
        std::cout << "Map element not found" << std::endl;
        return false;
    }

    // Get map attributes
    width = map->IntAttribute("width");
    height = map->IntAttribute("height");
    tileWidth = map->IntAttribute("tilewidth");
    tileHeight = map->IntAttribute("tileheight");

    // Load tileset information
    XMLElement* tilesetElement = map->FirstChildElement("tileset");
    firstTileID = tilesetElement->IntAttribute("firstgid");

    // Get image element and path
    XMLElement* image = tilesetElement->FirstChildElement("image");
    const char* imagepath = image->Attribute("source");

    // Load tileset image
    sf::Image img;
    if (!img.loadFromFile(imagepath))
    {
        std::cout << "Failed to load tile sheet." << std::endl;
        return false;
    }

    img.createMaskFromColor(sf::Color(109, 159, 185));
    tilesetImage.loadFromImage(img);
    tilesetImage.setSmooth(false);

    // Calculate tileset properties
    int columns = tilesetImage.getSize().x / tileWidth;
    int rows = tilesetImage.getSize().y / tileHeight;

    // Create texture rectangles
    std::vector<sf::Rect<int>> subRects;
    for (int y = 0; y < rows; y++)
        for (int x = 0; x < columns; x++)
        {
            sf::Rect<int> rect;
            rect.top = y * tileHeight;
            rect.height = tileHeight;
            rect.left = x * tileWidth;
            rect.width = tileWidth;
            subRects.push_back(rect);
        }

    // Parse layers
    for (XMLElement* layerElement = map->FirstChildElement("layer");
        layerElement;
        layerElement = layerElement->NextSiblingElement("layer"))
    {
        Layer layer;

        // Get layer opacity
        if (layerElement->FindAttribute("opacity"))
        {
            float opacity = layerElement->FloatAttribute("opacity");
            layer.opacity = 255 * opacity;
        }
        else
        {
            layer.opacity = 255;
        }

        // Get layer data element
        XMLElement* layerDataElement = layerElement->FirstChildElement("data");
        if (!layerDataElement)
        {
            std::cout << "Bad map. No layer information found." << std::endl;
            return false;
        }

        // Parse tile elements
        int x = 0, y = 0;
        for (XMLElement* tileElement = layerDataElement->FirstChildElement("tile");
            tileElement;
            tileElement = tileElement->NextSiblingElement("tile"))
        {
            int tileGID = 0;
            if (tileElement->FindAttribute("gid"))
            {
                tileGID = tileElement->IntAttribute("gid");
            }

            int subRectToUse = tileGID - firstTileID;
            if (subRectToUse >= 0)
            {
                sf::Sprite sprite;
                sprite.setTexture(tilesetImage);
                sprite.setTextureRect(subRects[subRectToUse]);
                sprite.setPosition(x * tileWidth, y * tileHeight);
                sprite.setColor(sf::Color(255, 255, 255, layer.opacity));
                layer.tiles.push_back(sprite);
            }

            x++;
            if (x >= width)
            {
                x = 0;
                y++;
                if (y >= height)
                    y = 0;
            }
        }

        layers.push_back(layer);
    }

    // Parse object groups
    XMLElement* objectGroupElement = map->FirstChildElement("objectgroup");
    if (objectGroupElement)
    {
        while (objectGroupElement)
        {
            for (XMLElement* objectElement = objectGroupElement->FirstChildElement("object");
                objectElement;
                objectElement = objectElement->NextSiblingElement("object"))
            {
                std::string objectType = objectElement->Attribute("type") ? objectElement->Attribute("type") : "";
                std::string objectName = objectElement->Attribute("name") ? objectElement->Attribute("name") : "";

                int x = objectElement->IntAttribute("x");
                int y = objectElement->IntAttribute("y");
                int width = 0, height = 0;

                sf::Sprite sprite;
                sprite.setTexture(tilesetImage);
                sprite.setTextureRect(sf::Rect<int>(0, 0, 0, 0));
                sprite.setPosition(x, y);

                if (objectElement->FindAttribute("width"))
                {
                    width = objectElement->IntAttribute("width");
                    height = objectElement->IntAttribute("height");
                    if (objectElement->FindAttribute("gid"))
                    {
                        sprite.setTextureRect(subRects[objectElement->IntAttribute("gid") - firstTileID]);
                    }
                }
                else if (objectElement->FindAttribute("gid"))
                {
                    int gid = objectElement->IntAttribute("gid");
                    width = subRects[gid - firstTileID].width;
                    height = subRects[gid - firstTileID].height;
                    sprite.setTextureRect(subRects[gid - firstTileID]);
                }

                Object object;
                object.name = objectName;
                object.type = objectType;
                object.sprite = sprite;

                sf::Rect<int> objectRect;
                objectRect.top = y;
                objectRect.left = x;
                objectRect.height = height;
                objectRect.width = width;
                object.rect = objectRect;

                // Parse object properties
                XMLElement* properties = objectElement->FirstChildElement("properties");
                if (properties)
                {
                    for (XMLElement* prop = properties->FirstChildElement("property");
                        prop;
                        prop = prop->NextSiblingElement("property"))
                    {
                        std::string propertyName = prop->Attribute("name");
                        std::string propertyValue = prop->Attribute("value");
                        object.properties[propertyName] = propertyValue;
                    }
                }

                objects.push_back(object);
            }
            objectGroupElement = objectGroupElement->NextSiblingElement("objectgroup");
        }
    }
    else
    {
        std::cout << "No object layers found..." << std::endl;
    }

    return true;
}

Object Level::GetObject(std::string name)
{
    for (const auto& object : objects)
        if (object.name == name)
            return object;
    return Object(); // Return empty object if not found
}

std::vector<Object> Level::GetObjects(std::string name)
{
    std::vector<Object> vec;
    for (const auto& object : objects)
        if (object.name == name)
            vec.push_back(object);
    return vec;
}

sf::Vector2i Level::GetTileSize()
{
    return sf::Vector2i(tileWidth, tileHeight);
}

void Level::Draw(sf::RenderWindow& window)
{
    for (const auto& layer : layers)
        for (const auto& tile : layer.tiles)
            window.draw(tile);
}