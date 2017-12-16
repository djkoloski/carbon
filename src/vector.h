#pragma once

#include "util.h"

typedef struct Vector Vector;

struct Vector
{
	void **data;
	size_t size;
	size_t capacity;
};

void Vector_Create(Vector *vector, size_t initialCapacity);
void Vector_Destroy(Vector *vector);
void *Vector_Get(const Vector *vector, size_t index);
void Vector_Set(const Vector *vector, size_t index, void *value);
void Vector_Push(Vector *vector, void *value);
void *Vector_Pop(Vector *vector);
void *Vector_RemoveAt(Vector *vector, size_t index);
void Vector_Clear(Vector *vector);