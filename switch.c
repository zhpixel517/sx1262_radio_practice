#include <stdio.h>

typedef enum { STATE_LOCKED, STATE_UNLOCKED } state_t;
typedef enum { EVENT_COIN, EVENT_PUSH} event_t;

static const char *state_name(state_t s)
{
    switch (s) {
        case STATE_LOCKED: return "LOCKED";
        case STATE_UNLOCKED: return "UNLOCKED";
    }
    return "?";
}

static state_t turnstile_handle(state_t state, event_t event)
{
    switch(state) {
        case STATE_LOCKED: {
            if (event == EVENT_COIN){
                printf("  coin accepted, unlocking\n");
                return STATE_UNLOCKED;
            }
            if (event == EVENT_PUSH) {
                printf(" locked, insert a coin first\n");
            }
            break;
        }

        case STATE_UNLOCKED: {
            if (event == EVENT_PUSH) {
                printf(" person passed through, locking\n");
                return STATE_LOCKED;
            }
            if (event == EVENT_COIN){
                printf(" already unlocked, returning coin\n");
            }
            break;
        }
    }
    return state;
}

int main(void)
{
    state_t state = STATE_LOCKED;
    event_t script[] = {EVENT_PUSH, EVENT_COIN, EVENT_COIN, EVENT_PUSH, EVENT_PUSH };
    int n = sizeof(script) / sizeof(script[0]);

    for (int i = 0; i < n; i++) {
        printf("[%s] got %s\n", state_name(state), script[i] == EVENT_COIN ? "COIN" : "PUSH");
        state = turnstile_handle(state, script[i]);
    }
    printf("final state %s\n", state_name(state));
    return 0;
}