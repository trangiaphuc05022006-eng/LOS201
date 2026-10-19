#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_RECORDS  100
#define MSG_SIZE     256

typedef struct {
    int  id;
    char *name;     
    int  *scores;   
} Record;

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

/* ĐÃ SỬA: Bỏ comment free(r->name) và free(r->scores) */
void delete_record(Record *r) {
    free(r->name);   
    free(r->scores); 
    free(r);
}

/* ĐÃ SỬA: Thêm free(buf) sau khi dùng xong */
void process_messages(int count) {
    for (int i = 0; i < count; i++) {
        char *buf = malloc(MSG_SIZE);
        snprintf(buf, MSG_SIZE, "Message #%d: data payload here", i);
        if (i % 10 == 0)
            printf("  Processed: %s\n", buf);
        free(buf);  /* Đã giải phóng bộ nhớ buffer */
    }
}

int main(void) {
    printf("=== Leaky App FIXED – LAB-03 Memory Analysis ===\n");
    
    printf("[1] Creating %d records...\n", NUM_RECORDS);
    Record **records = malloc(sizeof(Record*) * NUM_RECORDS);
    for (int i = 0; i < NUM_RECORDS; i++)
        records[i] = create_record(i, "EmbeddedLinuxStudent", 5);

    printf("[2] Deleting records (fixed)...\n");
    for (int i = 0; i < NUM_RECORDS; i++)
        delete_record(records[i]);
    free(records);

    printf("[3] Processing messages (fixed)...\n");
    process_messages(50);

    printf("Program finished successfully – no leaks!\n");
    return 0;
}
