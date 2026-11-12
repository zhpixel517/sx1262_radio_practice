#include <stdio.h>

typedef enum { STATE_LOCKED, STATE_UNLOCKED, STATE_COUNT } state_t;
typedef enum { EVENT_COIN, EVENT_PUSH, EVENT_COUNT } event_t;

typedef void (*action_fn)(void);

static void unlock(void)       { printf("  coin accepted, unlocking\n"); }
static void lock(void)         { printf("  person passed through, locking\n"); }
static void refund(void)       { printf("  already unlocked, returning coin\n"); }
static void complain(void)     { printf("  locked! insert a coin first\n"); }

typedef struct {
    state_t next;
    action_fn action; // could be NULL
} transition_t;

static const transition_t table[STATE_COUNT][EVENT_COUNT] = {
    [STATE_LOCKED] = {
        [EVENT_COIN] = {STATE_UNLOCKED, unlock},
        [EVENT_PUSH] = {STATE_LOCKED, complain}
    },
    [STATE_UNLOCKED] = {
        [EVENT_COIN] = {STATE_UNLOCKED, refund},
        [EVENT_PUSH] = {STATE_LOCKED, lock},
    }
};

static state_t dispatch(state_t state, event_t event)
{
    const transition_t *t = &table[state][event];
    if (t->action != NULL){
        t->action();
    }
    return t->next;
}

int main(void)
{
    static const char *names[] = {"LOCKED", "UNLOCKED"};
    state_t state = STATE_LOCKED;
    event_t script[] = { EVENT_PUSH, EVENT_COIN, EVENT_COIN, EVENT_PUSH, EVENT_PUSH };

    for (unsigned i = 0; i < sizeof(script) / sizeof(script[0]); i++)
    {
        printf("[%s] got %s\n", names[state], script[i] == EVENT_COIN ? "COIN" : "PUSH");
        state = dispatch(state, script[i]);
    }
    printf("final state: %s\n", names[state]);
    return 0;
}