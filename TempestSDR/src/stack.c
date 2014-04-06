#include "stack.h"
#include <stdlib.h>

struct stack;
typedef struct stack * stack_t;

struct stack {
	int data0;
	int data1;
	struct stack * prev;
};

void stack_init(stack_t * stack) {
	(*stack) = (struct stack *) malloc(sizeof(struct stack));
	(*stack)->prev = NULL;
}

void stack_push(stack_t * stack, int val0, int val1) {
	if (*stack == NULL) return;
	struct stack * prev = *stack;

	prev->data0 = val0;
	prev->data1 = val1;
	(*stack) = (struct stack *) malloc(sizeof(struct stack));
	(*stack)->prev = prev;
}

int stack_pop(stack_t * stack, int * val0, int * val1) {
	if (*stack == NULL) return 0;
	const int empty = (*stack)->prev == NULL;

	if ((*stack)->prev != NULL) {
		*val0 = (*stack)->prev->data0;
		*val1 = (*stack)->prev->data1;
	}

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
	int temp0, temp1;
	while (*stack != NULL)
		stack_pop(stack, &temp0, &temp1);
}

void stack_purge(stack_t * stack) {
	stack_free(stack);
	stack_init(stack);
}

int stack_contains(stack_t * stack, int val0, int val1) {
	if (*stack == NULL) return 0;
	struct stack * ptr = (*stack)->prev;
	int count = 0;
	while (ptr != NULL) {
		if (ptr->data0 == val0 && ptr->data1 == val1)
			count++;
		ptr = ptr->prev;
	}
	return count;
}
