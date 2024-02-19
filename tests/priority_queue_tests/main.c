#include <stdio.h>
#include <stdint.h>

#define PARENT_INDEX(IDX) (((IDX) - 1) / 2)
#define LEFT_CHILD_INDEX(IDX) (2 * (IDX) + 1)
#define RIGHT_CHILD_INDEX(IDX) (2 * (IDX) + 2)

#define PARENT(PQ, IDX) ((PQ)->arr[PARENT_INDEX((IDX))])
#define LEFT_CHILD(PQ, IDX) ((PQ)->arr[LEFT_CHILD_INDEX((IDX))])
#define RIGHT_CHILD(PQ, IDX) ((PQ)->arr[RIGHT_CHILD_INDEX((IDX))])

#define RR 0xf

/* rr priority reset condition:
 * 1. the rr priority of any of the threads reaches 0
 * 2. a task is unsuspended
 *
 * changing the rr priority does not require you to re-heapify
 *
 * ready-ing: O(log(n));
 * suspending: O(log(n));
 * getting top element priorty: O(1);
 *
 * round robing execution for threads with the same priority:
 * O(n)
 */

struct pq_elem_st {
	size_t id; /* < the id of a element; must be unique */
	uint8_t data; /* < the data by which the priority queue is sorted */
	uint8_t rr_priority;
};

struct priority_queue_st {
	struct pq_elem_st *arr;
	size_t arr_size; /* < the allocated size for arr */
	size_t pq_size; /* < the amount of elements inserted into the pq */
};

int elem_cmp(struct pq_elem_st *a, struct pq_elem_st *b);
void swap_elem(struct pq_elem_st *a, struct pq_elem_st *b);
int pq_heapify(struct priority_queue_st *pq);
int pq_insert_element(struct priority_queue_st *pq, int id, uint8_t data,
		      uint8_t rr_priority);
struct pq_elem_st pq_pop_from_top(struct priority_queue_st *pq);
int pq_delete_element_by_id(struct priority_queue_st *pq);

int
elem_cmp(struct pq_elem_st *a, struct pq_elem_st *b)
{
	if (a->data == b->data)
		return a->rr_priority - b->rr_priority;
	return a->data - b->data;
}

void
swap_elem(struct pq_elem_st *a, struct pq_elem_st *b)
{
	struct pq_elem_st aux;

	aux = *a;
	*a = *b;
	*b = aux;
}

int
pq_heapify(struct priority_queue_st *pq)
{
}

int
pq_insert_element(struct priority_queue_st *pq, int id, uint8_t data,
		  uint8_t rr_priority)
{
	size_t idx;
	uint8_t swapped;

	if (pq == NULL)
		return 1;

	/* BEGIN_ATOMIC */

	if (pq->pq_size >= pq->arr_size)
		return 1;
	
	/* insert the element */
	pq->arr[pq->pq_size].id = id;
	pq->arr[pq->pq_size].data = data;
	pq->arr[pq->pq_size].rr_priority = rr_priority;
	pq->pq_size++;

	/* restore the heap structure:
	 * if the parent has a lower value, then swap them around
	 */
	idx = pq->pq_size - 1;
	swapped = 1;
	while (swapped && idx > 0) {
		swapped = 0;
		if (elem_cmp(&pq->arr[idx], &PARENT(pq, idx)) > 0) {
			swap_elem(&PARENT(pq, idx), &pq->arr[idx]);

			idx = PARENT_INDEX(idx);
			swapped = 1;
		}
	}

	/* END_ATOMIC */

	return 0;
}

struct pq_elem_st
pq_top(struct priority_queue_st *pq)
{
	size_t idx, max_child_idx;
	struct pq_elem_st ret = {0};

	if (pq == NULL || pq->pq_size == 0)
		return ret;

	/* BEGIN_ATOMIC */

	ret = pq->arr[0];

	/* END_ATOMIC */

	return ret;
}

void
pq_decrease_top_rr(struct priority_queue_st *pq)
{
	size_t idx, max_child_idx;
	struct pq_elem_st ret = {0};

	if (pq == NULL || pq->pq_size == 0)
		return;

	if (pq->arr[0].data > pq->arr[1].data &&
	    pq->arr[0].data > pq->arr[2].data) {
	    	pq->arr[0].rr_priority = RR;
		return;
	}

	/* BEGIN_ATOMIC */

	if (pq->arr[0].rr_priority == 1)
		for (idx = 0; idx < pq->pq_size; idx++)
			if (pq->arr[0].data == pq->arr[idx].data)
				pq->arr[idx].rr_priority = RR;
	pq->arr[0].rr_priority--;

	idx = 0;
	while (LEFT_CHILD_INDEX(idx) < pq->pq_size) {
		/* if both children are lesser than the current element, then
		 * the heap condition is met
		 */
		if (elem_cmp(&LEFT_CHILD(pq, idx), &pq->arr[idx]) < 0 && 
		    elem_cmp(&RIGHT_CHILD(pq, idx), &pq->arr[idx]) < 0)
			break;

		if (elem_cmp(&LEFT_CHILD(pq, idx), &RIGHT_CHILD(pq, idx)) > 0)
			max_child_idx = LEFT_CHILD_INDEX(idx);
		else
			max_child_idx = RIGHT_CHILD_INDEX(idx);

		swap_elem(&pq->arr[max_child_idx], &pq->arr[idx]);

		idx = max_child_idx;
	}


	/* END_ATOMIC */
}

struct pq_elem_st
pq_pop_from_top(struct priority_queue_st *pq)
{
	size_t idx, max_child_idx;
	struct pq_elem_st ret = {0};

	if (pq == NULL || pq->pq_size == 0)
		return ret;
	
	/* BEGIN_ATOMIC */

	if (pq->arr[0].rr_priority == 0)
		for (idx = 0; idx < pq->pq_size; idx++)
			if (pq->arr[0].data == pq->arr[idx].data)
				pq->arr[idx].rr_priority = RR;
	pq->arr[0].rr_priority--;

	ret = pq->arr[0];
	pq->arr[0] = pq->arr[pq->pq_size - 1];
	pq->pq_size--;

	idx = 0;
	while (LEFT_CHILD_INDEX(idx) < pq->pq_size) {
		/* if both children are lesser than the current element, then
		 * the heap condition is met
		 */
		if (elem_cmp(&LEFT_CHILD(pq, idx), &pq->arr[idx]) <= 0 && 
		    elem_cmp(&RIGHT_CHILD(pq, idx), &pq->arr[idx]) <= 0)
			break;

		if (elem_cmp(&LEFT_CHILD(pq, idx), &RIGHT_CHILD(pq, idx)) > 0)
			max_child_idx = LEFT_CHILD_INDEX(idx);
		else
			max_child_idx = RIGHT_CHILD_INDEX(idx);

		swap_elem(&pq->arr[max_child_idx], &pq->arr[idx]);

		idx = max_child_idx;
	}

	/* END_ATOMIC */

	return ret;
}

int
pq_delete_element_by_id(struct priority_queue_st *pq)
{
}

int
main(void)
{
	size_t i;
	static struct pq_elem_st pq_arr[100];
	struct pq_elem_st x;
	struct priority_queue_st my_priority_queue = {0};

	my_priority_queue.arr = pq_arr;
	my_priority_queue.arr_size = 100;

	pq_insert_element(&my_priority_queue, 1, 2, RR);
	pq_insert_element(&my_priority_queue, 2, 2, RR);
	pq_insert_element(&my_priority_queue, 3, 2, RR);
	pq_insert_element(&my_priority_queue, 4, 2, RR);
	pq_insert_element(&my_priority_queue, 5, 2, RR);
	pq_insert_element(&my_priority_queue, 6, 1, RR);
	pq_insert_element(&my_priority_queue, 7, 1, RR);

	while (i++ < RR * 7) {
		if ((i - 1) % 5 == 0)
			printf("\n");
		x = pq_top(&my_priority_queue);
		printf("%d %d %d\n", x.id, x.data, x.rr_priority);
		pq_decrease_top_rr(&my_priority_queue);
	}

	return 0;
}

