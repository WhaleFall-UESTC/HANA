/**
 * This code is from Stephen's OS (MIT License)
 * Original source: https://github.com/brenns10/sos/blob/master/lib/list.c
 * Copyright (c) 2018-2022 Stephen Brennan
 * For full license text, see LICENSE-MIT-sos file in this repository
 * 
 * list.c: a dead simple, C89 linked list library
 *
 * See list.h
 */

#include <tools/list.h>

void hlist_insert(struct hlist_head *head, struct hlist_head *item)
{
    item->next = head->next;
    head->next = item;
}

void hlist_remove(struct hlist_head *parent_or_head, struct hlist_head *item)
{
    while (parent_or_head->next != item)
        parent_or_head = parent_or_head->next;
    parent_or_head->next = item->next;
}

void list_insert(struct list_head *head, struct list_head *item)
{
    item->next = head->next;
    item->prev = head;
    item->next->prev = item;
    head->next = item;
}

void list_insert_end(struct list_head *head, struct list_head *item)
{
    item->next = head;
    item->prev = head->prev;
    head->prev->next = item;
    head->prev = item;
}

void list_remove(struct list_head *item)
{
    item->prev->next = item->next;
    item->next->prev = item->prev;
}

void list_init(struct list_head *head)
{
    head->prev = head;
    head->next = head;
}
