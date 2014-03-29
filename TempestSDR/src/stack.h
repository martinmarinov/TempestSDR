#ifndef STACK_H_
#define STACK_H_

struct stack;
typedef struct stack * stack_t;

void stack_init(stack_t * stack);
void stack_push(stack_t * stack, int val);
int stack_pop(stack_t * stack, int * val);
void stack_free(stack_t * stack);
void stack_purge(stack_t * stack);
int stack_size(stack_t * stack);
int stack_contains(stack_t * stack, int val);

/** USAGE **\

int main(void) {
	stack_t stack;
	stack_init (&stack);

	stack_push(&stack, 0);
	stack_push(&stack, 1);
	stack_push(&stack, 2);
	stack_push(&stack, 3);

	int val;
	while (stack_pop(&stack, &val))
		printf("%d\n", val);

	stack_free (&stack);
	return 0;
}

 -- OUTPUT --

	3
	2
	1
	0

\**       **/


#endif
