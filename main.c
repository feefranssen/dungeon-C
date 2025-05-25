
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Enums & Structs

typedef enum { HEAL, DAMAGE } ItemType;

typedef struct Item {
    char name[32];
    ItemType type;
    int value;
} Item;

typedef struct Monster {
    char name[32];
    int health, damage;
} Monster;

typedef struct Room Room;

typedef struct RoomList {
    Room* room;
    struct RoomList* next;
} RoomList;

struct Room {
    int id, hasTreasure, visited;
    RoomList* connections;
    Item* item;
    Monster* monster;
};

typedef struct Player {
    Room* currentRoom;
    int health, baseDamage, bonusDamage;
} Player;

// Dungeon Helpers

Item* randomItem() {
    if (rand() % 4 < 3) return NULL;
    Item* i = malloc(sizeof(Item));
    if (rand() % 2) {
        strcpy(i->name, "Health Potion");
        i->type = HEAL;
        i->value = rand() % 10 + 5;
    } else {
        strcpy(i->name, "Sword Upgrade");
        i->type = DAMAGE;
        i->value = rand() % 3 + 1;
    }
    return i;
}

Monster* randomMonster() {
    if (rand() % 4 < 3) return NULL;
    Monster* m = malloc(sizeof(Monster));
    if (rand() % 2) {
        strcpy(m->name, "Goblin");
        m->health = 10; m->damage = 2;
    } else {
        strcpy(m->name, "Orc");
        m->health = 15; m->damage = 3;
    }
    return m;
}

void addConnection(Room* a, Room* b) {
    for (RoomList* r = a->connections; r; r = r->next)
        if (r->room == b) return;
    RoomList* n = malloc(sizeof(RoomList));
    n->room = b; n->next = a->connections; a->connections = n;
}

Room* createRoom(int id) {
    Room* r = calloc(1, sizeof(Room));
    r->id = id;
    return r;
}

void generateDungeon(Room** rooms, int count) {
    for (int i = 0; i < count; i++) {
        rooms[i] = createRoom(i);
        if (i > 0) {
            int r = rand() % i;
            addConnection(rooms[i], rooms[r]);
            addConnection(rooms[r], rooms[i]);
        }
        rooms[i]->item = randomItem();
        rooms[i]->monster = randomMonster();
    }
    rooms[rand() % count]->hasTreasure = 1;
}


void fight(Player* p, Monster* m) {
    printf("Gevecht gestart tegen %s\n", m->name);
    while (p->health > 0 && m->health > 0) {
        int pattern = rand() % 16;
        printf("Aanval volgorde: ");
        for (int i = 3; i >= 0; i--)
            printf("%d", (pattern >> i) & 1);
        printf("\n");
        for (int i = 3; i >= 0; i--) {
            if ((pattern >> i) & 1) {
                m->health -= (p->baseDamage + p->bonusDamage);
                printf("Speler valt aan (%s HP: %d)\n", m->name, m->health);
            } else {
                p->health -= m->damage;
                printf("%s valt aan (Speler HP: %d)\n", m->name, p->health);

            }
            if (m->health <= 0 || p->health <= 0) break;
        }
    }
    if (p->health <= 0) { printf("Je bent gestorven!\n"); exit(0); }
    printf("%s verslagen!\n", m->name); free(m);
}

void enterRoom(Player* p, Room* r) {
    printf("\n--- Kamer %d ---\n", r->id);
    r->visited = 1; p->currentRoom = r;
    if (r->hasTreasure) { printf("Je vond de schat! Gewonnen!\n"); exit(0); }
    if (r->monster) { fight(p, r->monster); r->monster = NULL; }
    if (r->item) {
        if (r->item->type == HEAL) {
            p->health += r->item->value;
            printf("%s gevonden! +%d HP (Totaal: %d)\n", r->item->name, r->item->value, p->health);
        } else {
            p->bonusDamage += r->item->value;
            printf("%s gevonden! +%d schade\n", r->item->name, r->item->value);
        }
        free(r->item); r->item = NULL;
    }
    printf("Verbindingen: ");
    for (RoomList* rl = r->connections; rl; rl = rl->next)
        printf("%d ", rl->room->id);
    printf("\n");
}

Room* getRoomById(Room** rooms, int count, int id) {
    return (id >= 0 && id < count) ? rooms[id] : NULL;
}

void playGame(Room** rooms, int count, Player* player) {
    char input[16], filename[64];
    while (1) {
        enterRoom(player, player->currentRoom);
        printf("Typ kamernummer, 's' om op te slaan, 'q' om te stoppen: ");
        scanf("%s", input);
        if (!strcmp(input, "s")) {
            printf("Bestandsnaam: "); scanf("%s", filename);
            FILE* f = fopen(filename, "w");
            fprintf(f, "%d %d %d %d\n", player->currentRoom->id, player->health, player->baseDamage, player->bonusDamage);
            for (int i = 0; i < count; i++) {
                fprintf(f, "%d %d %d ", rooms[i]->id, rooms[i]->visited, rooms[i]->hasTreasure);
                for (RoomList* r = rooms[i]->connections; r; r = r->next)
                    fprintf(f, "%d ", r->room->id);
                fprintf(f, "-1\n");
            }
            fclose(f); printf("Spel opgeslagen.\n"); continue;
        } else if (!strcmp(input, "q")) break;

        int choice = atoi(input);
        for (RoomList* rl = player->currentRoom->connections; rl; rl = rl->next)
            if (rl->room->id == choice) { player->currentRoom = rl->room; goto nextTurn; }
        printf("Ongeldige keuze.\n");
        nextTurn:;
    }
}

void loadGame(Player* p, Room** rooms, int count, const char* fname) {
    FILE* f = fopen(fname, "r");
    if (!f) { printf("Kan bestand niet openen.\n"); exit(1); }
    int roomId; fscanf(f, "%d %d %d %d", &roomId, &p->health, &p->baseDamage, &p->bonusDamage);
    for (int i = 0; i < count; i++) rooms[i] = createRoom(i);
    for (int i = 0, id, vis, treas; i < count; i++) {
        fscanf(f, "%d %d %d", &id, &vis, &treas);
        rooms[id]->visited = vis; rooms[id]->hasTreasure = treas;
        int conn;
        while (fscanf(f, "%d", &conn) && conn != -1)
            addConnection(rooms[id], rooms[conn]);
    }
    p->currentRoom = rooms[roomId];
    fclose(f);
}

void freeRoom(Room* r) {
    RoomList* c = r->connections;
    while (c) { RoomList* next = c->next; free(c); c = next; }
    if (r->item) free(r->item);
    if (r->monster) free(r->monster);
    free(r);
}

void cleanup(Room** rooms, int count) {
    for (int i = 0; i < count; i++) freeRoom(rooms[i]);
    free(rooms);
}


int main(int argc, char* argv[]) {
    srand(time(NULL));
    if (argc < 3) {
        printf("Gebruik: %s new <n> | load <bestand>\n", argv[0]);
        return 1;
    }
    int count = 100;
    Room** rooms = malloc(sizeof(Room*) * count);
    Player player = {.health = 30, .baseDamage = 5, .bonusDamage = 0};
    if (!strcmp(argv[1], "new")) {
        count = atoi(argv[2]);
        rooms = malloc(sizeof(Room*) * count);
        generateDungeon(rooms, count);
        player.currentRoom = rooms[0];
    } else if (!strcmp(argv[1], "load")) {
        loadGame(&player, rooms, count, argv[2]);
    } else {
        printf("Ongeldige modus.\n"); return 1;
    }
    playGame(rooms, count, &player);
    cleanup(rooms, count);
    return 0;
}
