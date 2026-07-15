#include "fraus/arena.h"

#include <stdlib.h>

static const size_t frDefaultCapacity = 0x4000;

void frArenaCreate(FrArena* const arena)
{
	*arena = (FrArena){};
}

static void* frArenaNewRegion(FrArena* const arena, FrRegion* const region, const size_t capacity, const size_t size)
{
	FrRegion* const newRegion = malloc(sizeof(*newRegion) + capacity);
	if(!newRegion)
	{
		return nullptr;
	}

	*newRegion = (FrRegion){
		.previous = region ? region->previous : nullptr,
		.next = region ? region->next : nullptr,
		.capacity = capacity,
		.size = size
	};

	free(region);

	if(newRegion->previous)
	{
		newRegion->previous->next = newRegion;
	}
	if(newRegion->next)
	{
		newRegion->next->previous = newRegion;
	}

	if(arena->start == region)
	{
		arena->start = newRegion;
	}
	arena->end = newRegion;

	return newRegion->data;
}

void frArenaFree(FrArena* const arena)
{
	FrRegion* region = arena->start;
	while(region)
	{
		FrRegion* const next = region->next;

		free(region);

		region = next;
	}

	*arena = (FrArena){};
}

void* frArenaAllocate(FrArena* const arena, const size_t size, const size_t alignment)
{
	FrRegion* region = arena->end;
	while(region)
	{
		if(region->size == 0)
		{
			if(region->capacity >= size)
			{
				arena->end = region;
				region->size = size;

				return region->data;
			}

			return frArenaNewRegion(arena, region, size, size);
		}

		size_t offset = region->size;

		if(region->capacity - offset < size)
		{
			goto next;
		}

		const size_t alignmentOffset = -offset & (alignment - 1);

		if(region->capacity - offset < alignmentOffset)
		{
			goto next;
		}

		offset += alignmentOffset;

		if(region->capacity - offset >= size)
		{
			region->size = offset + size;
			arena->end = region;

			return region->data + offset;
		}

		next:
		if(!region->next)
		{
			break;
		}

		region = region->next;
		region->size = 0;
	}

	const size_t capacity = size < frDefaultCapacity ? frDefaultCapacity : size;

	return frArenaNewRegion(arena, nullptr, capacity, size);
}

void frArenaClear(FrArena* const arena)
{
	if(arena->start)
	{
		arena->start->size = 0;
		arena->end = arena->start;
	}
}

FrArenaSave frArenaSave(const FrArena* const arena)
{
	return (FrArenaSave){
		.previous = arena->end ? arena->end->previous : nullptr,
		.size = arena->end ? arena->end->size : 0
	};
}

void frArenaRestore(FrArena* const arena, const FrArenaSave save)
{
	if(save.previous)
	{
		arena->end = save.previous->next;
		arena->end->size = save.size;
	}
}
