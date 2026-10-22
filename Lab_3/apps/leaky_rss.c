#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_RECORDS 100
#define MSG_SIZE    256

typedef struct {
    int id;
    char *name;
    int *scores;
} Record;

/* In dong VmRSS cua chinh tien trinh nay */
static void print_rss(const char *label) {
    char line[128];
    FILE *f = fopen("/proc/self/status", "r");
    if (!f)
        return;
    while (fgets(line, sizeof(line), f))
        if (strncmp(line, "VmRSS:", 6) == 0)
            printf("%s %s", label, line);
    fclose(f);
}

Record *create_record(int id, const char *name, int num_scores) {
    Record *r = malloc(sizeof(Record));
    r->id     = id;
    r->name   = malloc(strlen(name) + 1);
    strcpy(r->name, name);
    r->scores = malloc(sizeof(int) * num_scores);
    for (int i = 0; i < num_scores; i++)
        r->scores[i] = id * 10 + i;
    return r;
}

void delete_record(Record *r) {
#ifdef FIXED
    free(r->name);
    free(r->scores);
#endif
    free(r);
}

void process_messages(int count) {
    for (int i = 0; i < count; i++) {
        char *buf = malloc(MSG_SIZE);
        snprintf(buf, MSG_SIZE, "Message #%d: data payload here", i);
#ifdef FIXED
        free(buf);
#endif
    }
}

int main(void) {
    print_rss("[truoc]");

    Record **records = malloc(sizeof(Record *) * NUM_RECORDS);
    for (int i = 0; i < NUM_RECORDS; i++)
        records[i] = create_record(i, "EmbeddedLinuxStudent", 5);
    for (int i = 0; i < NUM_RECORDS; i++)
        delete_record(records[i]);
    free(records);

    process_messages(50);

    print_rss("[sau]");
    return 0;
}
