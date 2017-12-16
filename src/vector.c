#include "vector.h"

#include <stdlib.h>
#include <string.h>

void Vector_Create(Vector *vector, size_t initialCapacity)
{
	vector->data = malloc(initialCapacity * sizeof(void *));
	vector->size = 0;
	vector->capacity = initialCapacity;
}
void Vector_Destroy(Vector *vector)
{
	free(vector->data);
}
void *Vector_Get(const Vector *vector, size_t index)
{
	return vector->data[index];
}
void Vector_Set(const Vector *vector, size_t index, void *value)
{
	vector->data[index] = value;
}
static void Vector_IncreaseCapacity(Vector *vector)
{
	size_t newCapacity = vector->capacity + vector->capacity / 2;
	void *newData = malloc(newCapacity * sizeof(void *));
	memcpy(newData, vector->data, vector->size * sizeof(void *));
	free(vector->data);
	vector->data = newData;
	vector->capacity = newCapacity;
}
void Vector_Push(Vector *vector, void *value)
{
	if (vector->size == vector->capacity)
	{
		Vector_IncreaseCapacity(vector);
	}
	
	vector->data[vector->size] = value;
	++vector->size;
}
void *Vector_Pop(Vector *vector)
{
	--vector->size;
	return vector->data[vector->size];
}
void Vector_InsertAt(Vector *vector, size_t index, void *value)
{
	if (vector->size == vector->capacity)
	{
		Vector_IncreaseCapacity(vector);
	}
	
	memmove(vector->data + index + 1, vector->data + index, vector->size - index);
	vector->data[index] = value;
	++vector->size;
}
void *Vector_RemoveAt(Vector *vector, size_t index)
{
	void *result = vector->data[index];
	memmove(vector->data + index, vector->data + index + 1, vector->size - index - 1);
	--vector->size;
	return result;
}
void Vector_Clear(Vector *vector)
{
	vector->size = 0;
}