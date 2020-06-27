#ifndef _ring_buffer_H_
#define _ring_buffer_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * Ring buffer type.
 *
 * The buffer is empty when head == tail.
 *
 * The buffer is full when the head is one byte in front of the tail,
 * modulo buffer length.
 *
 * One byte is left free to distinguish empty from full. */
typedef struct
{
    volatile uint8_t * buf; /**< Buffer items are stored into */
    volatile uint32_t head; /**< Index of the next item to remove */
    volatile uint32_t tail; /**< Index where the next item will get inserted */
    volatile uint32_t size; /**< Buffer capacity minus one */
} ring_buffer_t;

/**
 * Initialise a ring buffer.
 *
 *  @param rb   Instance to initialise
 *
 *  @param size Number of items in buf
 *
 *  @param buf  Buffer to store items into
 */
static inline void rb_init(ring_buffer_t * rb, uint32_t size, uint8_t * buf)
{
    rb->head = 0;
    rb->tail = 0;
    rb->size = size;
    rb->buf = buf;
}

/**
 * @brief Return the max number of elements that can be stored in the ring buffer.
 * @param rb Buffer whose elements to count.
 */
static inline uint32_t rb_capacity(ring_buffer_t * rb)
{
    return rb->size;
}

/**
 * @brief Return the number of elements stored in the ring buffer.
 * @param rb Buffer whose elements to count.
 */
static inline uint32_t rb_occupancy(ring_buffer_t * rb)
{
    return rb->tail - rb->head;
}

/**
 * @brief Return the number of free spaces in the ring buffer.
 * @param rb Buffer whose elements to count.
 */
static inline uint32_t rb_free(ring_buffer_t * rb)
{
    return rb->size - rb_occupancy(rb);
}

/**
 * @brief Returns true if and only if the ring buffer is full.
 * @param rb Buffer to test.
 */
static inline bool rb_is_full(ring_buffer_t * rb)
{
    return rb_occupancy(rb) == rb->size;
}

/**
 * @brief Returns true if and only if the ring buffer is empty.
 * @param rb Buffer to test.
 */
static inline bool rb_is_empty(ring_buffer_t * rb)
{
    return rb->head == rb->tail;
}

/**
 * Append element onto the end of a ring buffer.
 * @param rb Buffer to append onto.
 * @param element Value to append.
 */
static inline void rb_insert(ring_buffer_t * rb, uint8_t element)
{
    rb->buf[rb->tail % rb->size] = element;
    rb->tail++;
}

/**
 * @brief Remove and return the first item from a ring buffer.
 * @param rb Buffer to remove from, must contain at least one element.
 */
static inline uint8_t rb_remove(ring_buffer_t * rb)
{
    uint8_t ch = rb->buf[rb->head % rb->size];
    rb->head++;
    return ch;
}

/*
 * @brief Return the first item from a ring buffer, without removing it
 * @param rb Buffer to remove from, must contain at least one element.
 */
static inline uint8_t rb_peek(ring_buffer_t * rb)
{
    return rb->buf[rb->head % rb->size];
}

/*
 * @brief Return the nth element from a ring buffer, without removing it
 * @param rb Buffer to remove from, must contain at least one element.
 */
static inline uint8_t rb_peek_at(ring_buffer_t * rb, uint32_t element)
{
    return rb->buf[(rb->head + element) % rb->size];
}

/**
 * @brief Attempt to remove the first item from a ring buffer.
 *
 * If the ring buffer is nonempty, removes and returns its first item.
 * If it is empty, does nothing and returns a negative value.
 *
 * @param rb Buffer to attempt to remove from.
 */
static inline int32_t rb_safe_remove(ring_buffer_t * rb)
{
    return rb_is_empty(rb) ? -1 : rb_remove(rb);
}

/**
 * @brief Attempt to insert an element into a ring buffer.
 *
 * @param rb Buffer to insert into.
 * @param element Value to insert into rb.
 * @sideeffect If rb is not full, appends element onto buffer.
 * @return If element was appended, then true; otherwise, false. */
static inline bool rb_safe_insert(ring_buffer_t * rb, uint8_t element)
{
    if (rb_is_full(rb))
        return false;

    rb_insert(rb, element);
    return true;
}

/**
 * @brief Append an item onto the end of a non-full ring buffer.
 *
 * If the buffer is full, removes its first item, then inserts the new
 * element at the end.
 *
 * @param rb Ring buffer to insert into.
 * @param element Value to insert into ring buffer.
 * @return returns true if element was removed else false
 */
static inline bool rb_push_insert(ring_buffer_t * rb, uint8_t element)
{
    bool ret = false;
    if (rb_is_full(rb))
    {
        rb_remove(rb);
        ret = true;
    }
    rb_insert(rb, element);
    return ret;
}

/**
 * @brief Discard all items from a ring buffer.
 * @param rb Ring buffer to discard all items from.
 */
static inline void rb_reset(ring_buffer_t * rb)
{
    rb->tail = rb->head;
}

#endif /* _ring_buffer_H_ */
