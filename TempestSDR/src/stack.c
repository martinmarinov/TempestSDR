#include "stack.h"
#include <stdlib.h>

struct stack;
typedef struct stack * stack_t;

struct stack {
	int data;
	struct stack * prev;
};

void stack_init(stack_t * stack) {
	(*stack) = (struct stack *) malloc(sizeof(struct stack));
	(*stack)->prev = NULL;
}

void stack_push(stack_t * stack, int val) {
	if (*stack == NULL) return;
	struct stack * prev = *stack;

	prev->data = val;
	(*stack) = (struct stack *) malloc(sizeof(struct stack));
	(*stack)->prev = prev;
}

int stack_pop(stack_t * stack, int * val) {
	if (*stack == NULL) return 0;
	const int empty = (*stack)->prev == NULL;

	if ((*stack)->prev != NULL)
		*val = (*stack)->prev->data;

	struct stack * last = (*stack);
	(*stack) = (*stack)->prev;
	free (last);

	return !empty;
}

int stack_size(stack_t * stack) {
	if (*stack == NULL) return 0;
	int c = -1;
	struct stack * ptr = *stack;
	while (ptr != NULL) {
		ptr = ptr->prev;
		c++;
	}
	return c;
}

void stack_free(stack_t * stack) {
	if (*stack == NULL) return;
	int temp;
	while (*stack != NULL)
		stack_pop(stack, &temp);
}

void stack_purge(stack_t * stack) {
	stack_free(stack);
	stack_init(stack);
}

int stack_contains(stack_t * stack, int val) {
	if (*stack == NULL) return 0;
	struct stack * ptr = (*stack)->prev;
	while (ptr != NULL) {
		if (ptr->data == val)
			return 1;
		ptr = ptr->prev;
	}
	return 0;
}
