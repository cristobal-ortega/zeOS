#ifndef __CBUFFER_H__
#define __CBUFFER_H__

/* Simple circular buffer implementation w/ static memory.
 * Distinguishes FULL from EMPTY by keeping count of the data elements.
 */

#define BUFFER_SIZE 32

struct circular_buffer {
    int         size;   /* maximum number of elements           */
    int		count;
    int       start;  /* index of oldest element              */
    int       end;    /* index at which to write new element  */
    char data[BUFFER_SIZE];
};

static inline void cbInit(struct circular_buffer *cb, int size)
{
    cb->size  = BUFFER_SIZE;
    cb->start = 0;
    cb->count = 0;
}
 
static inline int cbIsFull(struct circular_buffer *cb)
{
    return cb->count == cb->size;
}
 
static inline int cbIsEmpty(struct circular_buffer *cb)
{
    return cb->count == 0;
}

static inline int cbCount(struct circular_buffer *cb)
{
    return cb->count;
} 
 
static inline void cbWrite(struct circular_buffer *cb, char *elem)
{
	if (cb->count != cb->size) {
		int end = (cb->start + cb->count) % cb->size;
		cb->data[end] = *elem;
		++(cb->count);
	}
}
 
static inline void cbRead(struct circular_buffer *cb, char *elem)
{
    *elem = cb->data[cb->start];
    cb->start = (cb->start + 1) % cb->size;
    --(cb->count);
}
static inline void cbRead2(struct circular_buffer *cb, char *buf, int count)
{
	int i = 0;
	for (i = 0; i < count; ++i) {
		*(buf + i) = cb->data[cb->start];
		cb->start = (cb->start + 1) % cb->size;
	}
	cb->count = cb->count - count;
}
#endif /* __CBUFFER_H__ */
