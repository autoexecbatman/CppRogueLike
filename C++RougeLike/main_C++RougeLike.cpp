// Debug needs to be set to x86
#include "main_C++RougeLike.h"
//====
#define GRASS_PAIR     1
#define EMPTY_PAIR     1
#define WATER_PAIR     2
#define MOUNTAIN_PAIR  3
#define NPC_PAIR       4
#define PLAYER_PAIR    5
//====
Map::Map(int height, int width) :  height(height) , width(width)
{
    tiles = new Tile[height * width];

    //Bsp bsp(0, 0, height, width);

    //bsp.mysplitRecursive(NULL, 8, ROOM_MAX_SIZE, ROOM_MAX_SIZE, 1.5f, 1.5f);

    //BspListener listener(*this);
    //bsp.traverseInvertedLevelOrder(&listener, NULL);

    TCODBsp bsp(0, 0, width, height);
    /*TCODBsp* ptrbsp = 0;*/
    
    bsp.splitRecursive(NULL, 8, ROOM_MAX_SIZE, ROOM_MAX_SIZE, 1.5f, 1.5f);
    BspListener listener(*this);
    bsp.traverseInvertedLevelOrder(&listener, NULL);

    //setWall(22, 30);
    //setWall(22, 40);
}

Map::~Map()
{
    delete[] tiles;
}

bool Map::isWall(int y, int x) const
{
    return !tiles[y + x * width].canWalk;
}

void Map::setWall(int y, int x)
{
    tiles[y + x * width].canWalk = false;
}

void Map::render() const
{
    int y = 0;
    int x = 0;
    //green
    static const int darkWall = 1;
    //blue
    static const int darkGround = 3;
    /*mvchgat(y, x, 1, A_NORMAL, isWall(y, x) ? darkWall : darkGround, NULL);*/

    
    for (int x = 0; x < width; x++)//if x smaller then map width
    {
        
        for (int y = 0; y < height; y++)//if y smaller then map width
        {
            //TCODConsole::root->setCharBackground(x, y,
            //    isWall(x, y) ? darkWall : darkGround);
            /*printw("w");*/
            
            /*mvchgat(y,x,-1, A_NORMAL,isWall(y,x) ? darkWall : darkGround, NULL);*/
            mvchgat(y,x,1, A_NORMAL,isWall(y,x) ? darkWall : darkGround, NULL);
            //addch('#');
            //setWall(y,x);

        }
    }
    
    //for (int i = 0; i < x; i++)
    //{
    //    if (isWall(y, x))
    //    {
    //        addch('#');
    //    }
    //    else
    //    {
    //        addch('_');
    //    }
    //}

}

void Map::dig(int y1, int x1, int y2, int x2)
{

        //int tiley = y1+20;
        //int tilex = x1+20;
        

        //tiles[tilex + tiley * width].canWalk = true;
        //printw("x1:%u|", x1);
        //printw("y1:%u|\n", x1);


        //swap x
        if (x2 < x1)
        {
            //printw("x1:%u|", x1);
            //printw("y1:%u", x1);
            //printw("num:%u\n", iter);
            int tmp = x2;
            x2 = x1;
            x1 = tmp;
        }
        //printw("x1:%u~", x1);
        //printw("y1:%u|", x1);

        //swap y
        if (y2 < y1)
        {
            int tmp = y2;
            y2 = y1;
            y1 = tmp;
        }

        for (int tilex = x1; tilex <= x2; tilex++)
        {
            for (int tiley = y1; tiley <= y2; tiley++)
            {
                tiles[tilex + tiley * width].canWalk = true;
            }
        }
        
        //for (int tiley = y1; tiley <= y2; tiley++)
        //{
        //    for (int tilex = x1; tilex <= x2; tilex++)
        //    {
        //        tiles[tilex + tiley * width].canWalk = true;
        //    }
        //}
}

void Map::createRoom(bool first, int y1, int x1, int y2, int x2)
{
    /*Engine engine;*/

    dig(y1, x1, y2, x2);

    if (first)
    {
        ////put the player in the first room
        //engine.player->y = (y1 + y2) / 2;
        //engine.player->x = (x1 + x2) / 2;

        engine.player->y = x1 + 1;
        engine.player->x = y1 + 1;

    }
    else
    {
        //TCODRandom random;
        //std::default_random_engine* generator;
        //std::uniform_int_distribution<int> distribution(1, 6);
        //auto dice = std::bind(distribution, generator);
        //auto rng = dice;
        
        /*sp::StaticRandomInitializer* rng;*/
        //sp::StaticRandomInitializer::StaticRandomInitializer()* rng = 0;
        /*sp::StaticRandomInitializer::StaticRandomInitializer();*/
        //auto* rng = sp::irandom;



        /*srand(time(NULL));*/
        
        //int* ptr_rng;
        //int* ptr_rng = &rngrand;

        
        if (int rng = rand() % 4 == 0)
        {
            engine.actors.push_back(
                new Actor(
                (x1 + x2) / 2,
                (y1 + y2) / 2,
                '@',
                4
                )
            );
        }
    }
}
//====
Engine::Engine()
{
    //int map_y = 80;
    //int map_x = 45;
    initscr();
    start_color();
    
    player = new Actor(25, 40, '@',3);
    actors.push_back(player);
    /*actors.push_back(new Actor(13, 60,  '@', 3));*/
    map = new Map(30, 120);
}

Engine::~Engine() 
{
    actors.clear();
    actors.~vector();
    delete map;
}

void Engine::update()
{
    
    /*mvaddch(1,1,"%c",player->y);*/

    //mvprintw(1,1,"player->y:%u",player->y);
    //mvprintw(2,1,"player->x:%u",player->x);
    //printw("?");
    int key = getch();

    switch (key)
    {
    case UP:
        if (!map->isWall(player->y - 1, player->x))
        {
            player->y--;
        }
        break;

    case DOWN:
        if (!map->isWall(player->y + 1, player->x))
        {
            player->y++;
        }
        break;

    case LEFT:
        if (!map->isWall(player->y , player->x - 1))
        {
            player->x--;
        }
        break;

    case RIGHT:
        if (!map->isWall(player->y,player->x + 1))
        {
            player->x++;
        }
        break;

    default:
        break;
    }
}

void Engine::render() 
{    
    clear();//curses clear()
    // draw the map
    map->render();

    // draw the actors
    for (auto actor : actors)
    {
        actor->render();
    }
}
//====
Actor::Actor(int y, int x, int ch, int col) : y(y), x(x), ch(ch), col(col)
{}

void Actor::render() const
{
    attron(COLOR_PAIR(col));
    mvaddch(y, x, ch);
    attroff(COLOR_PAIR(col));
}
//====

int main()
{
//====
    init_pair(1, COLOR_YELLOW, COLOR_GREEN);
    init_pair(2, COLOR_CYAN, COLOR_BLUE);
    init_pair(3, COLOR_BLACK, COLOR_WHITE);
    init_pair(4, COLOR_RED, COLOR_MAGENTA);
    init_pair(5, COLOR_RED, COLOR_YELLOW );
//====
        /*bool quit = false;*/
    while (true/*!engine.quit == true*/)
    {
        /*initscr();*/

        engine.update();
        engine.render();
        refresh();
    }
    return 0;
}