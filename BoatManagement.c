#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_BOATS 120

typedef enum {
    SLIP,
    LAND,
    TRAILOR,
    STORAGE
} LocationType;

typedef union {
    int slip_number;
    char bay_letter;
    char trailor_tag[16];
    int storage_number;
} LocationInfo;

typedef struct {
    char name[128];
    float length;
    LocationType type;
    LocationInfo location;
    float amount_owed;
} Boat;

Boat* boats[MAX_BOATS];
int boat_count = 0;

LocationType parse_location_type(const char* str) {
    if (strcasecmp(str, "slip") == 0) return SLIP;
    if (strcasecmp(str, "land") == 0) return LAND;
    if (strcasecmp(str, "trailor") == 0) return TRAILOR;
    if (strcasecmp(str, "storage") == 0) return STORAGE;
    return STORAGE;
}

const char* location_type_to_string(LocationType type) {
    switch (type) {
        case SLIP: return "slip";
        case LAND: return "land";
        case TRAILOR: return "trailor";
        case STORAGE: return "storage";
        default: return "unknown";
    }
}

int compare_boats(const void* a, const void* b) {
    Boat* boatA = *(Boat**)a;
    Boat* boatB = *(Boat**)b;
    return strcasecmp(boatA->name, boatB->name);
}

void sort_boats() {
    qsort(boats, boat_count, sizeof(Boat*), compare_boats);
}

void load_boats(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        exit(1);
    }

    char line[256];
    while (fgets(line, sizeof(line), file) && boat_count < MAX_BOATS) {
        if (strlen(line) <= 1) continue;

        Boat* boat = malloc(sizeof(Boat));
        if (!boat) {
            printf("Memory allocation failed\n");
            fclose(file);
            return;
        }

        char* name = strtok(line, ",");
        char* length_str = strtok(NULL, ",");
        char* type_str = strtok(NULL, ",");
        char* extra = strtok(NULL, ",");
        char* owed_str = strtok(NULL, "\n");

        if (!name || !length_str || !type_str || !extra || !owed_str) {
            printf("Invalid line: %s\n", line);
            free(boat);
            continue;
        }

        strncpy(boat->name, name, sizeof(boat->name));
        boat->length = atof(length_str);
        boat->type = parse_location_type(type_str);
        boat->amount_owed = atof(owed_str);

        switch (boat->type) {
            case SLIP: boat->location.slip_number = atoi(extra); break;
            case LAND: boat->location.bay_letter = extra[0]; break;
            case TRAILOR: strncpy(boat->location.trailor_tag, extra, sizeof(boat->location.trailor_tag)); break;
            case STORAGE: boat->location.storage_number = atoi(extra); break;
        }

        boats[boat_count++] = boat;
    }
    fclose(file);
    sort_boats();
}

void save_boats(const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("Error saving file");
        return;
    }

    for (int i = 0; i < boat_count; i++) {
        Boat* b = boats[i];
        fprintf(file, "%s,%.0f,%s,", b->name, b->length, location_type_to_string(b->type));
        switch (b->type) {
            case SLIP: fprintf(file, "%d,", b->location.slip_number); break;
            case LAND: fprintf(file, "%c,", b->location.bay_letter); break;
            case TRAILOR: fprintf(file, "%s,", b->location.trailor_tag); break;
            case STORAGE: fprintf(file, "%d,", b->location.storage_number); break;
        }
        fprintf(file, "%.2f\n", b->amount_owed);
    }
    fclose(file);
}

void print_inventory() {
    for (int i = 0; i < boat_count; i++) {
        Boat* b = boats[i];
        printf("%-20s %4.0f' ", b->name, b->length);
        switch (b->type) {
            case SLIP: printf("   slip   # %-3d", b->location.slip_number); break;
            case LAND: printf("   land      %c", b->location.bay_letter); break;
            case TRAILOR: printf(" trailor %s", b->location.trailor_tag); break;
            case STORAGE: printf("storage   # %-3d", b->location.storage_number); break;
        }
        printf("   Owes $%7.2f\n", b->amount_owed);
    }
}

void insert_boat_sorted(Boat* boat) {
    int i = boat_count - 1;
    while (i >= 0 && strcasecmp(boats[i]->name, boat->name) > 0) {
        boats[i + 1] = boats[i];
        i--;
    }
    boats[i + 1] = boat;
    boat_count++;
}

void add_boat_from_csv_line(char* line) {
    if (boat_count >= MAX_BOATS) {
        printf("Marina is full.\n");
        return;
    }

    Boat* boat = malloc(sizeof(Boat));
    if (!boat) {
        printf("Memory allocation failed\n");
        return;
    }

    char* name = strtok(line, ",");
    char* length_str = strtok(NULL, ",");
    char* type_str = strtok(NULL, ",");
    char* extra = strtok(NULL, ",");
    char* owed_str = strtok(NULL, "\n");

    if (!name || !length_str || !type_str || !extra || !owed_str) {
        printf("Invalid input format.\n");
        free(boat);
        return;
    }

    strncpy(boat->name, name, sizeof(boat->name));
    boat->length = atof(length_str);
    boat->type = parse_location_type(type_str);
    boat->amount_owed = atof(owed_str);

    switch (boat->type) {
        case SLIP: boat->location.slip_number = atoi(extra); break;
        case LAND: boat->location.bay_letter = extra[0]; break;
        case TRAILOR: strncpy(boat->location.trailor_tag, extra, sizeof(boat->location.trailor_tag)); break;
        case STORAGE: boat->location.storage_number = atoi(extra); break;
    }

    insert_boat_sorted(boat);
}

int find_boat_index_by_name(const char* name) {
    for (int i = 0; i < boat_count; i++) {
        if (strcasecmp(boats[i]->name, name) == 0) {
            return i;
        }
    }
    return -1;
}

void remove_boat(const char* name) {
    int index = find_boat_index_by_name(name);
    if (index == -1) {
        printf("No boat with that name\n");
        return;
    }
    free(boats[index]);
    for (int i = index; i < boat_count - 1; i++) {
        boats[i] = boats[i + 1];
    }
    boat_count--;
}

void accept_payment(const char* name, float amount) {
    int index = find_boat_index_by_name(name);
    if (index == -1) {
        printf("No boat with that name\n");
        return;
    }
    if (amount > boats[index]->amount_owed) {
        printf("That is more than the amount owed, $%.2f\n", boats[index]->amount_owed);
        return;
    }
    boats[index]->amount_owed -= amount;
}

void advance_month() {
    for (int i = 0; i < boat_count; i++) {
        float rate = 0;
        switch (boats[i]->type) {
            case SLIP: rate = 12.50; break;
            case LAND: rate = 14.00; break;
            case TRAILOR: rate = 25.00; break;
            case STORAGE: rate = 11.20; break;
        }
        boats[i]->amount_owed += boats[i]->length * rate;
    }
}

void free_all_boats() {
    for (int i = 0; i < boat_count; i++) {
        free(boats[i]);
    }
}

void show_menu() {
    printf("\n(I)nventory, (A)dd, (R)emove, (P)ayment, (M)onth, e(X)it : ");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <BoatData.csv>\n", argv[0]);
        return 1;
    }

    const char* filename = argv[1];
    load_boats(filename);

    printf("Welcome to the Boat Management System\n");
    printf("-------------------------------------\n");

    char input[256];
    while (1) {
        show_menu();
        if (!fgets(input, sizeof(input), stdin)) break;

        char choice = tolower(input[0]);
        if (choice == 'i') {
            print_inventory();
        } else if (choice == 'a') {
            printf("Please enter the boat data in CSV format                 : ");
            if (fgets(input, sizeof(input), stdin)) {
                input[strcspn(input, "\n")] = 0;
                add_boat_from_csv_line(input);
            }
        } else if (choice == 'r') {
            printf("Please enter the boat name                               : ");
            if (fgets(input, sizeof(input), stdin)) {
                input[strcspn(input, "\n")] = 0;
                remove_boat(input);
            }
        } else if (choice == 'p') {
            printf("Please enter the boat name                               : ");
            if (fgets(input, sizeof(input), stdin)) {
                input[strcspn(input, "\n")] = 0;
                int index = find_boat_index_by_name(input);
                if (index == -1) {
                    printf("No boat with that name\n");
                    continue;
                }
                printf("Please enter the amount to be paid                       : ");
                if (fgets(input, sizeof(input), stdin)) {
                    float amount = atof(input);
                    accept_payment(boats[index]->name, amount);
                }
            }
        } else if (choice == 'm') {
            advance_month();
        } else if (choice == 'x') {
            printf("Exiting the Boat Management System\n");
            save_boats(filename);
            free_all_boats();
            return 0;
        } else {
            printf("Invalid option %c\n", input[0]);
        }
    }

    free_all_boats();
    return 0;
}

