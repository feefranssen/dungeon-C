#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef enum { HEAL, DAMAGE } ItemType;

typedef struct Item {
    char name[32];
    ItemType type;
    int value;
} Item;

typedef struct Monster {
    char name[32];
    int health;
    int damage;
} Monster;

typedef struct Room {
    int id;
    struct Room** connections;
    int connectionCount;

    Item* item;
    Monster* monster;
    int hasTreasure;
    int visited;
} Room;

typedef struct Player {
    Room* currentRoom;
    int health;
    int baseDamage;
    int bonusDamage;
} Player;

Room* createRoom(int id);
void connectRooms(Room* a, Room* b);
Item* randomItem();
Monster* randomMonster();
void generateDungeon(Room** rooms, int count);
void fight(Player* player, Monster* monster);
void enterRoom(Player* player, Room* room);
void playGame(Room** rooms, int roomCount, Player* player);
void saveGame(Player* player, Room** rooms, int roomCount, const char* filename);
void loadGame(Player* player, Room** rooms, int roomCount, const char* filename);
void cleanup(Room** rooms, int roomCount);

Room* createRoom(int id) {
    Room* room = malloc(sizeof(Room));
    room->id = id;
    room->connections = malloc(sizeof(Room*) * 4);
    room->connectionCount = 0;
    room->item = NULL;
    room->monster = NULL;
    room->hasTreasure = 0;
    room->visited = 0;
    return room;
}

void connectRooms(Room* a, Room* b) {
    if (a->connectionCount < 4 && b->connectionCount < 4) {
        a->connections[a->connectionCount++] = b;
        b->connections[b->connectionCount++] = a;
    }
}

Item* randomItem() {
    if (rand() % 2 == 0) return NULL;

    Item* item = malloc(sizeof(Item));
    if (rand() % 2) {
        strcpy(item->name, "Healing Potion");
        item->type = HEAL;
        item->value = rand() % 10 + 5;
    } else {
        strcpy(item->name, "Sword Upgrade");
        item->type = DAMAGE;
        item->value = rand() % 3 + 1;
    }
    return item;
}

Monster* randomMonster() {
    if (rand() % 2 == 0) return NULL;

    Monster* m = malloc(sizeof(Monster));
    if (rand() % 2) {
        strcpy(m->name, "Goblin");
        m->health = 10;
        m->damage = 2;
    } else {
        strcpy(m->name, "Orc");
        m->health = 15;
        m->damage = 3;
    }
    return m;
}

void generateDungeon(Room** rooms, int count) {
    for (int i = 0; i < count; i++) {
        rooms[i] = createRoom(i);
        if (i > 0) {
            connectRooms(rooms[i], rooms[rand() % i]);
        }
        rooms[i]->item = randomItem();
        rooms[i]->monster = randomMonster();
    }
    rooms[rand() % count]->hasTreasure = 1; 
}

void fight(Player* player, Monster* monster) {
    printf("Er begint een gevecht tegen een %s!\n", monster->name);
    while (player->health > 0 && monster->health > 0) {
        int pattern = rand() % 16;
        printf("Aanval volgorde: ");
        for (int i = 3; i >= 0; i--) printf("%d", (pattern >> i) & 1);
        printf("\n");

        for (int i = 0; i < 4; i++) {
            if ((pattern >> i) & 1) {
                monster->health -= (player->baseDamage + player->bonusDamage);
                printf("De speler valt aan (%s HP: %d)\n", monster->name, monster->health);
            } else {
                player->health -= monster->damage;
                printf("De %s valt aan (Speler HP: %d)\n", monster->name, player->health);
            }
            if (monster->health <= 0 || player->health <= 0) break;
        }
    }
    if (player->health <= 0) {
        printf("De speler is gestorven!\n");
        exit(0);
    } else {
        printf("De %s is verslagen!\n", monster->name);
        free(monster);
    }
}

void enterRoom(Player* player, Room* room) {
    printf("\n--- Kamer %d ---\n", room->id);
    room->visited = 1;
    player->currentRoom = room;

    if (room->hasTreasure) {
        printf("Je hebt de schat gevonden! Je wint!\n");
        exit(0);
    }

    if (room->monster) {
        fight(player, room->monster);
        room->monster = NULL;
    }

    if (room->item) {
        if (room->item->type == HEAL) {
            player->health += room->item->value;
            printf("Je vond een %s! +%d HP (Totaal: %d)\n", room->item->name, room->item->value, player->health);
        } else {
            player->bonusDamage += room->item->value;
            printf("Je vond een %s! +%d schadebonus\n", room->item->name, room->item->value);
        }
        free(room->item);
        room->item = NULL;
    }

    printf("Verbindingen: ");
    for (int i = 0; i < room->connectionCount; i++) {
        printf("%d ", room->connections[i]->id);
    }
    printf("\n");
}

void playGame(Room** rooms, int roomCount, Player* player) {
    char input[16];
    char filename[64];

    while (1) {
        enterRoom(player, player->currentRoom);
        printf("Typ een kamernummer of 's' om op te slaan: ");
        scanf("%s", input);

        if (strcmp(input, "s") == 0 ) {
            printf("Voer bestandsnaam in om op te slaan: ");
            scanf("%s", filename);
            saveGame(player, rooms, roomCount, filename);
            printf("Spel opgeslagen in %s.\n", filename);
            continue;
        }

        int choice = atoi(input);  

        int valid = 0;
        for (int i = 0; i < player->currentRoom->connectionCount; i++) {
            if (player->currentRoom->connections[i]->id == choice) {
                player->currentRoom = player->currentRoom->connections[i];
                valid = 1;
                break;
            }
        }
        if (!valid) printf("Ongeldige keuze.\n");
    }
}

void saveGame(Player* player, Room** rooms, int count, const char* filename) {
    FILE* f = fopen(filename, "w");
    fprintf(f, "%d %d %d %d\n", player->currentRoom->id, player->health, player->baseDamage, player->bonusDamage);
    for (int i = 0; i < count; i++) {
        Room* r = rooms[i];
        fprintf(f, "%d %d %d %d\n", r->id, r->visited, r->hasTreasure, r->connectionCount);
        for (int j = 0; j < r->connectionCount; j++)
            fprintf(f, "%d ", r->connections[j]->id);
        fprintf(f, "\n");
    }
    fclose(f);
}

void loadGame(Player* player, Room** rooms, int count, const char* filename) {
    FILE* f = fopen(filename, "r");
    int roomId, visited, treasure, connCount, connId;
    fscanf(f, "%d %d %d %d", &roomId, &player->health, &player->baseDamage, &player->bonusDamage);

    for (int i = 0; i < count; i++) {
        rooms[i] = createRoom(i);
    }

    for (int i = 0; i < count; i++) {
        fscanf(f, "%d %d %d %d", &roomId, &visited, &treasure, &connCount);
        rooms[roomId]->visited = visited;
        rooms[roomId]->hasTreasure = treasure;
        for (int j = 0; j < connCount; j++) {
            fscanf(f, "%d", &connId);
            rooms[roomId]->connections[rooms[roomId]->connectionCount++] = rooms[connId];
        }
    }

    player->currentRoom = rooms[roomId];
    fclose(f);
}

void cleanup(Room** rooms, int count) {
    for (int i = 0; i < count; i++) {
        if (rooms[i]->item) free(rooms[i]->item);
        if (rooms[i]->monster) free(rooms[i]->monster);
        free(rooms[i]->connections);
        free(rooms[i]);
    }
}

int main(int argc, char* argv[]) {
    srand(time(NULL));
    if (argc < 3) {
        printf("Gebruik: .\\dungeon.exe new <kamers> | load <bestand>\n", argv[0]);
        return 1;
    }

    int roomCount;
    Room** rooms;
    Player player = {0};
    player.health = 30;
    player.baseDamage = 5;
    player.bonusDamage = 0;

    if (strcmp(argv[1], "new") == 0) {
        roomCount = atoi(argv[2]);
        rooms = malloc(sizeof(Room*) * roomCount);
        generateDungeon(rooms, roomCount);
        player.currentRoom = rooms[0];
    } else if (strcmp(argv[1], "load") == 0) {
        roomCount = 100; 
        rooms = malloc(sizeof(Room*) * roomCount);
        loadGame(&player, rooms, roomCount, argv[2]);
    } else {
        printf("Ongeldige optie.\n");
        return 1;
    }

    playGame(rooms, roomCount, &player);
    cleanup(rooms, roomCount);
    return 0;
}
